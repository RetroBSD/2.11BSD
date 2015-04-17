/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)xp.c	2.6 (2.11BSD GTE) 1998/4/3
 */

/*
 * RM02/03/05, RP04/05/06/07, CDC 9766, SI, Fuji 160 and Eagle.  This
 * driver will handle most variants of SMD drives on one or more controllers.
 */

#include "xp.h"
#if NXPD > 0

#include "param.h"
#include "../machine/seg.h"

#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "user.h"
#include "hpreg.h"
#include "dkbad.h"
#include "dk.h"
#include "disklabel.h"
#include "disk.h"
#include "file.h"
#include "map.h"
#include "uba.h"
#include "stat.h"
#include "syslog.h"

#define	XP_SDIST	2
#define	XP_RDIST	6

/*
 * 'xp' is unique amoung 2.11BSD disk drivers.  Drives are not organized
 * into groups of 8 drives per controller.  Instead drives are numbered
 * across controllers:  the second drive ("unit 1") could be on the 2nd
 * controller.  This has the effect of turning the high 5 bits of the minor
 * device number into the unit number and drives are thus numbered 0 thru 31.
 *
 * NOTE: this  is different than /boot's view of the world.  Sigh.
*/

#define	XPUNIT(dev)	(dkunit(dev) & 0x1f)

int xp_offset[] = {
	HPOF_P400,	HPOF_M400,	HPOF_P400,	HPOF_M400,
	HPOF_P800,	HPOF_M800,	HPOF_P800,	HPOF_M800,
	HPOF_P1200,	HPOF_M1200,	HPOF_P1200,	HPOF_M1200,
	0,		0,		0,		0,
};

struct xp_controller {
	struct	buf *xp_actf;		/* pointer to next active xputab */
	struct	buf *xp_actl;		/* pointer to last active xputab */
	struct	hpdevice *xp_addr;	/* csr address */
	char	xp_rh70;		/* massbus flag */
	char	xp_active;		/* nonzero if doing a transfer */
};

struct xp_drive {
	struct	xp_controller *xp_ctlr; /* controller to which slave attached */
	int	xp_unit;		/* slave number */
	u_short	xp_nsect;
	u_short	xp_ntrack;
	u_short	xp_nspc;		/* sectors/cylinder */
	u_short	xp_cc;			/* current cylinder, for RM's */
	u_long	xp_dd0;			/* drivedata longword 0 */
	struct	dkdevice xp_dk;		/* kernel resident portion of label */
	u_short	xp_ncyl;		/* cylinders per pack */
	};
/*
 * Some shorthand for accessing the in-kernel label structure.
*/
#define	xp_bopen	xp_dk.dk_bopenmask
#define	xp_copen	xp_dk.dk_copenmask
#define	xp_open		xp_dk.dk_openmask
#define	xp_flags	xp_dk.dk_flags
#define	xp_label	xp_dk.dk_label
#define	xp_parts	xp_dk.dk_parts

struct xp_controller	xp_controller[NXPC];
struct xp_drive	xp_drive[NXPD];

struct	buf	xptab;
struct	buf	xputab[NXPD];

#ifdef BADSECT
struct	dkbad	xpbad[NXPD];
struct	buf	bxpbuf[NXPD];
#endif

#ifdef UCB_METER
static	int		xp_dkn = -1;	/* number for iostat */
#endif

	int	xpstrategy();
	void	xpgetinfo();
	daddr_t	xpsize();
extern	size_t	physmem;

/*
 * Setup root SMD ('xp') device (use bootcsr passed from ROMs).  In the event
 * that the system was not booted from a SMD drive but swapdev is a SMD device
 * we attach the first (0176700) controller.  This would be a very unusual
 * configuration and is unlikely to be encountered.
 *
 * This is very confusing, it is an ugly hack, but short of moving autoconfig
 * back into the kernel there's nothing else I can think of to do.
 *
 * NOTE:  the swap device must be on the controller used for booting since 
 * that is the only one attached here - the other controllers are attached 
 * by /etc/autoconfig when it runs later.
 */

xproot(csr)
	register struct	hpdevice *csr;
	{

	if	(!csr)					/* XXX */
		csr = (struct hpdevice *)0176700;	/* XXX */
	xpattach(csr, 0);
	}

/*
 * Attach controller at xpaddr.  Mark as nonexistent if xpaddr is 0; otherwise
 * attach slaves.  This routine must be called once per controller
 * in ascending controller numbers.
 *
 * NOTE: This means that the 'xp' lines in /etc/dtab _MUST_ be in order
 * starting with 'xp 0' first.
 */

xpattach(xpaddr, unit)
	register struct hpdevice *xpaddr;
	int unit;	/* controller number */
{
	register struct xp_controller *xc = &xp_controller[unit];
	static int last_attached = -1;

#ifdef UCB_METER
	if (xp_dkn < 0) {
		dk_alloc(&xp_dkn, NXPD, "xp", 0L);
	}
#endif

	if ((unsigned)unit >= NXPC)
		return(0);
	if (xpaddr && (fioword(xpaddr) != -1)) {
		xc->xp_addr = xpaddr;
		if (fioword(&xpaddr->hpbae) != -1)
			xc->xp_rh70 = 1;
	/*
	 *  If already attached, ignore (don't want to renumber drives)
	 */
		if (unit > last_attached) {
			last_attached = unit;
			xpslave(xpaddr, xc);
		}
		return(1);
	}
	xc->xp_addr = 0;
	return(0);
}

/*
 * Determine what drives are attached to a controller; the type and geometry
 * information will be retrieved at open time from the disklabel.
 */
xpslave(xpaddr, xc)
register struct hpdevice *xpaddr;
struct xp_controller *xc;
{
	register struct xp_drive *xd;
	register struct xpst *st;
	int j, dummy;
	static int nxp = 0;

	for (j = 0; j < 8; j++) {
		xpaddr->hpcs1.w = HP_NOP;
		xpaddr->hpcs2.w = j;
		xpaddr->hpcs1.w = HP_GO;	/* testing... */
		delay(6000L);
		dummy = xpaddr->hpds;
		if (xpaddr->hpcs2.w & HPCS2_NED) {
			xpaddr->hpcs2.w = HPCS2_CLR;
			continue;
		}
		if (nxp < NXPD) {
			xd = &xp_drive[nxp++];
			xd->xp_ctlr = xc;
			xd->xp_unit = j;
/*
 * Allocate the disklabel now.  This is very early in the system's life
 * so fragmentation will be minimized if any labels are allocated from 
 * main memory.  Then initialize the flags to indicate a drive is present.
*/
			xd->xp_label = disklabelalloc();
			xd->xp_flags = DKF_ALIVE;
		}
	}
}

xpopen(dev, flags, mode)
	dev_t	dev;
	int	flags, mode;
	{
register struct xp_drive *xd;
	int	unit = XPUNIT(dev);
	int	i, part = dkpart(dev), rpm;
register int	mask;

	if	(unit >= NXPD)
		return(ENXIO);
 	xd = &xp_drive[unit];
	if	((xd->xp_flags & DKF_ALIVE) == 0)
		return(ENXIO);
/*
 * Now we read the label.  First wait for any pending opens/closes to
 * complete.
*/
	while	(xd->xp_flags & (DKF_OPENING|DKF_CLOSING))
		sleep(xd, PRIBIO);
/*
 * On first open get label (which has the geometry information as well as
 * the partition tables).  We may block reading the label so be careful to
 * stop any other opens.
*/
	if	(xd->xp_open == 0)
		{
		xd->xp_flags |= DKF_OPENING;
		xpgetinfo(xd, dev);
		xd->xp_flags &= ~DKF_OPENING;
		wakeup(xd);
		}
/*
 * Need to make sure the partition is not out of bounds.  This requires
 * mapping in the external label.  Since this only happens when a partition
 * is opened (at mount time for example) it is unlikely to be  an efficiency
 * concern.
*/
	mapseg5(xd->xp_label, LABELDESC);
	i = ((struct disklabel *)SEG5)->d_npartitions;
	rpm = ((struct disklabel *)SEG5)->d_rpm;
	normalseg5();
	if	(part >= i)
		return(ENXIO);
#ifdef	UCB_METER
	if	(xp_dkn >= 0) {
		dk_wps[xp_dkn+unit] = (long) xd->xp_nsect * (rpm / 60) * 256L;
		}
#endif
	mask = 1 << part;
	dkoverlapchk(xd->xp_open, dev, xd->xp_label, "xp");
	if	(mode == S_IFCHR)
		xd->xp_copen |= mask;
	else if	(mode == S_IFBLK)
		xd->xp_bopen |= mask;
	else
		return(EINVAL);
	xd->xp_open |= mask;
	return(0);
	}

xpclose(dev, flags, mode)
	dev_t	dev;
	int	flags, mode;
	{
	int	s;
register int mask;
register struct xp_drive *xd;
	int	unit = XPUNIT(dev);

	xd = &xp_drive[unit];
	mask = 1 << dkpart(dev);
	if	(mode == S_IFCHR)
		xd->xp_copen &= ~mask;
	else if	(mode == S_IFBLK)
		xd->xp_bopen &= ~mask;
	else
		return(EINVAL);
	xd->xp_open = xd->xp_copen | xd->xp_bopen;
	if	(xd->xp_open == 0)
		{
		xd->xp_flags |= DKF_CLOSING;
		s = splbio();
		while	(xputab[unit].b_actf)
			sleep(&xputab[unit], PRIBIO);
		xd->xp_flags &= ~DKF_CLOSING;
		splx(s);
		wakeup(xd);
		}
	return(0);
	}

void
xpdfltlbl(xd, lp)
	register struct	xp_drive *xd;
	register struct	disklabel *lp;
	{
	register struct partition *pi = &lp->d_partitions[0];

/*
 * NOTE: partition 0 ('a') is used to read the label.  Therefore 'a' must
 * start at the beginning of the disk!  If there is no label or the label
 * is corrupted then a label containing a geometry sufficient *only* to 
 * read/write sector 1 (LABELSECTOR) is created.  1 track, 1 cylinder and
 * 2 sectors per track.
*/

	bzero(lp, sizeof (*lp));
	lp->d_type = DTYPE_SMD;
	lp->d_secsize = 512;			/* XXX */
	lp->d_nsectors = LABELSECTOR + 1;	/* # sectors/track */
	lp->d_ntracks = 1;			/* # tracks/cylinder */
	lp->d_secpercyl = LABELSECTOR + 1;	/* # sectors/cylinder */
	lp->d_ncylinders = 1;			/* # cylinders */
	lp->d_npartitions = 1;			/* 1 partition  = 'a' */
/*
 * Need to put the information where the driver expects it.  This is normally
 * done after reading the label.  Since we're creating a fake label we have to
 * copy the invented geometry information to the right place.
*/
	xd->xp_nsect = lp->d_nsectors;
	xd->xp_ntrack = lp->d_ntracks;
	xd->xp_nspc = lp->d_secpercyl;
	xd->xp_ncyl = lp->d_ncylinders;
	xd->xp_dd0 = lp->d_drivedata[0];

	pi->p_size = LABELSECTOR + 1;
	pi->p_fstype = FS_V71K;
	pi->p_frag = 1;
	pi->p_fsize = 1024;
	bcopy(pi, xd->xp_parts, sizeof (lp->d_partitions));
	}

/*
 * Read disklabel.  It is tempting to generalize this routine so that
 * all disk drivers could share it.  However by the time all of the 
 * necessary parameters are setup and passed the savings vanish.  Also,
 * each driver has a different method of calculating the number of blocks
 * to use if one large partition must cover the disk.
 *
 * This routine used to always return success and callers carefully checked
 * the return status.  Silly.  This routine will fake a label (a single
 * partition spanning the drive) if necessary but will never return an error.
 *
 * It is the caller's responsibility to check the validity of partition 
 * numbers, etc.
*/

void
xpgetinfo(xd, dev)
	register struct xp_drive *xd;
	dev_t	dev;
	{
	struct	disklabel locallabel;
	char	*msg;
	register struct disklabel *lp = &locallabel;

	xpdfltlbl(xd, lp);
	msg = readdisklabel((dev & ~7) | 0, xpstrategy, lp);	/* 'a' */
	if	(msg != 0)
		{
		log(LOG_NOTICE, "xp%da using labelonly geometry: %s\n",
			XPUNIT(dev), msg);
		xpdfltlbl(xd, lp);
		}
	mapseg5(xd->xp_label, LABELDESC);
	bcopy(lp, (struct disklabel *)SEG5, sizeof (struct disklabel));
	normalseg5();
	bcopy(lp->d_partitions, xd->xp_parts, sizeof (lp->d_partitions));
	xd->xp_nsect = lp->d_nsectors;
	xd->xp_ntrack = lp->d_ntracks;
	xd->xp_nspc = lp->d_secpercyl;
	xd->xp_ncyl = lp->d_ncylinders;
	xd->xp_dd0 = lp->d_drivedata[0];
	return;
	}

xpstrategy(bp)
register struct buf *bp;
	{
	register struct xp_drive *xd;
	struct partition *pi;
	int	unit;
	struct buf *dp;
	register int	s;

	unit = XPUNIT(bp->b_dev);
	xd = &xp_drive[unit];

	if	(unit >= NXPD || !xd->xp_ctlr || !(xd->xp_flags & DKF_ALIVE))
		{
		bp->b_error = ENXIO;
		goto bad;
		}
	s = partition_check(bp, &xd->xp_dk);
	if	(s < 0)
		goto bad;
	if	(s == 0)
		goto done;
	if	(xd->xp_ctlr->xp_rh70 == 0)
		mapalloc(bp);
	pi = &xd->xp_parts[dkpart(bp->b_dev)];
	bp->b_cylin = (bp->b_blkno + pi->p_offset) / xd->xp_nspc;
	dp = &xputab[unit];
	s = splbio();
	disksort(dp, bp);
	if	(dp->b_active == 0)
		{
		xpustart(unit);
		if	(xd->xp_ctlr->xp_active == 0)
			xpstart(xd->xp_ctlr);
		}
	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
	iodone(bp);
	return;
	}

/*
 * Unit start routine.  Seek the drive to where the data are and then generate
 * another interrupt to actually start the transfer.  If there is only one
 * drive or we are very close to the data, don't bother with the search.  If
 * called after searching once, don't bother to look where we are, just queue
 * for transfer (to avoid positioning forever without transferring).
 */
xpustart(unit)
	int unit;
{
	register struct xp_drive *xd;
	register struct hpdevice *xpaddr;
	register struct buf *dp;
	struct buf *bp, *bbp;
	daddr_t bn;
	int	sn, cn, csn;

	xd = &xp_drive[unit];
	xpaddr = xd->xp_ctlr->xp_addr;
	xpaddr->hpcs2.w = xd->xp_unit;
	xpaddr->hpcs1.c[0] = HP_IE;
	xpaddr->hpas = 1 << xd->xp_unit;
#ifdef UCB_METER
	if (xp_dkn >= 0) {
		dk_busy &= ~(1 << (xp_dkn + unit));
	}
#endif
	dp = &xputab[unit];
	if ((bp=dp->b_actf) == NULL)
		return;
	/*
	 * If we have already positioned this drive,
	 * then just put it on the ready queue.
	 */
	if (dp->b_active)
		goto done;
	dp->b_active++;
	/*
	 * If drive has just come up, set up the pack.
	 */
	if (((xpaddr->hpds & HPDS_VV) == 0) || !(xd->xp_flags & DKF_ONLINE)) {
		xpaddr->hpcs1.c[0] = HP_IE | HP_PRESET | HP_GO;
		xpaddr->hpof = HPOF_FMT22;
		xd->xp_flags |= DKF_ONLINE;
#ifdef	XPDEBUG
		log(LOG_NOTICE, "xp%d preset done\n", unit);
#endif

/*
 * XXX - The 'h' partition is used below to access the bad block area.  This
 * XXX - will almost certainly be wrong if the user has defined another 
 * XXX - partition to span the entire drive including the bad block area.  It
 * XXX - is not known what to do about this.
*/
#ifdef BADSECT
		bbp = &bxpbuf[unit];
		bbp->b_flags = B_READ | B_BUSY | B_PHYS;
		bbp->b_dev = bp->b_dev | 7;	/* "h" partition whole disk */
		bbp->b_bcount = sizeof(struct dkbad);
		bbp->b_un.b_addr = (caddr_t)&xpbad[unit];
		bbp->b_blkno = (daddr_t)xd->xp_ncyl * xd->xp_nspc - xd->xp_nsect;
		bbp->b_cylin = xd->xp_ncyl - 1;
		if (xd->xp_ctlr->xp_rh70 == 0)
			mapalloc(bbp);
		dp->b_actf = bbp;
		bbp->av_forw = bp;
		bp = bbp;
#endif BADSECT
	}

#if NXPD > 1
	/*
	 * If drive is offline, forget about positioning.
	 */
	if	(xpaddr->hpds & (HPDS_DREADY) != (HPDS_DREADY))
		{
		xd->xp_flags &= ~DKF_ONLINE;
		goto done;
		}
	/*
	 * Figure out where this transfer is going to
	 * and see if we are close enough to justify not searching.
	 */
	bn = bp->b_blkno;
	cn = bp->b_cylin;
	sn = bn % xd->xp_nspc;
	sn += xd->xp_nsect - XP_SDIST;
	sn %= xd->xp_nsect;

	if	((!(xd->xp_dd0 & XP_CC) && (xd->xp_cc != cn))
		 || xpaddr->hpcc != cn)
		goto search;
	if	(xd->xp_dd0 & XP_NOSEARCH)
		goto done;
	csn = (xpaddr->hpla >> 6) - sn + XP_SDIST - 1;
	if (csn < 0)
		csn += xd->xp_nsect;
	if (csn > xd->xp_nsect - XP_RDIST)
		goto done;
search:
	xpaddr->hpdc = cn;
	xpaddr->hpda = sn;
	xpaddr->hpcs1.c[0] = (xd->xp_dd0 & XP_NOSEARCH) ?
		(HP_IE | HP_SEEK | HP_GO) : (HP_IE | HP_SEARCH | HP_GO);
	xd->xp_cc = cn;
#ifdef UCB_METER
	/*
	 * Mark unit busy for iostat.
	 */
	if (xp_dkn >= 0) {
		int dkn = xp_dkn + unit;

		dk_busy |= 1<<dkn;
		dk_seek[dkn]++;
	}
#endif
	return;
#endif NXPD > 1
done:
	/*
	 * Device is ready to go.
	 * Put it on the ready queue for the controller.
	 */
	dp->b_forw = NULL;
	if (xd->xp_ctlr->xp_actf == NULL)
		xd->xp_ctlr->xp_actf = dp;
	else
		xd->xp_ctlr->xp_actl->b_forw = dp;
	xd->xp_ctlr->xp_actl = dp;
}

/*
 * Start up a transfer on a controller.
 */
xpstart(xc)
register struct xp_controller *xc;
{
	register struct hpdevice *xpaddr;
	register struct buf *bp;
	struct xp_drive *xd;
	struct buf *dp;
	daddr_t bn;
	int	unit, part, sn, tn, cn;

	xpaddr = xc->xp_addr;
loop:
	/*
	 * Pull a request off the controller queue.
	 */
	if ((dp = xc->xp_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
/*
 * No more requests for this drive, remove from controller queue and
 * look at next drive.  We know we're at the head of the controller queue.
 * The drive may not need anything, in which case it might be shutting
 * down in xpclose() and a wakeup is done.
*/
		dp->b_active = 0;
		xc->xp_actf = dp->b_forw;
		unit = dp - xputab;
		xd = &xp_drive[unit];
		if	(xd->xp_open == 0)
			wakeup(dp);	/* finish close protocol */
		goto loop;
	}
	/*
	 * Mark controller busy and determine destination of this request.
	 */
	xc->xp_active++;
	part = dkpart(bp->b_dev);
	unit = XPUNIT(bp->b_dev);
	xd = &xp_drive[unit];
	bn = bp->b_blkno;
	cn = (xd->xp_parts[part].p_offset + bn) / xd->xp_nspc;
	sn = bn % xd->xp_nspc;
	tn = sn / xd->xp_nsect;
	sn = sn % xd->xp_nsect;
	/*
	 * Select drive.
	 */
	xpaddr->hpcs2.w = xd->xp_unit;
	/*
 	 * Check that it is ready and online.
	 */
	if ((xpaddr->hpds & HPDS_DREADY) != (HPDS_DREADY)) {
		xd->xp_flags &= ~DKF_ONLINE;
		xc->xp_active = 0;
		dp->b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		goto loop;
	}
	xd->xp_flags |= DKF_ONLINE;

	if (dp->b_errcnt >= 16 && (bp->b_flags & B_READ)) {
		xpaddr->hpof = xp_offset[dp->b_errcnt & 017] | HPOF_FMT22;
		xpaddr->hpcs1.w = HP_OFFSET | HP_GO;
		while ((xpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY);
	}
	xpaddr->hpdc = cn;
	xpaddr->hpda = (tn << 8) + sn;
	xpaddr->hpba = bp->b_un.b_addr;
	if (xc->xp_rh70)
		xpaddr->hpbae = bp->b_xmem;
	xpaddr->hpwc = -(bp->b_bcount >> 1);
	/*
	 * Warning:  unit is being used as a temporary.
	 */
	unit = ((bp->b_xmem & 3) << 8) | HP_IE | HP_GO;
#ifdef XP_FORMAT
	if (minor(bp->b_dev) & 0200)
		unit |= bp->b_flags & B_READ ? HP_RHDR : HP_WHDR;
	else
		unit |= bp->b_flags & B_READ ? HP_RCOM : HP_WCOM;
#else
	if (bp->b_flags & B_READ)
		unit |= HP_RCOM;
	else
		unit |= HP_WCOM;
#endif
	xpaddr->hpcs1.w = unit;
#ifdef UCB_METER
	if (xp_dkn >= 0) {
		int dkn = xp_dkn + XPUNIT(bp->b_dev);

		dk_busy |= 1<<dkn;
		dk_xfer[dkn]++;
		dk_seek[dkn]++;
		dk_wds[dkn] += bp->b_bcount>>6;
	}
#endif
}

/*
 * Handle a disk interrupt.
 */
xpintr(dev)
int dev;
{
	register struct hpdevice *xpaddr;
	register struct buf *dp;
	struct xp_controller *xc;
	struct xp_drive *xd;
	struct buf *bp;
	register int unit;
	int	as;

	xc = &xp_controller[dev];
	xpaddr = xc->xp_addr;
	as = xpaddr->hpas & 0377;
	if (xc->xp_active) {
	/*
 	 * Get device and block structures.  Select the drive.
	 */
		dp = xc->xp_actf;
		bp = dp->b_actf;
#ifdef BADSECT
		if (bp->b_flags & B_BAD)
			if (xpecc(bp, CONT))
				return;
#endif
		unit = XPUNIT(bp->b_dev);
#ifdef UCB_METER
		if (xp_dkn >= 0) {
			dk_busy &= ~(1 << (xp_dkn + unit));
		}
#endif
		xd = &xp_drive[unit];
		xpaddr->hpcs2.c[0] = xd->xp_unit;
		/*
		 * Check for and process errors.
		 */
		if (xpaddr->hpcs1.w & HP_TRE) {
			while ((xpaddr->hpds & HPDS_DRY) == 0);
			if (xpaddr->hper1 & HPER1_WLE) {
			/*
			 * Give up on write locked deviced immediately.
			 */
				log(LOG_NOTICE, "xp%d: write locked\n", unit);
				bp->b_flags |= B_ERROR;
#ifdef BADSECT
			}
			else if ((xpaddr->rmer2 & RMER2_BSE)
				|| (xpaddr->hper1 & HPER1_FER)) {
#ifdef XP_FORMAT
			/*
			 * Allow this error on format devices.
			 */
				if (minor(bp->b_dev) & 0200)
					goto errdone;
#endif
				if (xpecc(bp, BSE))
					return;
				else
					goto hard;
#endif BADSECT
			}
			else {
			/*
			 * After 28 retries (16 without offset and
			 * 12 with offset positioning), give up.
			 */
				if (++dp->b_errcnt > 28) {
hard:
					harderr(bp, "xp");
					log(LOG_NOTICE,"cs2=%b er1=%b er2=%b\n",
						xpaddr->hpcs2.w, HPCS2_BITS,
						xpaddr->hper1, HPER1_BITS,
						xpaddr->rmer2, RMER2_BITS);
					bp->b_flags |= B_ERROR;
				}
				else
					xc->xp_active = 0;
			}
			/*
			 * If soft ecc, correct it (continuing by returning if
			 * necessary).  Otherwise, fall through and retry the
			 * transfer.
			 */
			if((xpaddr->hper1 & (HPER1_DCK|HPER1_ECH)) == HPER1_DCK)
				if (xpecc(bp, ECC))
					return;
errdone:
			xpaddr->hpcs1.w = HP_TRE | HP_IE | HP_DCLR | HP_GO;
			if ((dp->b_errcnt & 07) == 4) {
				xpaddr->hpcs1.w = HP_RECAL | HP_IE | HP_GO;
				while ((xpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY);
			}
			xd->xp_cc = -1;
		}
		if (xc->xp_active) {
			if (dp->b_errcnt) {
				xpaddr->hpcs1.w = HP_RTC | HP_GO;
				while ((xpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY);
			}
			xc->xp_active = 0;
			xc->xp_actf = dp->b_forw;
			dp->b_active = 0;
			dp->b_errcnt = 0;
			dp->b_actf = bp->b_actf;
			xd->xp_cc = bp->b_cylin;
			bp->b_resid = - (xpaddr->hpwc << 1);
			iodone(bp);
			xpaddr->hpcs1.w = HP_IE;
			if (dp->b_actf)
				xpustart(unit);
		}
		as &= ~(1 << xp_drive[unit].xp_unit);
	}
	else {
		if (as == 0)
			xpaddr->hpcs1.w = HP_IE;
		xpaddr->hpcs1.c[1] = HP_TRE >> 8;
	}
	for (unit = 0; unit < NXPD; unit++)
		if ((xp_drive[unit].xp_ctlr == xc) &&
			(as & (1 << xp_drive[unit].xp_unit)))
			xpustart(unit);
	xpstart(xc);
}

#define exadr(x,y)	(((long)(x) << 16) | (unsigned)(y))

/*
 * Correct an ECC error and restart the i/o to complete the transfer if
 * necessary.  This is quite complicated because the correction may be going
 * to an odd memory address base and the transfer may cross a sector boundary.
 */
xpecc(bp, flag)
register struct	buf *bp;
	int	flag;
{
	register struct xp_drive *xd;
	register struct hpdevice *xpaddr;
	register unsigned byte;
	ubadr_t bb, addr;
	long wrong;
	int	bit, wc;
	unsigned ndone, npx;
	int	ocmd;
	int	cn, tn, sn;
	daddr_t bn;
	struct ubmap *ubp;
	int	unit;

	/*
	 * ndone is #bytes including the error which is assumed to be in the
	 * last disk page transferred.
	 */
	unit = XPUNIT(bp->b_dev);
	xd = &xp_drive[unit];
	xpaddr = xd->xp_ctlr->xp_addr;
#ifdef BADSECT
	if (flag == CONT) {
		npx = bp->b_error;
		bp->b_error = 0;
		ndone = npx * NBPG;
		wc = ((int)(ndone - bp->b_bcount)) / (int)NBPW;
	}
	else {
#endif
		wc = xpaddr->hpwc;
		ndone = ((unsigned)wc * NBPW) + bp->b_bcount;
		npx = ndone / NBPG;
#ifdef BADSECT
	}
#endif
	ocmd = (xpaddr->hpcs1.w & ~HP_RDY) | HP_IE | HP_GO;
	bb = exadr(bp->b_xmem, bp->b_un.b_addr);
	bn = bp->b_blkno;
	cn = bp->b_cylin - (bn / xd->xp_nspc);
	bn += npx;
	cn += bn / xd->xp_nspc;
	sn = bn % xd->xp_nspc;
	tn = sn;
	tn /= xd->xp_nsect;
	sn %= xd->xp_nsect;
	switch (flag) {
		case ECC:
			log(LOG_NOTICE, "xp%d%c: soft ecc sn%D\n",
				unit, 'a' + dkpart(bp->b_dev),
				bp->b_blkno + (npx - 1));
			wrong = xpaddr->hpec2;
			if (wrong == 0) {
				xpaddr->hpof = HPOF_FMT22;
				xpaddr->hpcs1.w |= HP_IE;
				return (0);
			}
			/*
			 * Compute the byte/bit position of the err
			 * within the last disk page transferred.
			 * Hpec1 is origin-1.
			 */
			byte = xpaddr->hpec1 - 1;
			bit = byte & 07;
			byte >>= 3;
			byte += ndone - NBPG;
			wrong <<= bit;
			/*
			 * Correct until mask is zero or until end of
			 * transfer, whichever comes first.
			 */
			while (byte < bp->b_bcount && wrong != 0) {
				addr = bb + byte;
				if (bp->b_flags & (B_MAP|B_UBAREMAP)) {
					/*
					 * Simulate UNIBUS map if UNIBUS
					 * transfer.
					 */
					ubp = UBMAP + ((addr >> 13) & 037);
					addr = exadr(ubp->ub_hi, ubp->ub_lo) + (addr & 017777);
				}
				putmemc(addr, getmemc(addr) ^ (int) wrong);
				byte++;
				wrong >>= 8;
			}
			break;
#ifdef BADSECT
		case BSE:
			if ((bn = isbad(&xpbad[unit], cn, tn, sn)) < 0)
				return(0);
			bp->b_flags |= B_BAD;
			bp->b_error = npx + 1;
			bn = (daddr_t)xd->xp_ncyl * xd->xp_nspc - xd->xp_nsect - 1 - bn;
			cn = bn/xd->xp_nspc;
			sn = bn%xd->xp_nspc;
			tn = sn;
			tn /= xd->xp_nsect;
			sn %= xd->xp_nsect;
			log(LOG_NOTICE, "revector to cn %d tn %d sn %d\n",
				cn, tn, sn);
			wc = -(512 / (int)NBPW);
			break;
		case CONT:
			bp->b_flags &= ~B_BAD;
			log(LOG_NOTICE, "xpecc CONT: bn %D cn %d tn %d sn %d\n",
				bn, cn, tn, sn);
			break;
#endif BADSECT
	}
	xd->xp_ctlr->xp_active++;
	if (wc == 0)
		return (0);

	/*
	 * Have to continue the transfer.  Clear the drive and compute the
	 * position where the transfer is to continue.  We have completed
	 * npx sectors of the transfer already.
	 */
	xpaddr->hpcs2.w = xd->xp_unit;
	xpaddr->hpcs1.w = HP_TRE | HP_DCLR | HP_GO;
	addr = bb + ndone;
	xpaddr->hpdc = cn;
	xpaddr->hpda = (tn << 8) + sn;
	xpaddr->hpwc = wc;
	xpaddr->hpba = (caddr_t)addr;
	if (xd->xp_ctlr->xp_rh70)
		xpaddr->hpbae = (int)(addr >> 16);
	xpaddr->hpcs1.w = ocmd;
	return (1);
}

xpioctl(dev, cmd, data, flag)
	dev_t	dev;
	int	cmd;
	caddr_t	data;
	int	flag;
	{
	register int error;
	struct	dkdevice *disk = &xp_drive[XPUNIT(dev)].xp_dk;

	error = ioctldisklabel(dev, cmd, data, flag, disk, xpstrategy);
	return(error);
	}

#ifdef XP_DUMP
/*
 * Dump routine.  Dumps from dumplo to end of memory/end of disk section for
 * minor(dev).
 */
#define DBSIZE	16			/* number of blocks to write */

xpdump(dev)
	dev_t dev;
	{
	struct xp_drive *xd;
	register struct hpdevice *xpaddr;
	struct	partition *pi;
	daddr_t bn, dumpsize;
	long	paddr;
	int	sn, count, memblks, unit;
	register struct	ubmap *ubp;

	unit = XPUNIT(dev);
	xd = &xp_drive[unit];

	if	(unit > NXPD || xd->xp_ctlr == 0)
		return(EINVAL);
	if	(!(xd->xp_flags & DKF_ALIVE))
		return(ENXIO);

	pi = &xd->xp_parts[dkpart(dev)];
	if	(pi->p_fstype != FS_SWAP)
		return(EFTYPE);

	xpaddr = xd->xp_ctlr->xp_addr;

	dumpsize = xpsize(dev) - dumplo;
	memblks = ctod(physmem);

	if	(dumplo < 0 || dumpsize <= 0)
		return(EINVAL);
	if	(memblks > dumpsize)
		memblks = dumpsize;
	bn = dumplo + pi->p_offset;

	xpaddr->hpcs2.w = xd->xp_unit;
	if	((xpaddr->hpds & HPDS_VV) == 0)
		{
		xpaddr->hpcs1.w = HP_DCLR | HP_GO;
		xpaddr->hpcs1.w = HP_PRESET | HP_GO;
		xpaddr->hpof = HPOF_FMT22;
		}
	if	((xpaddr->hpds & HPDS_DREADY) != (HPDS_DREADY))
		return(EFAULT);
	ubp = &UBMAP[0];
	for	(paddr = 0L; memblks > 0; )
		{
		count = MIN(memblks, DBSIZE);
		xpaddr->hpdc = bn / xd->xp_nspc;
		sn = bn % xd->xp_nspc;
		xpaddr->hpda = ((sn / xd->xp_nsect) << 8) | (sn % xd->xp_nsect);
		xpaddr->hpwc = -(count << (PGSHIFT - 1));
		xpaddr->hper1 = 0;
		xpaddr->hper3 = 0;
		if	(ubmap && (xd->xp_ctlr->xp_rh70 == 0))
			{
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			xpaddr->hpba = 0;
			xpaddr->hpcs1.w = HP_WCOM | HP_GO;
			}
		else
			{
			/*
			 * Non-UNIBUS map, or 11/70 RH70 (MASSBUS)
			 */
			xpaddr->hpba = (caddr_t)loint(paddr);
			if	(xd->xp_ctlr->xp_rh70)
				xpaddr->hpbae = hiint(paddr);
			xpaddr->hpcs1.w = HP_WCOM | HP_GO | ((paddr >> 8) & (03 << 8));
			}
		/* Emulex controller emulating two RM03's needs a delay */
		delay(50000L);
		while	(xpaddr->hpcs1.w & HP_GO)
			continue;
		if	(xpaddr->hpcs1.w & HP_TRE)
			return(EIO);
		paddr += (count << PGSHIFT);
		bn += count;
		memblks -= count;
		}
	return(0);
}
#endif XP_DUMP

/*
 * Return the number of blocks in a partition.  Call xpopen() to read the
 * label if necessary.  If an open is necessary then a matching close
 * will be done.
*/
daddr_t
xpsize(dev)
	register dev_t dev;
	{
	register struct xp_drive *xd;
	daddr_t	psize;
	int	didopen = 0;

	xd = &xp_drive[XPUNIT(dev)];
/*
 * This should never happen but if we get called early in the kernel's
 * life (before opening the swap or root devices) then we have to do
 * the open here.
*/
	if	(xd->xp_open == 0)
		{
		if	(xpopen(dev, FREAD|FWRITE, S_IFBLK))
			return(-1);
		didopen = 1;
		}
	psize = xd->xp_parts[dkpart(dev)].p_size;
	if	(didopen)
		xpclose(dev, FREAD|FWRITE, S_IFBLK);
	return(psize);
	}
#endif /* NXPD */

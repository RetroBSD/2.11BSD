/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)hk.c	2.3 (2.11BSD GTE) 1998/4/3
 */

/*
 * RK611/RK0[67] disk driver
 *
 * Heavily modified for disklabel support.  Still only supports 1 controller
 * (but who'd have more than one of these on a system anyhow?) - 1997/11/11 sms
 *
 * This driver mimics the 4.1bsd rk driver.
 * It does overlapped seeks, ECC, and bad block handling.
 * 	salkind@nyu
 *
 * dkunit() takes a 'dev_t' now instead of 'buf *'.  1995/04/13 - sms
 *
 * Removed ifdefs on both Q22 and UNIBUS_MAP, substituting a runtime
 * test for presence of a Unibus Map.  Reworked the partition logic,
 * the 'e' partiton no longer overlaps the 'a'+'b' partitions - a separate
 * 'b' partition is now present.  Old root filesystems can still be used
 * because the size is the same, but user data will have to be saved and
 * then reloaded.  12/28/92 -- sms@wlv.iipo.gtegsc.com
 *
 * Modified to correctly handle 22 bit addressing available on DILOG
 * DQ615 controller. 05/31/90 -- tymann@oswego.edu
 *
 */

#include "hk.h"
#if	NHK > 0
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "machine/seg.h"
#include "conf.h"
#include "user.h"
#include "map.h"
#include "uba.h"
#include "hkreg.h"
#include "dkbad.h"
#include "dk.h"
#include "stat.h"
#include "file.h"
#include "disklabel.h"
#include "disk.h"
#include "syslog.h"

#define	NHK7CYL	815
#define	NHK6CYL	411
#define	HK_NSECT	22
#define	HK_NTRAC	3
#define	HK_NSPC		(HK_NTRAC*HK_NSECT)

struct	hkdevice *HKADDR;

	daddr_t	hksize();
	void	hkdfltlbl();
	int	hkstrategy();

/* Can be u_char because all are less than 0377 */
u_char	hk_offset[] =
	{
	HKAS_P400,	HKAS_M400,	HKAS_P400,	HKAS_M400,
	HKAS_P800,	HKAS_M800,	HKAS_P800,	HKAS_M800,
	HKAS_P1200,	HKAS_M1200,	HKAS_P1200,	HKAS_M1200,
	0,		0,		0,		0,
	};

	int	hk_type[NHK];
	int	hk_cyl[NHK];

struct hk_softc
	{
	int	sc_softas;
	int	sc_recal;
	} hk;

	struct	buf	hktab;
	struct	buf	hkutab[NHK];
	struct	dkdevice hk_dk[NHK];

#ifdef BADSECT
	struct	dkbad	hkbad[NHK];
	struct	buf	bhkbuf[NHK];
#endif

#ifdef UCB_METER
	static	int		hk_dkn = -1;	/* number for iostat */
#endif

#define	hkwait(hkaddr)		while ((hkaddr->hkcs1 & HK_CRDY) == 0)
#define	hkncyl(unit)		(hk_type[unit] ? NHK7CYL : NHK6CYL)

void
hkroot()
	{
	hkattach((struct hkdevice *)0177440, 0);
	}

hkattach(addr, unit)
struct hkdevice *addr;
	{
#ifdef UCB_METER
	if	(hk_dkn < 0)
		{
		dk_alloc(&hk_dkn, NHK+1, "hk", 60L * (long)HK_NSECT * 256L);
		if	(hk_dkn >= 0)
			dk_wps[hk_dkn+NHK] = 0L;
		}
#endif
	if (unit != 0)
		return(0);
	HKADDR = addr;
	return(1);
	}

hkopen(dev, flag, mode)
	dev_t	dev;
	int	flag;
	int	mode;
	{
	register int unit = dkunit(dev);
	register struct hkdevice *hkaddr = HKADDR;
	register struct	dkdevice *disk;
	int	i, mask;

	if	(unit >= NHK || !HKADDR)
		return(ENXIO);
	disk = &hk_dk[unit];

	if	((disk->dk_flags & DKF_ALIVE) == 0)
		{
		hk_type[unit] = 0;
		hkaddr->hkcs1 = HK_CCLR;
		hkaddr->hkcs2 = unit;
		hkaddr->hkcs1 = HK_DCLR | HK_GO;
		hkwait(hkaddr);
		if	(hkaddr->hkcs2&HKCS2_NED || !(hkaddr->hkds&HKDS_SVAL))
			{
			hkaddr->hkcs1 = HK_CCLR;
			hkwait(hkaddr);
			return(ENXIO);
			}
		disk->dk_flags |= DKF_ALIVE;
		if	((hkaddr->hkcs1&HK_CERR) && (hkaddr->hker&HKER_DTYE))
			{
			hk_type[unit] = HK_CDT;
			hkaddr->hkcs1 = HK_CCLR;
			hkwait(hkaddr);
			}
		}
/*
 * The drive has responded to a probe (is alive).  Now we read the
 * label.  Allocate an external label structure if one has not already
 * been assigned to this drive.  First wait for any pending opens/closes
 * to complete.
*/
	while	(disk->dk_flags & (DKF_OPENING | DKF_CLOSING))
		sleep(disk, PRIBIO);

/*
 * Next if an external label buffer has not already been allocated do so now.
 * This "can not fail" because if the initial pool of label buffers has
 * been exhausted the allocation takes place from main memory.  The return
 * value is the 'click' address to be used when mapping in the label.
*/

	if	(disk->dk_label == 0)
		disk->dk_label = disklabelalloc();

/*
 * On first open get label and partition info.  We may block reading the
 * label so be careful to stop any other opens.
*/
	if	(disk->dk_openmask == 0)
		{
		disk->dk_flags |= DKF_OPENING;
		hkgetinfo(disk, dev);
		disk->dk_flags &= ~DKF_OPENING;
		wakeup(disk);
		hk_cyl[unit] = -1;
		}
/*
 * Need to make sure the partition is not out of bounds.  This requires
 * mapping in the external label.  This only happens when a partition
 * is opened (at mount time) and isn't an efficiency problem.
*/
	mapseg5(disk->dk_label, LABELDESC);
	i = ((struct disklabel *)SEG5)->d_npartitions;
	normalseg5();
	if	(dkpart(dev) >= i)
		return(ENXIO);
	mask = 1 << dkpart(dev);
	dkoverlapchk(disk->dk_openmask, dev, disk->dk_label, "hk");
	if	(mode == S_IFCHR)
		disk->dk_copenmask |= mask;
	else if (mode == S_IFBLK)
		disk->dk_bopenmask |= mask;
	else
		return(EINVAL);
	disk->dk_openmask |= mask;
	return(0);
	}

/*
 * Disk drivers now have to have close entry points in order to keep
 * track of what partitions are still active on a drive.
*/
hkclose(dev, flag, mode)
register dev_t  dev;
	int     flag, mode;
	{
	int     s, drive = dkunit(dev);
	register int    mask;
	register struct dkdevice *disk;

	disk = &hk_dk[drive];
	mask = 1 << dkpart(dev);
	if	(mode == S_IFCHR)
		disk->dk_copenmask &= ~mask;
	else if (mode == S_IFBLK)
		disk->dk_bopenmask &= ~mask;
	else
		return(EINVAL);
	disk->dk_openmask = disk->dk_bopenmask | disk->dk_copenmask;
	if	(disk->dk_openmask == 0)
		{
		disk->dk_flags |= DKF_CLOSING;
		s = splbio();
		while   (hkutab[drive].b_actf)
			{
			disk->dk_flags |= DKF_WANTED;
			sleep(&hkutab[drive], PRIBIO);
			}
		splx(s);
/*
 * On last close of a drive we declare it not alive and offline to force a
 * probe on the next open in order to handle diskpack changes.
*/
		disk->dk_flags &= 
			~(DKF_CLOSING | DKF_WANTED | DKF_ALIVE | DKF_ONLINE);
		wakeup(disk);
		}
	return(0);
	}

/*
 * This code moved here from hkgetinfo() because it is fairly large and used
 * twice - once to initialize for reading the label and a second time if
 * there is no valid label present on the drive and the default one must be
 * used to span the drive.
*/

void
hkdfltlbl(disk, lp, dev)
	struct dkdevice *disk;
	register struct disklabel *lp;
	dev_t	dev;
	{
	register struct partition *pi = &lp->d_partitions[0];

	bzero(lp, sizeof (*lp));
        lp->d_type = DTYPE_DEC;
	lp->d_secsize = 512;            /* XXX */
	lp->d_nsectors = HK_NSECT;
	lp->d_ntracks = HK_NTRAC;
	lp->d_secpercyl = HK_NSPC;
	lp->d_npartitions = 1;          /* 'a' */
	lp->d_ncylinders = hkncyl(dkunit(dev));
	pi->p_size = lp->d_ncylinders * lp->d_secpercyl;     /* entire volume */
	pi->p_fstype = FS_V71K;
	pi->p_frag = 1;
	pi->p_fsize = 1024;
/*
 * Put where hkstrategy() will look.
*/
	bcopy(pi, disk->dk_parts, sizeof (lp->d_partitions));
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
hkgetinfo(disk, dev)
register struct dkdevice *disk;
	dev_t   dev;
	{
	struct  disklabel locallabel;
	char    *msg;
	register struct disklabel *lp = &locallabel;
/*
 * NOTE: partition 0 ('a') is used to read the label.  Therefore 'a' must
 * start at the beginning of the disk!  If there is no label or the label
 * is corrupted then 'a' will span the entire disk
*/
	hkdfltlbl(disk, lp, dev);
	msg = readdisklabel((dev & ~7) | 0, hkstrategy, lp);    /* 'a' */
	if      (msg != 0)
		{
		log(LOG_NOTICE, "hk%da is entire disk: %s\n", dkunit(dev), msg);
		hkdfltlbl(disk, lp, dev);
		}
	mapseg5(disk->dk_label, LABELDESC)
	bcopy(lp, (struct disklabel *)SEG5, sizeof (struct disklabel));
	normalseg5();
	bcopy(lp->d_partitions, disk->dk_parts, sizeof (lp->d_partitions));
	return;
	}

hkstrategy(bp)
register struct buf *bp;
{
	register struct buf *dp;
	int s, drive;
	register struct dkdevice *disk;

	drive = dkunit(bp->b_dev);
	disk = &hk_dk[drive];

	if	(drive >= NHK || !HKADDR  || !(disk->dk_flags & DKF_ALIVE))
		{
		bp->b_error = ENXIO;
		goto bad;
		}
	s = partition_check(bp, disk);
	if	(s < 0)
		goto bad;
	if	(s == 0)
		goto done;

	bp->b_cylin = bp->b_blkno / HK_NSPC;
	mapalloc(bp);
	dp = &hkutab[drive];
	s = splbio();
	disksort(dp, bp);
	if	(dp->b_active == 0)
		{
		hkustart(drive);
		if	(hktab.b_active == 0)
			hkstart();
		}
	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
	iodone(bp);
	}

hkustart(unit)
	int unit;
{
	register struct hkdevice *hkaddr = HKADDR;
	register struct buf *bp, *dp;
	struct dkdevice *disk;
	int didie = 0;

#ifdef UCB_METER
	if (hk_dkn >= 0)
		dk_busy &= ~(1 << (hk_dkn + unit));
#endif
	if (hktab.b_active) {
		hk.sc_softas |= (1 << unit);
		return(0);
	}

	hkaddr->hkcs1 = HK_CCLR;
	hkaddr->hkcs2 = unit;
	hkaddr->hkcs1 = hk_type[unit] | HK_DCLR | HK_GO;
	hkwait(hkaddr);

	dp = &hkutab[unit];
	disk = &hk_dk[unit];
	if ((bp = dp->b_actf) == NULL)
		return(0);
	if (dp->b_active)
		goto done;
	dp->b_active = 1;
	if	(!(hkaddr->hkds & HKDS_VV) || !(disk->dk_flags & DKF_ONLINE))
		{
		/* SHOULD WARN SYSTEM THAT THIS HAPPENED */
#ifdef BADSECT
		struct buf *bbp = &bhkbuf[unit];
#endif

		hkaddr->hkcs1 = hk_type[unit]|HK_PACK|HK_GO;
		disk->dk_flags |= DKF_ONLINE;
/*
 * XXX - The 'c' partition is used below to access the bad block area.  This
 * XXX - is DIFFERENT than the XP driver (which should have used 'c' but could
 * XXX - not due to historical reasons).  The 'c' partition MUST span the entire
 * XXX - disk including the bad sector track.  The 'h' partition should be 
 * XXX - used for user data.
*/
#ifdef BADSECT
		bbp->b_flags = B_READ|B_BUSY|B_PHYS;
		bbp->b_dev = (bp->b_dev & ~7) | ('c' - 'a');
		bbp->b_bcount = sizeof(struct dkbad);
		bbp->b_un.b_addr = (caddr_t)&hkbad[unit];
		bbp->b_blkno = (long)hkncyl(unit)*HK_NSPC - HK_NSECT;
		bbp->b_cylin = hkncyl(unit) - 1;
		mapalloc(bbp);
		dp->b_actf = bbp;
		bbp->av_forw = bp;
		bp = bbp;
#endif
		hkwait(hkaddr);
		}
	if	((hkaddr->hkds & HKDS_DREADY) != HKDS_DREADY)
		{
		disk->dk_flags &= ~DKF_ONLINE;
		goto done;
		}
#ifdef NHK > 1
	if (bp->b_cylin == hk_cyl[unit])
		goto done;
	hkaddr->hkcyl = bp->b_cylin;
	hk_cyl[unit] = bp->b_cylin;
	hkaddr->hkcs1 = hk_type[unit] | HK_IE | HK_SEEK | HK_GO;
	didie = 1;
#ifdef UCB_METER
	if (hk_dkn >= 0) {
		int dkn = hk_dkn + unit;

		dk_busy |= 1<<dkn;
		dk_seek[dkn]++;
	}
#endif
	return (didie);
#endif NHK > 1

done:
	if (dp->b_active != 2) {
		dp->b_forw = NULL;
		if (hktab.b_actf == NULL)
			hktab.b_actf = dp;
		else
			hktab.b_actl->b_forw = dp;
		hktab.b_actl = dp;
		dp->b_active = 2;
	}
	return (didie);
}

hkstart()
{
	register struct buf *bp, *dp;
	register struct hkdevice *hkaddr = HKADDR;
	register struct dkdevice *disk;
	daddr_t bn;
	int sn, tn, cmd, unit;

loop:
	if ((dp = hktab.b_actf) == NULL)
		return(0);
	if ((bp = dp->b_actf) == NULL) {
/*
 * No more requests for this drive, remove from controller queue and
 * look at next drive.  We know we're at the head of the controller queue.
 * The drive may not need anything, in which case it might be shutting
 * down in hkclose() and a wakeup is done.
*/
		hktab.b_actf = dp->b_forw;
		unit = dp - hkutab;
		disk = &hk_dk[unit];
		if	(disk->dk_flags & DKF_WANTED)
			{
			disk->dk_flags &= ~DKF_WANTED;
			wakeup(dp);	/* finish the close protocol */
			}
		goto loop;
	}
	hktab.b_active++;
	unit = dkunit(bp->b_dev);
	disk = &hk_dk[unit];
	bn = bp->b_blkno;

	sn = bn % HK_NSPC;
	tn = sn / HK_NSECT;
	sn %= HK_NSECT;
retry:
	hkaddr->hkcs1 = HK_CCLR;
	hkaddr->hkcs2 = unit;
	hkaddr->hkcs1 = hk_type[unit] | HK_DCLR | HK_GO;
	hkwait(hkaddr);

	if ((hkaddr->hkds & HKDS_SVAL) == 0)
		goto nosval;
	if (hkaddr->hkds & HKDS_PIP)
		goto retry;
	if ((hkaddr->hkds&HKDS_DREADY) != HKDS_DREADY) {
		disk->dk_flags &= ~DKF_ONLINE;
		log(LOG_WARNING, "hk%d: !ready\n", unit);
		if ((hkaddr->hkds&HKDS_DREADY) != HKDS_DREADY) {
			hkaddr->hkcs1 = hk_type[unit] | HK_DCLR | HK_GO;
			hkwait(hkaddr);
			hkaddr->hkcs1 = HK_CCLR;
			hkwait(hkaddr);
			hktab.b_active = 0;
			hktab.b_errcnt = 0;
			dp->b_actf = bp->av_forw;
			dp->b_active = 0;
			bp->b_flags |= B_ERROR;
			iodone(bp);
			goto loop;
		}
	}
	disk->dk_flags |= DKF_ONLINE;
nosval:
	hkaddr->hkcyl = bp->b_cylin;
	hk_cyl[unit] = bp->b_cylin;
	hkaddr->hkda = (tn << 8) + sn;
	hkaddr->hkwc = -(bp->b_bcount >> 1);
	hkaddr->hkba = bp->b_un.b_addr;
	if	(!ubmap)
		hkaddr->hkxmem=bp->b_xmem;

	cmd = hk_type[unit] | ((bp->b_xmem & 3) << 8) | HK_IE | HK_GO;
	if (bp->b_flags & B_READ)
		cmd |= HK_READ;
	else
		cmd |= HK_WRITE;
	hkaddr->hkcs1 = cmd;
#ifdef UCB_METER
	if (hk_dkn >= 0) {
		int dkn = hk_dkn + NHK;

		dk_busy |= 1<<dkn;
		dk_xfer[dkn]++;
		dk_wds[dkn] += bp->b_bcount>>6;
	}
#endif
	return(1);
}

hkintr()
{
	register struct hkdevice *hkaddr = HKADDR;
	register struct buf *bp, *dp;
	int unit;
	int as = (hkaddr->hkatt >> 8) | hk.sc_softas;
	int needie = 1;

	hk.sc_softas = 0;
	if (hktab.b_active) {
		dp = hktab.b_actf;
		bp = dp->b_actf;
		unit = dkunit(bp->b_dev);
#ifdef UCB_METER
		if (hk_dkn >= 0)
			dk_busy &= ~(1 << (hk_dkn + NHK));
#endif
#ifdef BADSECT
		if (bp->b_flags&B_BAD)
			if (hkecc(bp, CONT))
				return;
#endif
		if (hkaddr->hkcs1 & HK_CERR) {
			int recal;
			u_short ds = hkaddr->hkds;
			u_short cs2 = hkaddr->hkcs2;
			u_short er = hkaddr->hker;

			if (er & HKER_WLE) {
				log(LOG_WARNING, "hk%d: wrtlck\n", unit);
				bp->b_flags |= B_ERROR;
			} else if (++hktab.b_errcnt > 28 ||
			    ds&HKDS_HARD || er&HKER_HARD || cs2&HKCS2_HARD) {
hard:
				harderr(bp, "hk");
				log(LOG_WARNING, "cs2=%b ds=%b er=%b\n",
				    cs2, HKCS2_BITS, ds, 
				    HKDS_BITS, er, HKER_BITS);
				bp->b_flags |= B_ERROR;
				hk.sc_recal = 0;
			} else if (er & HKER_BSE) {
#ifdef BADSECT
				if (hkecc(bp, BSE))
					return;
				else
#endif
					goto hard;
			} else
				hktab.b_active = 0;
			if (cs2&HKCS2_MDS) {
				hkaddr->hkcs2 = HKCS2_SCLR;
				goto retry;
			}
			recal = 0;
			if (ds&HKDS_DROT || er&(HKER_OPI|HKER_SKI|HKER_UNS) ||
			    (hktab.b_errcnt&07) == 4)
				recal = 1;
			if ((er & (HKER_DCK|HKER_ECH)) == HKER_DCK)
				if (hkecc(bp, ECC))
					return;
			hkaddr->hkcs1 = HK_CCLR;
			hkaddr->hkcs2 = unit;
			hkaddr->hkcs1 = hk_type[unit]|HK_DCLR|HK_GO;
			hkwait(hkaddr);
			if (recal && hktab.b_active == 0) {
				hkaddr->hkcs1 = hk_type[unit]|HK_IE|HK_RECAL|HK_GO;
				hk_cyl[unit] = -1;
				hk.sc_recal = 0;
				goto nextrecal;
			}
		}
retry:
		switch (hk.sc_recal) {

		case 1:
			hkaddr->hkcyl = bp->b_cylin;
			hk_cyl[unit] = bp->b_cylin;
			hkaddr->hkcs1 = hk_type[unit]|HK_IE|HK_SEEK|HK_GO;
			goto nextrecal;
		case 2:
			if (hktab.b_errcnt < 16 ||
			    (bp->b_flags&B_READ) == 0)
				goto donerecal;
			hkaddr->hkatt = hk_offset[hktab.b_errcnt & 017];
			hkaddr->hkcs1 = hk_type[unit]|HK_IE|HK_OFFSET|HK_GO;
			/* fall into ... */
		nextrecal:
			hk.sc_recal++;
			hkwait(hkaddr);
			hktab.b_active = 1;
			return;
		donerecal:
		case 3:
			hk.sc_recal = 0;
			hktab.b_active = 0;
			break;
		}
		if (hktab.b_active) {
			hktab.b_active = 0;
			hktab.b_errcnt = 0;
			hktab.b_actf = dp->b_forw;
			dp->b_active = 0;
			dp->b_errcnt = 0;
			dp->b_actf = bp->av_forw;
			bp->b_resid = -(hkaddr->hkwc << 1);
			iodone(bp);
			if (dp->b_actf)
				if (hkustart(unit))
					needie = 0;
		}
		as &= ~(1<<unit);
	}
	for (unit = 0; as; as >>= 1, unit++)
		if (as & 1) {
			if (unit < NHK && (hk_dk[unit].dk_flags & DKF_ALIVE)) {
				if (hkustart(unit))
					needie = 0;
			} else {
				hkaddr->hkcs1 = HK_CCLR;
				hkaddr->hkcs2 = unit;
				hkaddr->hkcs1 = HK_DCLR | HK_GO;
				hkwait(hkaddr);
				hkaddr->hkcs1 = HK_CCLR;
			}
		}
	if (hktab.b_actf && hktab.b_active == 0)
		if (hkstart())
			needie = 0;
	if (needie)
		hkaddr->hkcs1 = HK_IE;
}

#ifdef HK_DUMP
/*
 *  Dump routine for RK06/07
 *  Dumps from dumplo to end of memory/end of disk section for minor(dev).
 *  It uses the UNIBUS map to dump all of memory if there is a UNIBUS map.
 */
#define	DBSIZE	(UBPAGE/NBPG)		/* unit of transfer, one UBPAGE */

hkdump(dev)
	dev_t dev;
	{
	register struct hkdevice *hkaddr = HKADDR;
	daddr_t	bn, dumpsize;
	long paddr;
	register struct ubmap *ubp;
	int	count, memblks;
	register struct partition *pi;
	struct dkdevice *disk;
	int com, cn, tn, sn, unit;

	unit = dkunit(dev);
	if	(unit >= NHK)
		return(EINVAL);

	disk = &hk_dk[unit];
	if	((disk->dk_flags & DKF_ALIVE) == 0)
		return(ENXIO);
	pi = &disk->dk_parts[dkpart(dev)];
	if	(pi->p_fstype != FS_SWAP)
		return(EFTYPE);

	dumpsize = hksize(dev) - dumplo;
	memblks = ctod(physmem);
	if	(dumplo < 0 || dumpsize <= 0)
		return(EINVAL);
	bn = dumplo + pi->p_offset;

	hkaddr->hkcs1 = HK_CCLR;
	hkwait(hkaddr);
	hkaddr->hkcs2 = unit;
	hkaddr->hkcs1 = hk_type[unit] | HK_DCLR | HK_GO;
	hkwait(hkaddr);
	if	((hkaddr->hkds & HKDS_VV) == 0)
		{
		hkaddr->hkcs1 = hk_type[unit]|HK_IE|HK_PACK|HK_GO;
		hkwait(hkaddr);
		}
	ubp = &UBMAP[0];
	for	(paddr = 0L; memblks > 0; )
		{
		count = MIN(memblks, DBSIZE);
		cn = bn/HK_NSPC;
		sn = bn%HK_NSPC;
		tn = sn/HK_NSECT;
		sn = sn%HK_NSECT;
		hkaddr->hkcyl = cn;
		hkaddr->hkda = (tn << 8) | sn;
		hkaddr->hkwc = -(count << (PGSHIFT-1));
		com = hk_type[unit]|HK_GO|HK_WRITE;
		if	(ubmap)
			{
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			hkaddr->hkba = 0;
			}
		else
			{
			/* non UNIBUS map */
			hkaddr->hkba = loint(paddr);
			hkaddr->hkxmem = hiint(paddr);
			com |= ((paddr >> 8) & (03 << 8));
			}
		hkaddr->hkcs2 = unit;
		hkaddr->hkcs1 = com;
		hkwait(hkaddr);
		if	(hkaddr->hkcs1 & HK_CERR)
			{
			if (hkaddr->hkcs2 & HKCS2_NEM)
				return(0);	/* made it to end of memory */
			return(EIO);
			}
		paddr += (count << PGSHIFT);
		bn += count;
		memblks -= count;
		}
	return(0);		/* filled disk minor dev */
}
#endif HK_DUMP

#define	exadr(x,y)	(((long)(x) << 16) | (unsigned)(y))

/*
 * Correct an ECC error and restart the i/o to complete
 * the transfer if necessary.  This is quite complicated because
 * the transfer may be going to an odd memory address base
 * and/or across a page boundary.
 */
hkecc(bp, flag)
register struct	buf *bp;
{
	register struct	hkdevice *hkaddr = HKADDR;
	ubadr_t	addr;
	int npx, wc;
	int cn, tn, sn;
	daddr_t	bn;
	unsigned ndone;
	int cmd;
	int unit;

#ifdef BADSECT
	if (flag == CONT) {
		npx = bp->b_error;
		ndone = npx * NBPG;
		wc = ((int)(ndone - bp->b_bcount)) / NBPW;
	} else
#endif
		{
		wc = hkaddr->hkwc;
		ndone = (wc * NBPW) + bp->b_bcount;
		npx = ndone / NBPG;
		}
	unit = dkunit(bp->b_dev);
	bn = bp->b_blkno;
	cn = bp->b_cylin - bn / HK_NSPC;
	bn += npx;
	cn += bn / HK_NSPC;
	sn = bn % HK_NSPC;
	tn = sn / HK_NSECT;
	sn %= HK_NSECT;
	hktab.b_active++;

	switch (flag) {
	case ECC:
		{
		register byte;
		int bit;
		long mask;
		ubadr_t bb;
		unsigned o;
		struct ubmap *ubp;

		log(LOG_WARNING, "hk%d%c:  soft ecc sn %D\n",
			unit, 'a' + (bp->b_dev & 07), bp->b_blkno + npx - 1);
		mask = hkaddr->hkecpt;
		byte = hkaddr->hkecps - 1;
		bit = byte & 07;
		byte >>= 3;
		mask <<= bit;
		o = (ndone - NBPG) + byte;
		bb = exadr(bp->b_xmem, bp->b_un.b_addr);
		bb += o;
		if (ubmap && (bp->b_flags & (B_MAP|B_UBAREMAP))) {
			ubp = UBMAP + ((bb >> 13) & 037);
			bb = exadr(ubp->ub_hi, ubp->ub_lo) + (bb & 017777);
		}
		/*
		 * Correct until mask is zero or until end of
		 * sector or transfer, whichever comes first.
		 */
		while (byte < NBPG && o < bp->b_bcount && mask != 0) {
			putmemc(bb, getmemc(bb) ^ (int)mask);
			byte++;
			o++;
			bb++;
			mask >>= 8;
		}
		if (wc == 0)
			return(0);
		break;
	}

#ifdef BADSECT
	case BSE:
		if ((bn = isbad(&hkbad[unit], cn, tn, sn)) < 0)
			return(0);
		bp->b_flags |= B_BAD;
		bp->b_error = npx + 1;
		bn = (long)hkncyl(unit)*HK_NSPC - HK_NSECT - 1 - bn;
		cn = bn/HK_NSPC;
		sn = bn%HK_NSPC;
		tn = sn/HK_NSECT;
		sn %= HK_NSECT;
		wc = -(NBPG / NBPW);
		break;

	case CONT:
		bp->b_flags &= ~B_BAD;
		if (wc == 0)
			return(0);
		break;
#endif BADSECT
	}
	/*
	 * Have to continue the transfer.  Clear the drive
	 * and compute the position where the transfer is to continue.
	 * We have completed npx sectors of the transfer already.
	 */
	hkaddr->hkcs1 = HK_CCLR;
	hkwait(hkaddr);
	hkaddr->hkcs2 = unit;
	hkaddr->hkcs1 = hk_type[unit] | HK_DCLR | HK_GO;
	hkwait(hkaddr);

	addr = exadr(bp->b_xmem, bp->b_un.b_addr);
	addr += ndone;
	hkaddr->hkcyl = cn;
	hkaddr->hkda = (tn << 8) + sn;
	hkaddr->hkwc = wc;
	hkaddr->hkba = (caddr_t)addr;

	if	(!ubmap)
		hkaddr->hkxmem=hiint(addr);
	cmd = hk_type[unit] | ((hiint(addr) & 3) << 8) | HK_IE | HK_GO;
	if (bp->b_flags & B_READ)
		cmd |= HK_READ;
	else
		cmd |= HK_WRITE;
	hkaddr->hkcs1 = cmd;
	hktab.b_errcnt = 0;	/* error has been corrected */
	return (1);
}

/*
 * Return the number of blocks in a partition.  Call hkopen() to read the
 * label if necessary.  If an open is necessary then a matching close
 * will be done.
*/

daddr_t
hksize(dev)
register dev_t dev;
	{
	register struct dkdevice *disk;
	daddr_t psize;
	int     didopen = 0;

	disk = &hk_dk[dkunit(dev)];
/*
 * This should never happen but if we get called early in the kernel's
 * life (before opening the swap or root devices) then we have to do
 * the open here.
*/

	if      (disk->dk_openmask == 0)
		{
		if      (hkopen(dev, FREAD|FWRITE, S_IFBLK))
			return(-1);
		didopen = 1;
		}
	psize = disk->dk_parts[dkpart(dev)].p_size;
	if      (didopen)
		hkclose(dev, FREAD|FWRITE, S_IFBLK);
	return(psize);
	}
#endif NHK > 0

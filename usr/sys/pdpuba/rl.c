/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rl.c	1.11 (2.11BSD GTE) 1998/4/3
 */

/*
 *  RL01/RL02 disk driver
 *
 * Date: July 19, 1996
 * The driver was taking the WRITE LOCK (RLMP_WL) bit to indicate
 * an error, when all it really does is indicate that the disk is
 * write protected.  Set up RLMP_MASK to ignore this bit.  (Tim Shoppa)
 *
 * Date: January 7, 1996
 * Fix broken UCB_METER statistics gathering.
 *
 * Date: November 27, 1995
 * Add support for using the software unibus/qbus map.  This allows 3rd
 * party 18bit RL controllers (DSD-880) to be used in a 22bit Qbus system.
 * NOTE:  I have been told that the DEC RLV11 does not properly monitor
 * the I/O Page signal which means the RLV11 still can not be used.
 *
 * Date: August 1, 1995
 * Fix bug which prevented labeling disks with no label or a corrupted label.
 * Correct typographical error, the raclose() routine was being called by
 * mistake in the rlsize() routine.
 * 
 * Date: June 15, 1995.
 * Modified to handle disklabels.  This provides the ability to partition
 * a drive.  An RL02 can hold a root and swap partition quite easily and is
 * useful for maintenance.
 */

#include "rl.h"
#if NRL > 0

#if NRL > 4
error to have more than 4 drives - only 1 controller is supported.
#endif

#include "param.h"
#include "buf.h"
#include "machine/seg.h"
#include "user.h"
#include "systm.h"
#include "conf.h"
#include "dk.h"
#include "file.h"
#include "ioctl.h"
#include "stat.h"
#include "map.h"
#include "uba.h"
#include "disklabel.h"
#include "disk.h"
#include "syslog.h"
#include "rlreg.h"

#define	RL01_NBLKS	10240	/* Number of UNIX blocks for an RL01 drive */
#define	RL02_NBLKS	20480	/* Number of UNIX blocks for an RL02 drive */
#define	RL_CYLSZ	10240	/* bytes per cylinder */
#define	RL_SECSZ	256	/* bytes per sector */

#define	rlwait(r)	while (((r)->rlcs & RL_CRDY) == 0)
#define	RLUNIT(x) 	(dkunit(x) & 7)
#define RLMP_MASK	( ~( RLMP_WL | RLMP_DTYP | RLMP_HSEL ) )
#define RLMP_OK		( RLMP_HO | RLMP_BH | RLMP_LCKON )

struct	rldevice *RLADDR;

static	char	q22bae;
static	char	rlsoftmap = -1;	/* -1 = OK to change during attach
				 *  0 = Never use soft map
				 *  1 = Always use soft map
				 */
	daddr_t	rlsize();
	int	rlstrategy();
	void	rldfltlbl();

struct	buf	rlutab[NRL];	/* Seek structure for each device */
struct	buf	rltab;

struct	rl_softc {
	short	cn[4];		/* location of heads for each drive */
	short	nblks[4];	/* number of blocks on drive */
	short	dn;		/* drive number */
	short	com;		/* read or write command word */
	short	chn;		/* cylinder and head number */
	u_short	bleft;		/* bytes left to be transferred */
	u_short	bpart;		/* number of bytes transferred */
	short	sn;		/* sector number */
	union	{
		short	w[2];
		long	l;
	} rl_un;		/* address of memory for transfer */

} rl = {-1,-1,-1,-1};	/* initialize cn[] */

struct	dkdevice rl_dk[NRL];

#ifdef UCB_METER
static	int		rl_dkn = -1;	/* number for iostat */
#endif

rlroot()
{
	rlattach((struct rldevice *)0174400, 0);
}

rlattach(addr, unit)
	register struct rldevice *addr;
{
#ifdef UCB_METER
	if (rl_dkn < 0) {
		dk_alloc(&rl_dkn, NRL, "rl", 20L * 10L * 512L);
	}
#endif

	if (unit != 0)
		return (0);
	if ((addr != (struct rldevice *)NULL) && (fioword(addr) != -1)) {
		RLADDR = addr;
		if (fioword(&addr->rlbae) == -1)
			q22bae = -1;
#ifdef	SOFUB_MAP
		if (q22bae != 0 && !ubmap && rlsoftmap == -1)
			rlsoftmap = 1;
#endif
		return (1);
	}
	RLADDR = (struct rldevice *)NULL;
	return (0);
}

rlopen(dev, flag, mode)
	dev_t dev;
	int flag;
	int mode;
	{
	int	i, mask;
	int	drive = RLUNIT(dev);
	register struct	dkdevice *disk;
	
	if	(drive >= NRL || !RLADDR)
		return (ENXIO);
	disk = &rl_dk[drive];
	if	((disk->dk_flags & DKF_ALIVE) == 0)
		{
		if	(rlgsts(drive) < 0)
			return(ENXIO);
		}
/*
 * The drive has responded to a GETSTATUS (is alive).  Now we read the
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
		rlgetinfo(disk, dev);
		disk->dk_flags &= ~DKF_OPENING;
		wakeup(disk);
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
	dkoverlapchk(disk->dk_openmask, dev, disk->dk_label, "rl");
	if	(mode == S_IFCHR)
		disk->dk_copenmask |= mask;
	else if	(mode == S_IFBLK)
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
rlclose(dev, flag, mode)
	register dev_t	dev;
	int	flag, mode;
	{
	int	s, drive = RLUNIT(dev);
	register int	mask;
	register struct dkdevice *disk;

	disk = &rl_dk[drive];
	mask = 1 << dkpart(dev);
	if	(mode == S_IFCHR)
		disk->dk_copenmask &= ~mask;
	else if	(mode == S_IFBLK)
		disk->dk_bopenmask &= ~mask;
	else
		return(EINVAL);
	disk->dk_openmask = disk->dk_bopenmask | disk->dk_copenmask;
	if	(disk->dk_openmask == 0)
		{
		disk->dk_flags |= DKF_CLOSING;
		s = splbio();
		while	(rlutab[drive].b_actf)
			{
			disk->dk_flags |= DKF_WANTED;
			sleep(&rlutab[drive], PRIBIO);
			}
		splx(s);
		disk->dk_flags &= ~(DKF_CLOSING | DKF_WANTED);
		wakeup(disk);
		}
	return(0);
	}

/*
 * This code was moved from rlgetinfo() because it is fairly large and used
 * twice - once to initialize for reading the label and a second time if
 * there is no valid label present on the drive and the default one must be
 * used.
*/

void
rldfltlbl(disk, lp, dev)
	struct dkdevice *disk;
	register struct	disklabel *lp;
	dev_t	dev;
	{
	register struct	partition *pi = &lp->d_partitions[0];

	bzero(lp, sizeof (*lp));
	lp->d_type = DTYPE_DEC;
	lp->d_secsize = 512;		/* XXX */
	lp->d_nsectors = 20;
	lp->d_ntracks = 2;
	lp->d_secpercyl = 2 * 20;
	lp->d_npartitions = 1;		/* 'a' */
	pi->p_size = rl.nblks[dkunit(dev)];	/* entire volume */
	pi->p_fstype = FS_V71K;
	pi->p_frag = 1;
	pi->p_fsize = 1024;
/*
 * Put where rlstrategy() will look.
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
rlgetinfo(disk, dev)
	register struct dkdevice *disk;
	dev_t	dev;
	{
	struct	disklabel locallabel;
	char	*msg;
	register struct disklabel *lp = &locallabel;
/*
 * NOTE: partition 0 ('a') is used to read the label.  Therefore 'a' must
 * start at the beginning of the disk!  If there is no label or the label
 * is corrupted then 'a' will span the entire disk
*/

	rldfltlbl(disk, lp, dev);
	msg = readdisklabel((dev & ~7) | 0, rlstrategy, lp);	/* 'a' */
	if	(msg != 0)
		{
		log(LOG_NOTICE, "rl%da is entire disk: %s\n", dkunit(dev), msg);
		rldfltlbl(disk, lp, dev);
		}
	mapseg5(disk->dk_label, LABELDESC)
	bcopy(lp, (struct disklabel *)SEG5, sizeof (struct disklabel));
	normalseg5();
	bcopy(lp->d_partitions, disk->dk_parts, sizeof (lp->d_partitions));
	return;
	}

rlstrategy(bp)
	register struct	buf *bp;
{
	int	drive;
	register int	s;
	register struct dkdevice *disk;

	drive = RLUNIT(bp->b_dev);
	disk = &rl_dk[drive];

	if	(drive >= NRL || !RLADDR || !(disk->dk_flags & DKF_ALIVE))
		{
		bp->b_error = ENXIO;
		goto bad;
		}
	s = partition_check(bp, disk);
	if	(s < 0)
		goto bad;
	if	(s == 0)
		goto done;
#ifdef	SOFUB_MAP
	if	(rlsoftmap == 1)
		{
		if	(sofub_alloc(bp) == 0)
			return;
		}
	else
#endif
		mapalloc(bp);

	bp->av_forw = NULL;
	bp->b_cylin = (int)(bp->b_blkno/20L);
	s = splbio();
	disksort(&rlutab[drive], bp);	/* Put the request on drive Q */
	if	(rltab.b_active == 0)
		rlstart();
	splx(s);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
	iodone(bp);
	return;
}

rlstart()
{
	register struct rl_softc *rlp = &rl;
	register struct buf *bp, *dp;
	struct	dkdevice *disk;
	int unit;

	if((bp = rltab.b_actf) == NULL) {
		for(unit = 0;unit < NRL;unit++) {	/* Start seeks */
			dp = &rlutab[unit];
			if	(dp->b_actf == NULL)
				{
/*
 * No more requests in the drive queue.  If a close is pending waiting
 * for activity to be done on the drive then issue a wakeup and clear the
 * flag.
*/
				disk = &rl_dk[unit];
				if	(disk->dk_flags & DKF_WANTED)
					{
					disk->dk_flags &= ~DKF_WANTED;
					wakeup(dp);
					}
				continue;
				}
			rlseek((int)(dp->b_actf->b_blkno/20l),unit);
		}

		rlgss();	/* Put shortest seek on Q */
		if((bp = rltab.b_actf) == NULL)	/* No more work */
			return;
	}
	rltab.b_active++;
	rlp->dn = RLUNIT(bp->b_dev);
	rlp->chn = bp->b_blkno / 20;
	rlp->sn = (bp->b_blkno % 20) << 1;
	rlp->bleft = bp->b_bcount;
	rlp->rl_un.w[0] = bp->b_xmem & 077;
	rlp->rl_un.w[1] = (int) bp->b_un.b_addr;
	rlp->com = (rlp->dn << 8) | RL_IE;
	if (bp->b_flags & B_READ)
		rlp->com |= RL_RCOM;
	else
		rlp->com |= RL_WCOM;
	rlio();
}

rlintr()
{
	register struct buf *bp;
	register struct rldevice *rladdr = RLADDR;
	register status;

	if (rltab.b_active == NULL)
		return;
	bp = rltab.b_actf;
#ifdef UCB_METER
	if (rl_dkn >= 0)
		dk_busy &= ~(1 << (rl_dkn + rl.dn));
#endif
	if (rladdr->rlcs & RL_CERR) {
		if (rladdr->rlcs & RL_HARDERR && rltab.b_errcnt > 2) {
			harderr(bp, "rl");
			log(LOG_ERR, "cs=%b da=%b\n", rladdr->rlcs, RL_BITS,
				rladdr->rlda, RLDA_BITS);
		}
		if (rladdr->rlcs & RL_DRE) {
			rladdr->rlda = RLDA_GS;
			rladdr->rlcs = (rl.dn <<  8) | RL_GETSTATUS;
			rlwait(rladdr);
			status = rladdr->rlmp;
			if(rltab.b_errcnt > 2) {
				harderr(bp, "rl");
				log(LOG_ERR, "mp=%b da=%b\n", status, RLMP_BITS,
					rladdr->rlda, RLDA_BITS);
			}
			rladdr->rlda = RLDA_RESET | RLDA_GS;
			rladdr->rlcs = (rl.dn << 8) | RL_GETSTATUS;
			rlwait(rladdr);
			if(status & RLMP_VCHK) {
				rlstart();
				return;
			}
		}
		if (++rltab.b_errcnt <= 10) {
			rl.cn[rl.dn] = -1;
			rlstart();
			return;
		}
		else {
			bp->b_flags |= B_ERROR;
			rl.bpart = rl.bleft;
		}
	}

	if ((rl.bleft -= rl.bpart) > 0) {
		rl.rl_un.l += rl.bpart;
		rl.sn=0;
		rl.chn++;
		rlseek(rl.chn,rl.dn);	/* Seek to new position */
		rlio();
		return;
	}
	bp->b_resid = 0;
	rltab.b_active = NULL;
	rltab.b_errcnt = 0;
	rltab.b_actf = bp->av_forw;
#ifdef notdef
	if((bp != NULL)&&(rlutab[rl.dn].b_actf != NULL))
		rlseek((int)(rlutab[rl.dn].b_actf->b_blkno/20l),rl.dn);
#endif
#ifdef	SOFUB_MAP
	if	(rlsoftmap == 1)
		sofub_relse(bp, bp->b_bcount);
#endif
	iodone(bp);
	rlstart();
}

rlio()
{
	register struct rldevice *rladdr = RLADDR;

	if (rl.bleft < (rl.bpart = RL_CYLSZ - (rl.sn * RL_SECSZ)))
		rl.bpart = rl.bleft;
	rlwait(rladdr);
	rladdr->rlda = (rl.chn << 6) | rl.sn;
	rladdr->rlba = (caddr_t)rl.rl_un.w[1];
	rladdr->rlmp = -(rl.bpart >> 1);
	if	(q22bae == 0)
		rladdr->rlbae = rl.rl_un.w[0];
	rladdr->rlcs = rl.com | (rl.rl_un.w[0] & 03) << 4;
#ifdef UCB_METER
	if (rl_dkn >= 0) {
		int dkn = rl_dkn + rl.dn;

		dk_busy |= 1<<dkn;
		dk_xfer[dkn]++;
		dk_wds[dkn] += rl.bpart>>6;
	}
#endif
}

/*
 * Start a seek on an rl drive
 * Greg Travis, April 1982 - Adapted to 2.8/2.9 BSD Oct 1982/May 1984
 */
static
rlseek(cyl, dev)
	register int cyl;
	register int dev;
{
	struct rldevice *rp;
	register int dif;

	rp = RLADDR;
	if(rl.cn[dev] < 0)	/* Find the frigging heads */
		rlfh(dev);
	dif = (rl.cn[dev] >> 1) - (cyl >> 1);
	if(dif || ((rl.cn[dev] & 01) != (cyl & 01))) {
		if(dif < 0)
			rp->rlda = (-dif << 7) | RLDA_SEEKHI | ((cyl & 01) << 4);
		else
			rp->rlda = (dif << 7) | RLDA_SEEKLO | ((cyl & 01) << 4);
		rp->rlcs = (dev << 8) | RL_SEEK;
		rl.cn[dev] = cyl;
#ifdef UCB_METER
		if (rl_dkn >= 0) {
			int dkn = rl_dkn + dev;

			dk_busy |= 1<<dkn;	/* Mark unit busy */
			dk_seek[dkn]++;		/* Number of seeks */
		}
#endif
		rlwait(rp);	/* Wait for command */
	}
}

/* Find the heads for the given drive */
static
rlfh(dev)
	register int dev;
{
	register struct rldevice *rp;

	rp = RLADDR;
	rp->rlcs = (dev << 8) | RL_RHDR;
	rlwait(rp);
	rl.cn[dev] = ((unsigned)rp->rlmp & 0177700) >> 6;
}

/*
 * Find the shortest seek for the current drive and put
 * it on the activity queue
 */
static
rlgss()
{
	register int unit, dcn;
	register struct buf *dp;

	rltab.b_actf = NULL;	/* We fill this queue with up to 4 reqs */
	for(unit = 0;unit < NRL;unit++) {
		dp = rlutab[unit].b_actf;
		if(dp == NULL)
			continue;
		rlutab[unit].b_actf = dp->av_forw;	/* Out */
		dp->av_forw = dp->av_back = NULL;
		dcn = (dp->b_blkno/20) >> 1;
		if(rl.cn[unit] < 0)
			rlfh(unit);
		if(dcn < rl.cn[unit])
			dp->b_cylin = (rl.cn[unit] >> 1) - dcn;
		else
			dp->b_cylin = dcn - (rl.cn[unit] >> 1);
		disksort(&rltab, dp);	/* Put the request on the current q */
	}
}

rlioctl(dev, cmd, data, flag)
	dev_t	dev;
	int	cmd;
	caddr_t data;
	int	flag;
	{
	register int	error;
	struct	dkdevice *disk = &rl_dk[RLUNIT(dev)];

	error = ioctldisklabel(dev, cmd, data, flag, disk, rlstrategy);
	return(error);
	}

#ifdef RL_DUMP
/*
 * Dump routine for RL01/02
 * This routine is stupid (because the rl is stupid) and assumes that
 * dumplo begins on a track boundary!
 */

#define DBSIZE	10	/* Half a track of sectors.  Can't go higher 
			 * because only a single UMR is set for the transfer.
			 */

rldump(dev)
	dev_t dev;
{
	register struct rldevice *rladdr = RLADDR;
	struct	dkdevice *disk;
	struct	partition *pi;
	daddr_t bn, dumpsize;
	long paddr;
	int count, memblks;
	u_int com;
	int ccn, cn, tn, sn, unit, dif, partition;
	register struct ubmap *ubp;

	unit = RLUNIT(dev);
	if	(unit >= NRL)
		return(EINVAL);
	partition = dkpart(dev);
	disk = &rl_dk[unit];
	pi = &disk->dk_parts[partition];

	if	(!(disk->dk_flags & DKF_ALIVE))
		return(ENXIO);
	if	(pi->p_fstype != FS_SWAP)
		return(EFTYPE);
	if	(rlsoftmap == 1)	/* No crash dumps via soft map */
		return(EFAULT);

	dumpsize = rlsize(dev) - dumplo;
	memblks = ctod(physmem);

	if	(dumplo < 0 || dumpsize <= 0)
		return(EINVAL);
	if	(memblks > dumpsize)
		memblks = dumpsize;
	bn = dumplo + pi->p_offset;

	rladdr->rlcs = (dev << 8) | RL_RHDR;	/* Find the heads */
	rlwait(rladdr);
	ccn = ((unsigned)rladdr->rlmp&0177700) >> 6;

	ubp = &UBMAP[0];
	for (paddr = 0L; memblks > 0; ) {
		count = MIN(memblks, DBSIZE);
		cn = bn / 20;
		sn = (unsigned)(bn % 20) << 1;
		dif = (ccn >> 1) - (cn >> 1);
		if(dif || ((ccn & 01) != (cn & 01))) {
			if(dif < 0)
				rladdr->rlda = (-dif << 7) | RLDA_SEEKHI |
					((cn & 01) << 4);
			else
				rladdr->rlda = (dif << 7) | RLDA_SEEKLO |
					((cn & 01) << 4);
			rladdr->rlcs = (dev << 8) | RL_SEEK;
			ccn = cn;
			rlwait(rladdr);
		}
		rladdr->rlda = (cn << 6) | sn;
		rladdr->rlmp = -(count << (PGSHIFT-1));
		com = (dev << 8) | RL_WCOM;
		/* If there is a map - use it */
		if(ubmap) {
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			rladdr->rlba = 0;
		} else {
			rladdr->rlba = loint(paddr);
			if	(q22bae == 0)
				rladdr->rlbae = hiint(paddr);
			com |= (hiint(paddr) & 03) << 4;
		}
		rladdr->rlcs = com;
		rlwait(rladdr);
		if(rladdr->rlcs & RL_CERR) {
			if(rladdr->rlcs & RL_NXM)
				return(0);	/* End of memory */
			log(LOG_ERR, "rl%d: dmp err, cs=%b da=%b mp=%b\n",
				dev,rladdr->rlcs,RL_BITS,rladdr->rlda,
				RLDA_BITS, rladdr->rlmp, RLMP_BITS);
			return(EIO);
		}
		paddr += (count << PGSHIFT);
		bn += count;
		memblks -= count;
	}
	return(0);	/* Filled the disk */
}
#endif RL_DUMP

/*
 * Return the number of blocks in a partition.  Call rlopen() to online
 * the drive if necessary.  If an open is necessary then a matching close
 * will be done.
*/
daddr_t
rlsize(dev)
	register dev_t dev;
	{
	register struct dkdevice *disk;
	daddr_t	psize;
	int	didopen = 0;

	disk = &rl_dk[RLUNIT(dev)];
/*
 * This should never happen but if we get called early in the kernel's
 * life (before opening the swap or root devices) then we have to do
 * the open here.
*/
	if	(disk->dk_openmask == 0)
		{
		if	(rlopen(dev, FREAD|FWRITE, S_IFBLK))
			return(-1);
		didopen = 1;
		}
	psize = disk->dk_parts[dkpart(dev)].p_size;
	if	(didopen)
		rlclose(dev, FREAD|FWRITE, S_IFBLK);
	return(psize);
	}

/*
 * This routine is only called by rlopen() the first time a drive is
 * touched.  Once the number of blocks has been determined the drive is
 * marked 'alive'.
 *
 * For some unknown reason the RL02 (seems to be
 * only drive 1) does not return a valid drive status
 * the first time that a GET STATUS request is issued
 * for the drive, in fact it can take up to three or more
 * GET STATUS requests to obtain the correct status.
 * In order to overcome this "HACK" the driver has been
 * modified to issue a GET STATUS request, validate the
 * drive status returned, and then use it to determine the
 * drive type. If a valid status is not returned after eight
 * attempts, then an error message is printed.
 */
rlgsts(drive)
	register int	drive;
	{
	register int	ctr = 0;
	register struct	rldevice *rp = RLADDR;

	do	{ /* get status and reset when first touching this drive */
		rp->rlda = RLDA_RESET | RLDA_GS;
		rp->rlcs = (drive << 8) | RL_GETSTATUS;	/* set up csr */
		rlwait(rp);
		} while (((rp->rlmp & RLMP_MASK) != RLMP_OK) && (++ctr < 16));
	if	(ctr >= 16)
		{
		log(LOG_ERR, "rl%d: !sts cs=%b da=%b\n", drive,
			rp->rlcs, RL_BITS, rp->rlda, RLDA_BITS);
		rl_dk[drive].dk_flags &= ~DKF_ALIVE;
		return(-1);
		}
	if	(rp->rlmp & RLMP_DTYP)
		rl.nblks[drive] = RL02_NBLKS;	/* drive is RL02 */
	else
		rl.nblks[drive] = RL01_NBLKS;	/* drive RL01 */
	rl_dk[drive].dk_flags |= DKF_ALIVE;
	return(0);
	}
#endif /* NRL */

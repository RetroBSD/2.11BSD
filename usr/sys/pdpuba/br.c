/*
 * BR disk driver
 * modified from the UNIX RP03 driver for BR disk drive -JM
 * 11/11/84 - Added dump routine for taking 2.9 crash dumps in swap. -SMS
 * 9/28/85  - Use brreg.h so as to look more like a 2.9 disk handler. -SMS
 * 2/16/86  - Rewrite!  Drop the bropen, brclose and brgch functions.  Do
 *	      initialization on a per drive basis rather than controller.
 *	      The bropen and brclose functions were only used to perform
 *	      the init'ing of a drive, and there was a problem in single
 *	      user mode where a 'umount' could close the device before all
 *	      data for that filesystem was written out thereby giving offline
 *	      errors and causing corruption of the disc.  Also defined a
 *	      disc slice that encompassed the whole drive for ease in copying,
 *	      several other mods made because this was done.  Overall, several
 *	      pages of code were removed. -SMS
 *	      Just discovered that bropen() and brclose() were defined as 
 *	      nulldev() in c.c!  Wasted code all this time!  The offline errors
 *	      observed were a result of entry into brstrategy() during a 
 *	      'umount' when bropen() had never been called in the first place!!!
 * 12/6/87  - Changes to run under 2.10bsd.  Still single controller only.
 *	      Considering we don't make the controller any more that's fairly
 *	      safe to assume.  Partitions drastically changed to allow room
 *	      on T300 and T200 to hold the source distribution.
 *	      Autoconfigure logic finally added though. -SMS
 * 2/17/89  - For 2.10.1BSD added old 2.9BSD /usr,/userfiles, and /minkie 
 *	      partitions as partitions 'e', 'f', and 'g' as an aid in
 *	      converting the systems.  BE CAREFUL!  For T300 only.
 * 8/4/89   - Use the log() function to record soft errors.
 * 9/22/91  - remove read and write entry - use common raw read/write routine.
 * 12/23/92 - add the partition size routine.
 * 1/2/93   - remove unibus map ifdefs, the run time check using 'ubmap' is
 *	      sufficient and does the right thing.
 * 1995/04/13 - change reference to dkunit.
 */

#include "br.h"
#if NBR > 0

#include "param.h"
#include "../machine/seg.h"

#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "user.h"
#include "brreg.h"
#include "dk.h"
#include "disklabel.h"
#include "disk.h"
#include "syslog.h"
#include "map.h"
#include "uba.h"

#define	BRADDR ((struct brdevice *) 0176710)
#define	brunit(dev)	((dev >> 3) & 7)
#define	SECTRK  brc->sectrk
#define	TRKCYL  brc->trkcyl

struct br_char {
	struct br_dsz {
		daddr_t nblocks;
		int cyloff;
	} br_sizes[8];
	int sectrk, trkcyl;
} br_chars[] =
{				/* T300 */
	18240,	 0,		/* cyl 000 - 029 */
	12160,	30,		/* cyl 030 - 049 */
	232256,	50,		/* cyl 050 - 431 */
	232256,	432,		/* cyl 432 - 813 */
	154432,	 50,		/* 'e' is old 2.9 'c' partition */
	154432,	 304,		/* 'f' is old 2.9 'd' partition */
	154432,	 558,		/* 'g' is old 2.9 'e' partition */
	495520,	 0,		/* cyl 000 - 814 */
	32,	 19,		/* 32 sectrk, 19 trkcyl */
 /* T200 */
	18392,  0,		/* cyl 000 - 043 */
	12122,  43,		/* cyl 044 - 072 */
	231990, 73,		/* cyl 073 - 627 */
	78166,  443,            /* cyl 628 - 814 */
	0,	0,
	0,	0,
	0,      0,
	340670, 0,		/* cyl 000 - 814  */
	22,     19,		/* 22 sectrk, 19 trkcyl */
/* T80 */
	18400,	  0,		/* cyl 000 - 114 */
	12320,	115,		/* cyl 115 - 190 */
	99840,	191,		/* cyl 191 - 814 */
	0,	  0,
	0,	  0,
	0,	  0,
	0,	  0,
	130300,	  0,
	32,	  5,		/* 32 sectrk, 5 trkcyl */
/* T50 */
	18260,	  0,		/* cyl 000 - 165 */
	12210,	166,		/* cyl 166 - 276 */
	59180,	277,		/* cyl 277 - 814 */
	0,	  0,
	0,	  0,
	0,	  0,
	0,	  0,
	89650,	  0,
	22,	  5,		/* 22 sectrk, 5 trkcyl */
};

/*
 * Define the recovery strobes and offsets in br_da
 */
static int br_offs[] = {
	0,		0,		0,		STBE,
	STBE,		STBL,		STBL,		OFFP+STBL,
	OFFP+STBL,	OFFP,		OFFP,		OFFP+STBE,
	OFFP+STBE,	OFFM+STBE,	OFFM+STBE,	OFFP,
	OFFP,		OFFP+STBL,	OFFP+STBL,	0
};

#ifdef UCB_METER
static int br_dkn = -1;
#endif
struct buf brtab;
struct br_char *br_disk[NBR];
struct brdevice *Br_addr;

brroot()
{
	brattach((struct brdevice *)BRADDR, 0);
}

brattach(braddr, unit)
	register struct brdevice *braddr;
	int unit;
{

#ifdef UCB_METER
	if (br_dkn < 0)
		dk_alloc(&br_dkn, NBR, "br", 0L);
#endif
	if (unit >= NBR)
		return(0);
	if (braddr && (fioword(braddr) != -1)) {
		Br_addr = braddr;
		return(1);
	}
	return(0);
}

bropen(dev, flag)
	dev_t	dev;
	int	flag;
{
	register int dn = brunit(dev);

	if	(dn >= NBR || !Br_addr)
		return(ENXIO);
	if	(!br_disk[dn])
		brinit(dn);
	if	(!br_disk[dn])
		return(EIO);
	return(0);
}

brstrategy(bp)
	register struct buf *bp;
{
	register struct buf *dp;
	register int unit;
	struct br_char *brc;
	struct br_dsz *brz;
	long sz;
	int drive, s;

	unit = bp->b_dev & 07;
	drive = brunit(bp->b_dev);
	if	(!(brc = br_disk[drive])) {
		brinit(drive);
		if (!(brc = br_disk[drive])) {
			bp->b_error = ENODEV;
			bp->b_flags |= B_ERROR;
			iodone(bp);
			return;
		}
	}
	brz = &brc->br_sizes[unit];
	sz = (bp->b_bcount + 511L) >> 9;
	if (bp->b_blkno == brz->nblocks) {
		bp->b_resid = bp->b_bcount;
		iodone(bp);
		return;
	}
	if (bp->b_blkno + sz > brz->nblocks) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if (Br_addr->brae >= 0)
		mapalloc(bp);
	bp->b_cylin = bp->b_blkno/(SECTRK*TRKCYL) + brz->cyloff;
	s = splbio();
	dp = &brtab;
	disksort(dp, bp);
	if (dp->b_active == NULL)
		brstart();
	splx(s);
}

static
brstart()
{
	register struct buf *bp;
	register int unit;
	int com,cn,tn,sn,dn;
	daddr_t bn;
	struct br_char *brc;
	struct br_dsz *brz;
 
	if ((bp = brtab.b_actf) == NULL)
		return;
	Br_addr->brds = -1;
	brtab.b_active++;
	if (!(Br_addr->brcs.w & BR_RDY)) {
		timeout(brstart, 0, 4);
		return;
	}
	unit = bp->b_dev & 07;
	dn = brunit(bp->b_dev);
	if (!(brc = br_disk[dn])) {
		brinit(dn);
		if (!(brc = br_disk[dn])) {
			bp->b_flags |= B_ERROR;
			brdone(bp);
			return;
		}
	}
	brz = &brc->br_sizes[unit];
	bn = bp->b_blkno;
	cn = bn/(SECTRK*TRKCYL) + brz->cyloff;
	sn = bn%(SECTRK*TRKCYL);
	tn = sn/SECTRK;
	sn = sn%SECTRK;
	if (Br_addr->brae < 0)
		Br_addr->brae = bp->b_xmem;
	Br_addr->brcs.w = (dn<<8);
	Br_addr->brda = (tn<<8) | sn;
	cn |= br_offs[brtab.b_errcnt];
	Br_addr->brca = cn;
	Br_addr->brba = bp->b_un.b_addr;
	Br_addr->brwc = -(bp->b_bcount>>1);
	com = ((bp->b_xmem&3)<<4) | BR_IDE | BR_GO;
	if (bp->b_flags & B_READ)
		com |= BR_RCOM;
	else
		com |= BR_WCOM;
	Br_addr->brcs.w |= com;
#ifdef UCB_METER
	if (br_dkn >= 0) {
		dk_busy |= 1 << (br_dkn + dn);
		dk_xfer[br_dkn + dn]++;
		dk_seek[br_dkn + dn]++;
		dk_wds[br_dkn + dn] += (bp->b_bcount >> 6);
	}
#endif
}

static
brinit(drive) 
	register int drive;
{
	register int ctr = 0;
	register struct br_char **br = &br_disk[drive];
 
	/*
	 * Clear the drive's entry in br_disk.  Select the unit.  If the
	 * unit exists, switch on the spindle type. and set the br_disk
	 * table entry
	 */
	*br = (struct br_char *)NULL;
	do {
		Br_addr->brcs.w = (drive << 8) | BR_HSEEK | BR_GO;
		while ((Br_addr->brcs.w & BR_RDY) == 0 && --ctr) ; 
	} while (Br_addr->brer & BRER_SUBUSY);
	if ((Br_addr->brcs.w & BR_HE) == 0) {
		switch (Br_addr->brae & AE_DTYP) {
		case AE_T300:
			*br = &br_chars[0];
			break;
		case AE_T200:
			*br = &br_chars[1];
			break;
		case AE_T80:
			*br = &br_chars[2];
			break;
		case AE_T50:
			*br = &br_chars[3];
			break;
		}
#ifdef UCB_METER
		if (br_dkn >= 0)
			dk_wps[br_dkn + drive] =
			    (long)(*br)->sectrk * (60L * 256L);
#endif
	}
}

brintr(dev)
	int dev;
{
	register struct buf *bp;
	register int ctr = 0;
	struct brdevice brsave;

	if (brtab.b_active == NULL)
		return;
	brsave = *Br_addr;
	bp = brtab.b_actf;
	if (!(brsave.brcs.w & BR_RDY))
		return;
#ifdef UCB_METER
	if (br_dkn >= 0)
		dk_busy &= ~(1<<(br_dkn + dev));
#endif
	if (brsave.brcs.w < 0) {
		if (brsave.brer & BRER_SUBUSY) {
			timeout(brstart, 0, 5);
			return;
		}
		if (brsave.brds & (BRDS_SUFU|BRDS_SUSI|BRDS_HNF)) {
			Br_addr->brcs.c[0] = BR_HSEEK|BR_GO;
			while (((Br_addr->brds&BRDS_SURDY) == 0) && --ctr);
		}
		Br_addr->brcs.w = BR_IDLE|BR_GO;
		ctr = 0; 
		while (((Br_addr->brcs.w&BR_RDY) == 0) && --ctr) ;
		if (brtab.b_errcnt == 0) {
			log(LOG_WARNING,"br%d%c ds:%b er:%b cs:%b wc:%o ba:%o ca:%o da:%o bae:%o\n",
			    dkunit(bp->b_dev), 'a'+ dkpart(bp->b_dev),
			    brsave.brds, BRDS_BITS, brsave.brer, BRER_BITS,
			    brsave.brcs.w, BR_BITS, brsave.brwc,brsave.brba,
			    brsave.brca, brsave.brda, brsave.brae);
		}
		brtab.b_errcnt++;
		if (brtab.b_errcnt < 20) {
			brstart();
			return;
		}
		harderr(bp,"br");
		bp->b_flags |= B_ERROR;
	}
	brdone(bp);
}

static
brdone (bp)
	register struct buf *bp;
{
	brtab.b_active = NULL;
	brtab.b_errcnt = 0;
	brtab.b_actf = bp->av_forw;
	bp->b_resid = 0;
	iodone(bp);
	brstart();
}
 
#ifdef BR_DUMP
/*
 * Dump routine.  Dumps from dumplo to end of memory/end of disk section for
 * minor(dev).
 */
#define	DBSIZE	16			/* unit of transfer, same number */

brdump(dev)
	dev_t dev;
{
	struct br_char *brc;
	struct ubmap *ubp;
	daddr_t bn, dumpsize;
	long paddr;
	int count, cyl, dn, cn, tn, sn, unit, com;

	unit = dev & 07;
	dn = brunit(dev);
	if ((bdevsw[major(dev)].d_strategy != brstrategy) || dn >= NBR)
		return(EINVAL);
	if (!Br_addr || !(brc = br_disk[dn]))
		return(ENXIO);
	dumpsize = brc->br_sizes[unit].nblocks;
	cyl = brc->br_sizes[unit].cyloff;
	if ((dumplo < 0) || (dumplo >= dumpsize))
		return(EINVAL);
	dumpsize -= dumplo;
	while (!(Br_addr->brcs.w & BR_RDY));
	ubp = &UBMAP[0];
	for (paddr = 0L; dumpsize > 0; dumpsize -= count) {
		count = dumpsize > DBSIZE ? DBSIZE : dumpsize;
		bn = dumplo + (paddr >> PGSHIFT);
		cn = (bn / (SECTRK * TRKCYL)) + cyl;
		sn = bn % (SECTRK * TRKCYL);
		tn = sn / SECTRK;
		sn = sn % SECTRK;
		Br_addr->brca = cn;
		Br_addr->brda = (tn << 8) | sn;
		Br_addr->brwc = -(count << (PGSHIFT-1));
		com = (dn << 8) | BR_GO | BR_WCOM;
		if (ubmap && Br_addr->brae >= 0) {
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			Br_addr->brba = 0;
		}
		else {
			Br_addr->brba = (caddr_t)loint(paddr);
			Br_addr->brae = hiint(paddr);
			com |= ((hiint(paddr) & 3) << 4);
		}
		Br_addr->brcs.w = com;
		while (!(Br_addr->brcs.w & BR_RDY));
		if (Br_addr->brcs.w < 0) {
			if (Br_addr->brer & BRER_NXME)
				return(0);	/* end of memory */
			return(EIO);
		}
		paddr += (DBSIZE << PGSHIFT);
	}
	return(0);				/* filled disk */
}
#endif /* BR_DUMP */

/*
 * Assumes the 'open' entry point has been called to validate the unit
 * number and fill in the drive type structure.
*/
daddr_t
brsize(dev)
	register dev_t dev;
	{
	register struct	br_char *brc;

	brc = br_disk[brunit(dev)];
	return(brc->br_sizes[dev & 7].nblocks);
	}
#endif /* NBR */

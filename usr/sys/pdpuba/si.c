/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)si.c	1.5 (2.11BSD GTE) 1995/04/13
 */

/*
 *	SI 9500 driver for CDC 9766 disks
 */

#include "si.h"
#if	NSI > 0
#include "param.h"
#include "../machine/seg.h"

#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "sireg.h"
#include "map.h"
#include "uba.h"
#include "dk.h"
#include "disklabel.h"
#include "disk.h"
#include "xp.h"
#include "errno.h"

#define	SI_NSECT	32
#define	SI_NTRAC	19

#if NXPD > 0
extern struct size {
	daddr_t	nblocks;
	int	cyloff;
} rm5_sizes[8];
#else !NXPD
/*
 * We reserve room for bad block forwarding information even though this
 * driver doesn't support bad block forwarding.  This allows us to simply
 * copy the table from xp.c.
 */
struct size {
	daddr_t	nblocks;
	int	cyloff;
} rm5_sizes[8] = {	/* RM05, CDC 9766 */
	9120,	  0,	/* a: cyl   0 -  14 */
	9120,	 15,	/* b: cyl  15 -  29 */
	234080,	 30,	/* c: cyl  30 - 414 */
	247906,	415,	/* d: cyl 415 - 822, reserve 1 track + 126 */
	164160,	 30,	/* e: cyl  30 - 299 */
	152000,	300,	/* f: cyl 300 - 549 */
	165826,	550,	/* g: cyl 550 - 822, reserve 1 track + 126 */
	500384,	  0,	/* h: cyl   0 - 822 */
};
#endif NXPD

struct	sidevice *SIADDR;

int	si_offset[] = { SI_OFP,	SI_OFM };

struct	buf	sitab;
struct	buf	siutab[NSI];

int	sicc[NSI];	/* Current cylinder */
int	dualsi = 0;	/* dual port flag */

#ifdef UCB_METER
static	int		si_dkn = -1;	/* number for iostat */
#endif

void
siroot()
{
	siattach((struct sidevice *)0176700, 0);
}

siattach(addr, unit)
register struct sidevice *addr;
{
#ifdef UCB_METER
	if (si_dkn < 0) {
		dk_alloc(&si_dkn, NSI+1, "si", 60L * 32L * 256L);
		if (si_dkn >= 0)
			dk_wps[si_dkn+NSI] = 0L;
	}
#endif

	if (unit != 0)
		return(0);
	if ((addr != (struct sidevice *) NULL) && (fioword(addr) != -1))
	{
		SIADDR = addr;
		if(addr ->siscr != 0)
			dualsi++;	/* dual port controller */
		if((addr->sierr & (SIERR_ERR | SIERR_CNT)) == (SIERR_ERR | SIERR_CNT))
			dualsi++;
		return(1);
	}
	SIADDR = (struct sidevice *) NULL;
	return(0);
}

siopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int unit;

	unit = minor(dev) & 077;
	if (unit >= (NSI << 3) || !SIADDR)
		return (ENXIO);
	return (0);
}

sistrategy(bp)
register struct	buf *bp;
{
	register struct buf *dp;
	register int unit;
	long bn;
	int s;

	unit = minor(bp->b_dev) & 077;
	if (unit >= (NSI << 3) || (SIADDR == (struct sidevice *) NULL)) {
		bp->b_error = ENXIO;
		goto errexit;
	}
	if (bp->b_blkno < 0 ||
	    (bn = bp->b_blkno) + (long) ((bp->b_bcount + 511) >> 9)
	    > rm5_sizes[unit & 07].nblocks) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	mapalloc(bp);
	bp->b_cylin = bn / (SI_NSECT * SI_NTRAC) + rm5_sizes[unit & 07].cyloff;
	unit = dkunit(bp->b_dev);
	dp = &siutab[unit];
	s = splbio();
	disksort(dp, bp);
	if (dp->b_active == 0) {
		siustart(unit);
		if (sitab.b_active == 0)
			sistart();
	}
	splx(s);
}

/*
 * Unit start routine.
 * Seek the drive to where the data is
 * and then generate another interrupt
 * to actually start the transfer.
 * If there is only one drive on the controller
 * or we are very close to the data, don't
 * bother with the search.  If called after
 * searching once, don't bother to look
 * where we are, just queue for transfer (to avoid
 * positioning forever without transferring).
 */
siustart(unit)
register unit;
{
	register struct	sidevice *siaddr = SIADDR;
	register struct buf *dp;
	struct	buf *bp;
	daddr_t	bn;
	int sn, cn, csn;

	if (unit >= NSI)
		return;
#ifdef UCB_METER
	if (si_dkn >= 0)
		dk_busy &= ~(1 << (si_dkn + unit));
#endif
	dp = &siutab[unit];
	if ((bp = dp->b_actf) == NULL)
		return;
	/*
	 * If we have already positioned this drive,
	 * then just put it on the ready queue.
	 */
	if (dp->b_active)
		goto done;
	dp->b_active++;

#if	NSI >	1
	/*
	 * If drive is not ready, forget about positioning.
	 */
	if(dualsi)
		getsi();
	if ((siaddr->sissr >> (unit*4)) & SISSR_NRDY)
		goto done;

	/*
	 * Figure out where this transfer is going to
	 * and see if we are close enough to justify not searching.
	 */
	bn = bp->b_blkno;
	cn = bp->b_cylin;
	sn = bn % (SI_NSECT * SI_NTRAC);
	sn = (sn + SI_NSECT) % SI_NSECT;

	if (sicc[unit] == cn)
		goto done;

search:
	siaddr->sisar = (unit << 10) | cn;
	if(dualsi)
		SIADDR->siscr = 0;
	sicc[unit] = cn;
#ifdef UCB_METER
	/*
	 * Mark unit busy for iostat.
	 */
	if (si_dkn >= 0) {
		int dkn = si_dkn + unit;

		dk_busy |= 1<<dkn;
		dk_seek[dkn]++;
	}
#endif
	return;
#endif	NSI > 1

done:
	/*
	 * Device is ready to go.
	 * Put it on the ready queue for the controller.
	 */
	dp->b_forw = NULL;
	if (sitab.b_actf == NULL)
		sitab.b_actf = dp;
	else
		sitab.b_actl->b_forw = dp;
	sitab.b_actl = dp;
}

/*
 * Start up a transfer on a drive.
 */
sistart()
{
	register struct sidevice *siaddr = SIADDR;
	register struct buf *bp;
	register unit;
	struct	buf *dp;
	daddr_t	bn;
	int	dn, sn, tn, cn;
	int	offset;

loop:
	/*
	 * Pull a request off the controller queue.
	 */
	if ((dp = sitab.b_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		sitab.b_actf = dp->b_forw;
		goto loop;
	}
	/*
	 * Mark controller busy and
	 * determine destination of this request.
	 */
	sitab.b_active++;
	unit = minor(bp->b_dev) & 077;
	dn = dkunit(bp->b_dev);
	bn = bp->b_blkno;
	cn = bn / (SI_NSECT * SI_NTRAC) + rm5_sizes[unit & 07].cyloff;
	sn = bn % (SI_NSECT * SI_NTRAC);
	tn = sn / SI_NSECT;
	sn = sn % SI_NSECT;

	/*
	 * Check that drive is ready.
	 */
	if(dualsi)
		getsi();
	if ((siaddr->sissr >> (dn*4)) & SISSR_NRDY) {
		sitab.b_active = 0;
		sitab.b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		goto loop;
	}

	offset = 0;
	if (sitab.b_errcnt >= 16 && (bp->b_flags & B_READ))
		offset = si_offset[sitab.b_errcnt & 1];

	siaddr->sipcr = (dn << 10) | cn;
	siaddr->sihsr = (tn << 5) + sn;
	siaddr->simar = bp->b_un.b_addr;
	siaddr->siwcr = bp->b_bcount >> 1;
	/*
	 * Warning:  unit is being used as a temporary.
	 */
	unit = offset | ((bp->b_xmem & 3) << 4) | SI_IE | SI_GO;
	if (bp->b_flags & B_READ)
		unit |= SI_READ;
	else
		unit |= SI_WRITE;
	siaddr->sicnr = unit;

#ifdef UCB_METER
	if (si_dkn >= 0) {
		int dkn = si_dkn + NSI;

		dk_busy |= 1<<dkn;
		dk_seek[dkn]++;
		dk_wds[dkn] += bp->b_bcount >> 6;
	}
#endif
}

/*
 * Handle a disk interrupt.
 */
siintr()
{
	register struct sidevice *siaddr = SIADDR;
	register struct buf *dp;
	register unit;
	struct	buf *bp;
	int	ss, ssr;

	ssr = siaddr->sissr;
	if (sitab.b_active) {
#ifdef UCB_METER
		if (si_dkn >= 0)
			dk_busy &= ~(1 << (si_dkn + NSI));
#endif
		/*
		 * Get device and block structures.
		 */
		dp = sitab.b_actf;
		bp = dp->b_actf;
		unit = dkunit(bp->b_dev);
		/*
		 * Check for and process errors.
		 */
		if (siaddr->sierr & SIERR_ERR) {
			/*
			 * After 18 retries (16 without offset and
			 * 2 with offset positioning), give up.
			 */
			if (++sitab.b_errcnt > 18) {
			    bp->b_flags |= B_ERROR;
			    harderr(bp, "si");
			    printf("cnr=%b err=%b\n", siaddr->sicnr,
				SI_BITS, siaddr->sierr, SIERR_BITS);
			} else
			    sitab.b_active = 0;
			
			siaddr->sicnr = SI_RESET;
			if(dualsi)
				getsi();
			siaddr->sicnr = SI_IE;

			sicc[unit] = -1;
		}

		if(dualsi)
			SIADDR->siscr = 0;
		if (sitab.b_active) {
			sitab.b_active = 0;
			sitab.b_errcnt = 0;
			sitab.b_actf = dp->b_forw;
			dp->b_active = 0;
			dp->b_actf = bp->av_forw;
			bp->b_resid = ~siaddr->siwcr;
			if (bp->b_resid == 0xffff) bp->b_resid = 0;
			bp->b_resid <<= 1;
			iodone(bp);
			if (dp->b_actf)
				siustart(unit);
		}
	}

	for (unit = 0; unit < NSI; unit++) {
		ss = (ssr >> unit*4) & 017;
		if (ss & (SISSR_BUSY|SISSR_ERR)) {
			harderr(bp, "si");
			printf("ssr=%b err=%b\n", ssr,
			    SISSR_BITS, siaddr->sierr, SIERR_BITS);
			sicc[unit] = -1;
			siustart(unit);
		}
		else if (ss & SISSR_DONE) 
			siustart(unit);
	}

	sistart();
}

#ifdef SI_DUMP
/*
 *  Dump routine for SI 9500
 *  Dumps from dumplo to end of memory/end of disk section for minor(dev).
 */

#define DBSIZE	16			/* number of blocks to write */

sidump(dev)
dev_t	dev;
{
	register struct sidevice *siaddr = SIADDR;
	daddr_t	bn, dumpsize;
	long	paddr;
	register count;
	register struct ubmap *ubp;
	int cn, tn, sn, unit;

	unit = minor(dev) >> 3;
	if ((bdevsw[major(dev)].d_strategy != sistrategy)	/* paranoia */
	    || unit > NSI)
		return(EINVAL);
	dumpsize = rm5_sizes[dev & 07].nblocks;
	if ((dumplo < 0) || (dumplo >= dumpsize))
		return(EINVAL);
	dumpsize -= dumplo;

	/*
	 * reset the 9500
	 */
	siaddr->sicnr = SI_RESET;
	ubp = &UBMAP[0];
	for (paddr = 0L; dumpsize > 0; dumpsize -= count) {
		count = dumpsize>DBSIZE? DBSIZE: dumpsize;
		bn = dumplo + (paddr >> PGSHIFT);
		cn = (bn/(SI_NSECT*SI_NTRAC)) + rm5_sizes[minor(dev)&07].cyloff;
		sn = bn % (SI_NSECT * SI_NTRAC);
		tn = sn / SI_NSECT;
		sn = sn % SI_NSECT;
		if(dualsi)
			getsi();
		siaddr->sipcr = (unit << 10) | cn;
		siaddr->sihsr = (tn << 5) + sn;
		siaddr->siwcr = count << (PGSHIFT-1);

		if (ubmap) {
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			siaddr->simar = 0;
			siaddr->sicnr = SI_WRITE | SI_GO;
		}
		else
			{
			siaddr->simar = loint(paddr);
			siaddr->sicnr = SI_WRITE|SI_GO|((paddr >> 8)&(03 << 4));
		}
		while (!(siaddr->sicnr & SI_DONE))
			;
		if (siaddr->sierr & SIERR_ERR) {
			if (siaddr->sierr & SIERR_TIMO)
				return(0);	/* made it to end of memory */
			return(EIO);
		}
		paddr += (DBSIZE << PGSHIFT);
	}
	return(0);		/* filled disk minor dev */
}
#endif SI_DUMP

getsi()
{
	register struct sidevice *siaddr = SIADDR;

	while(!(siaddr->siscr & 0200))
	{
		siaddr->sicnr = SI_RESET;
		siaddr->siscr = 1;
	}
}

/*
 * Simple minded but effective. Likely none of these are still around or in use.
*/
daddr_t
sisize(dev)
	dev_t	dev;
	{

	return(rm5_sizes[dev & 07].nblocks);
	}
#endif NSI

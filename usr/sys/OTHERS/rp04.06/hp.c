/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)hp.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 *	This driver has been modified to perform bad-sector forwarding.
 *	It has not been tested after that modification.
 *	If you want to use it, install it instead of the normal hp driver,
 *	AND TEST BOTH BAD-SECTOR FORWARDING AND ECC CORRECTION.
 *	The driver does not know how to distinguish RP04's from RP05/6's;
 *	make sure that the definition of HP_NCYL is correct for all
 *	of your HP's, or add some way to distinguish (by drive
 *	number or something).
 */

/*
 *	RJP04/RWP04/RJP06/RWP06 disk driver
 */
#include "hp.h"
#if	NHP > 0
#include "param.h"
#include "../machine/seg.h"

#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "user.h"
#include "hpreg.h"
#include "uba.h"
#include "dkbad.h"
#include "dk.h"
#include "xp.h"

#define	HP_NCYL		815
#define	HP_NSECT	32
#define	HP_NTRAC	19
#define	HP_SDIST	2
#define	HP_RDIST	6

#if NXPD > 0
extern struct size {
	daddr_t	nblocks;
	int	cyloff;
} hp_sizes[8];
#else !NXPD
struct size {
	daddr_t	nblocks;
	int	cyloff;
} hp_sizes[8] = {
	9614,	  0,		/* cyl   0 - 22 */
	8778,	 23,		/* cyl  23 - 43 */
	153406,	 44,		/* cyl  44 - 410 (rp04, rp05, rp06) */
	168872,	411,		/* cyl 411 - 814 (rp06) */
	322278,	 44,		/* cyl  44 - 814 (rp06) */
	0,	  0,
	171798,	  0,		/* cyl   0 - 410 (whole rp04/5) */
	340670,	  0		/* cyl   0 - 814 (whole rp06) */
};
#endif NXPD

struct	hpdevice *HPADDR = (struct hpdevice *)0176700;

#ifdef	HPDEBUG
int	hpdebug = 1;
#endif

int	hp_offset[] =
{
	HPOF_P400,	HPOF_M400,	HPOF_P400,	HPOF_M400,
	HPOF_P800,	HPOF_M800,	HPOF_P800,	HPOF_M800,
	HPOF_P1200,	HPOF_M1200,	HPOF_P1200,	HPOF_M1200,
	0,		0,		0,		0
};

struct	buf	hptab;
struct	buf	hputab[NHP];
#ifdef BADSECT
struct	dkbad	hpbad[NHP];
struct	buf	bhpbuf[NHP];
bool_t	hp_init[NHP];
#endif

void
hproot()
{
	hpattach(HPADDR, 0);
}

hpattach(addr, unit)
register struct hpdevice *addr;
{
	if (unit != 0)
		return(0);
	if ((addr != (struct hpdevice *) NULL) && (fioword(addr) != -1)) {
		HPADDR = addr;
		if (fioword(&(addr->hpbae)) != -1)
			hptab.b_flags |= B_RH70;
		return(1);
	}
	HPADDR = (struct hpdevice *) NULL;
	return(0);
}

hpstrategy(bp)
register struct	buf *bp;
{
	register struct buf *dp;
	register unit;
	long	bn;

	unit = minor(bp->b_dev) & 077;
	if (unit >= (NHP << 3) || (HPADDR == (struct hpdevice *) NULL)) {
		bp->b_error = ENXIO;
		goto errexit;
	}
	if (bp->b_blkno < 0 ||
	    (bn = dkblock(bp)) + (long) ((bp->b_bcount + 511) >> 9)
	    > hp_sizes[unit & 07].nblocks) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if ((hptab.b_flags & B_RH70) == 0)
		mapalloc(bp);
	bp->b_cylin = bn / (HP_NSECT * HP_NTRAC) + hp_sizes[unit & 07].cyloff;
	unit = dkunit(bp);
	dp = &hputab[unit];
	(void) _spl5();
	disksort(dp, bp);
	if (dp->b_active == 0) {
		hpustart(unit);
		if (hptab.b_active == 0)
			hpstart();
	}
	(void) _spl0();
}

/*
 * Unit start routine.
 * Seek the drive to where the data are
 * and then generate another interrupt
 * to actually start the transfer.
 * If there is only one drive on the controller
 * or we are very close to the data, don't
 * bother with the search.  If called after
 * searching once, don't bother to look
 * where we are, just queue for transfer (to avoid
 * positioning forever without transferring).
 */
hpustart(unit)
register unit;
{
	register struct	hpdevice *hpaddr = HPADDR;
	register struct buf *dp;
	struct	buf *bp;
	daddr_t	bn;
	int sn, cn, csn;

	hpaddr->hpcs2.w = unit;
	hpaddr->hpcs1.c[0] = HP_IE;
	hpaddr->hpas = 1 << unit;

	if (unit >= NHP)
		return;
#ifdef	HP_DKN
	dk_busy &= ~(1 << (unit + HP_DKN));
#endif	HP_DKN
	dp = &hputab[unit];
	if ((bp = dp->b_actf) == NULL)
		return;
	/*
	 * If we have already positioned this drive,
	 * then just put it on the ready queue.
	 */
	if (dp->b_active)
		goto done;
	dp->b_active++;

	/*
	 * If drive has just come up,
	 * set up the pack.
	 */
#ifdef BADSECT
	if (((hpaddr->hpds & HPDS_VV) == 0) || (hp_init[unit] == 0))
#else
	if ((hpaddr->hpds & HPDS_VV) == 0)
#endif
	{
#ifdef BADSECT
		struct buf *bbp = &bhpbuf[unit];
		hp_init[unit] = 1;
#endif
		/* SHOULD WARN SYSTEM THAT THIS HAPPENED */
		hpaddr->hpcs1.c[0] = HP_IE | HP_PRESET | HP_GO;
		hpaddr->hpof = HPOF_FMT22;
#ifdef BADSECT
		bbp->b_flags = B_READ | B_BUSY | B_PHYS;
		bbp->b_dev = bp->b_dev;
		bbp->b_bcount = sizeof(struct dkbad);
		bbp->b_un.b_addr = (caddr_t)&hpbad[unit];
		bbp->b_blkno = (daddr_t)HP_NCYL * (HP_NSECT*HP_NTRAC)
		    - HP_NSECT;
		bbp->b_cylin = HP_NCYL - 1;
		if ((hptab.b_flags & B_RH70) == 0)
			mapalloc(bbp);
		dp->b_actf = bbp;
		bbp->av_forw = bp;
		bp = bbp;
#endif	BADSECT
	}

#if	NHP >	1
	/*
	 * If drive is offline, forget about positioning.
	 */
	if ((hpaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL))
		goto done;

	/*
	 * Figure out where this transfer is going to
	 * and see if we are close enough to justify not searching.
	 */
	bn = dkblock(bp);
	cn = bp->b_cylin;
	sn = bn % (HP_NSECT * HP_NTRAC);
	sn = (sn + HP_NSECT - HP_SDIST) % HP_NSECT;

	if (hpaddr->hpcc != cn)
		goto search;
	csn = (hpaddr->hpla >> 6) - sn + HP_SDIST - 1;
	if (csn < 0)
		csn += HP_NSECT;
	if (csn > HP_NSECT - HP_RDIST)
		goto done;

search:
	hpaddr->hpdc = cn;
	hpaddr->hpda = sn;
	hpaddr->hpcs1.c[0] = HP_IE | HP_SEARCH | HP_GO;
#ifdef	HP_DKN
	/*
	 * Mark unit busy for iostat.
	 */
	unit += HP_DKN;
	dk_busy |= 1 << unit;
	dk_numb[unit]++;
#endif	HP_DKN
	return;
#endif	NHP > 1

done:
	/*
	 * Device is ready to go.
	 * Put it on the ready queue for the controller.
	 */
	dp->b_forw = NULL;
	if (hptab.b_actf == NULL)
		hptab.b_actf = dp;
	else
		hptab.b_actl->b_forw = dp;
	hptab.b_actl = dp;
}

/*
 * Start up a transfer on a drive.
 */
hpstart()
{
	register struct hpdevice *hpaddr = HPADDR;
	register struct buf *bp;
	register unit;
	struct	buf *dp;
	daddr_t	bn;
	int	dn, sn, tn, cn;

loop:
	/*
	 * Pull a request off the controller queue.
	 */
	if ((dp = hptab.b_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		hptab.b_actf = dp->b_forw;
		goto loop;
	}
	/*
	 * Mark controller busy and
	 * determine destination of this request.
	 */
	hptab.b_active++;
	unit = minor(bp->b_dev) & 077;
	dn = dkunit(bp);
	bn = dkblock(bp);
	cn = bn / (HP_NSECT * HP_NTRAC) + hp_sizes[unit & 07].cyloff;
	sn = bn % (HP_NSECT * HP_NTRAC);
	tn = sn / HP_NSECT;
	sn = sn % HP_NSECT;

	/*
	 * Select drive.
	 */
	hpaddr->hpcs2.w = dn;
	/*
	 * Check that it is ready and online.
	 */
	if ((hpaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL)) {
		hptab.b_active = 0;
		hptab.b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		goto loop;
	}
	if (hptab.b_errcnt >= 16 && (bp->b_flags & B_READ)) {
		hpaddr->hpof = hp_offset[hptab.b_errcnt & 017] | HPOF_FMT22;
		hpaddr->hpcs1.w = HP_OFFSET | HP_GO;
		while ((hpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
			;
	}
	hpaddr->hpdc = cn;
	hpaddr->hpda = (tn << 8) + sn;
	hpaddr->hpba = bp->b_un.b_addr;
	if (hptab.b_flags & B_RH70)
		hpaddr->hpbae = bp->b_xmem;
	hpaddr->hpwc = -(bp->b_bcount >> 1);
	/*
	 * Warning:  unit is being used as a temporary.
	 */
	unit = ((bp->b_xmem & 3) << 8) | HP_IE | HP_GO;
#ifdef	HP_FORMAT
	if (minor(bp->b_dev) & 0200)
		unit |= bp->b_flags & B_READ? HP_RHDR : HP_WHDR;
	else
		unit |= bp->b_flags & B_READ? HP_RCOM : HP_WCOM;
#else
	if (bp->b_flags & B_READ)
		unit |= HP_RCOM;
	else
		unit |= HP_WCOM;
#endif
	hpaddr->hpcs1.w = unit;

#ifdef	HP_DKN
	dk_busy |= 1 << (HP_DKN + NHP);
	dk_numb[HP_DKN + NHP]++;
	dk_wds[HP_DKN + NHP] += bp->b_bcount >> 6;
#endif	HP_DKN
}

/*
 * Handle a disk interrupt.
 */
hpintr()
{
	register struct hpdevice *hpaddr = HPADDR;
	register struct buf *dp;
	register unit;
	struct	buf *bp;
	int	as, i, j;

	as = hpaddr->hpas & 0377;
	if (hptab.b_active) {
#ifdef	HP_DKN
		dk_busy &= ~(1 << (HP_DKN + NHP));
#endif	HP_DKN
		/*
		 * Get device and block structures.  Select the drive.
		 */
		dp = hptab.b_actf;
		bp = dp->b_actf;
#ifdef BADSECT
		if (bp->b_flags&B_BAD)
			if (hpecc(bp, CONT))
				return;
#endif
		unit = dkunit(bp);
		hpaddr->hpcs2.c[0] = unit;
		/*
		 * Check for and process errors.
		 */
		if (hpaddr->hpcs1.w & HP_TRE) {
#ifdef	HPDEBUG
			if (hpdebug) {
				printf("cs2=%b ds=%b er=%b\n",
				    hpaddr->hpcs2.w, HPCS2_BITS,
				    hpaddr->hpds, HPDS_BITS,
				    hpaddr->hper1, HPER1_BITS);
			}
#endif

			while ((hpaddr->hpds & HPDS_DRY) == 0)
				;
			if (hpaddr->hper1 & HPER1_WLE) {
				/*
				 *	Give up on write locked devices
				 *	immediately.
				 */
				printf("hp%d: write locked\n", unit);
				bp->b_flags |= B_ERROR;
#ifdef	BADSECT
			} else if (hpaddr->hper1 & HPER1_FER) {
#ifdef	HP_FORMAT
				/*
				 * Allow this error on format devices.
				 */
				if (minor(bp->b_dev) & 0200)
					goto errdone;
#endif
				if (hpecc(bp, BSE))
					return;
				else
					goto hard;
#endif	BADSECT
			} else {
				/*
				 * After 28 retries (16 without offset and
				 * 12 with offset positioning), give up.
				 */
hard:
				if (++hptab.b_errcnt > 28) {
				    bp->b_flags |= B_ERROR;
				    harderr(bp, "hp");
				    printf("cs2=%b er1=%b\n", hpaddr->hpcs2.w,
					HPCS2_BITS, hpaddr->hper1, HPER1_BITS);
				} else
				    hptab.b_active = 0;
			}
			/*
			 * If soft ecc, correct it (continuing
			 * by returning if necessary).
			 * Otherwise, fall through and retry the transfer.
			 */
			if((hpaddr->hper1 & (HPER1_DCK|HPER1_ECH)) == HPER1_DCK)
				if (hpecc(bp, ECC))
					return;
errdone:
			hpaddr->hpcs1.w = HP_TRE | HP_IE | HP_DCLR | HP_GO;
			if ((hptab.b_errcnt & 07) == 4) {
				hpaddr->hpcs1.w = HP_RECAL | HP_IE | HP_GO;
				while ((hpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
					;
			}
		}
		if (hptab.b_active) {
			if (hptab.b_errcnt) {
				hpaddr->hpcs1.w = HP_RTC | HP_GO;
				while ((hpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
					;
			}
			hptab.b_active = 0;
			hptab.b_errcnt = 0;
			hptab.b_actf = dp->b_forw;
			dp->b_active = 0;
			dp->b_actf = bp->av_forw;
			bp->b_resid = - (hpaddr->hpwc << 1);
			iodone(bp);
			hpaddr->hpcs1.w = HP_IE;
			if (dp->b_actf)
				hpustart(unit);
		}
		as &= ~(1 << unit);
	} else
		{
		if (as == 0)
			hpaddr->hpcs1.w = HP_IE;
		hpaddr->hpcs1.c[1] = HP_TRE >> 8;
	}
	for (unit = 0; unit < NHP; unit++)
		if (as & (1 << unit))
			hpustart(unit);
	hpstart();
}

#define	exadr(x,y)	(((long)(x) << 16) | (unsigned)(y))

/*
 * Correct an ECC error and restart the i/o to complete
 * the transfer if necessary.  This is quite complicated because
 * the correction may be going to an odd memory address base
 * and the transfer may cross a sector boundary.
 */
hpecc(bp, flag)
register struct	buf *bp;
{
	register struct hpdevice *hpaddr = HPADDR;
	register unsigned byte;
	ubadr_t	bb, addr;
	long	wrong;
	int	bit, wc;
	unsigned ndone, npx;
	int	ocmd;
	int	cn, tn, sn;
	daddr_t	bn;
	struct	ubmap *ubp;
	int	unit;

	/*
	 *	ndone is #bytes including the error
	 *	which is assumed to be in the last disk page transferred.
	 */
	unit = dkunit(bp);
#ifdef	BADSECT
	if (flag == CONT) {
		npx = bp->b_error;
		bp->b_error = 0;
		ndone = npx * NBPG;
		wc = ((int)(ndone - bp->b_bcount)) / NBPW;
	} else
#endif
	{
		wc = hpaddr->hpwc;
		ndone = (wc * NBPW) + bp->b_bcount;
		npx = ndone / NBPG;
	}
	ocmd = (hpaddr->hpcs1.w & ~HP_RDY) | HP_IE | HP_GO;
	bb = exadr(bp->b_xmem, bp->b_un.b_addr);
	bn = dkblock(bp);
	cn = bp->b_cylin - bn / (HP_NSECT * HP_NTRAC);
	bn += npx;
	cn += bn / (HP_NSECT * HP_NTRAC);
	sn = bn % (HP_NSECT * HP_NTRAC);
	tn = sn / HP_NSECT;
	sn %= HP_NSECT;

	switch (flag) {
	case ECC:
		printf("hp%d%c: soft ecc bn %D\n",
			unit, 'a' + (minor(bp->b_dev) & 07),
			bp->b_blkno + (npx - 1));
		wrong = hpaddr->hpec2;
		if (wrong == 0) {
			hpaddr->hpof = HPOF_FMT22;
			hpaddr->hpcs1.w |= HP_IE;
			return (0);
		}

		/*
		 *	Compute the byte/bit position of the err
		 *	within the last disk page transferred.
		 *	Hpec1 is origin-1.
		 */
		byte = hpaddr->hpec1 - 1;
		bit = byte & 07;
		byte >>= 3;
		byte += ndone - NBPG;
		wrong <<= bit;

		/*
		 *	Correct until mask is zero or until end of transfer,
		 *	whichever comes first.
		 */
		while (byte < bp->b_bcount && wrong != 0) {
			addr = bb + byte;
			if (bp->b_flags & (B_MAP|B_UBAREMAP)) {
				/*
				 * Simulate UNIBUS map if UNIBUS transfer.
				 */
				ubp = UBMAP + ((addr >> 13) & 037);
				addr = exadr(ubp->ub_hi, ubp->ub_lo)
				    + (addr & 017777);
			}
			putmemc(addr, getmemc(addr) ^ (int) wrong);
			byte++;
			wrong >>= 8;
		}
		break;
#ifdef BADSECT
	case BSE:
#ifdef	HPDEBUG
		if (hpdebug)
			printf("hpecc: BSE: bn %D cn %d tn %d sn %d\n",
				bn, cn, tn, sn);
#endif
		if ((bn = isbad(&hpbad[unit], cn, tn, sn)) < 0)
			return(0);
		bp->b_flags |= B_BAD;
		bp->b_error = npx + 1;
		bn = (daddr_t)HP_NCYL * (HP_NSECT * HP_NTRAC)
		    - HP_NSECT - 1 - bn;
		cn = bn/(HP_NSECT * HP_NTRAC);
		sn = bn%(HP_NSECT * HP_NTRAC);
		tn = sn/HP_NSECT;
		sn %= HP_NSECT;
#ifdef HPDEBUG
	if (hpdebug)
		printf("revector to cn %d tn %d sn %d\n", cn, tn, sn);
#endif
		wc = -(512 / NBPW);
		break;

	case CONT:
		bp->b_flags &= ~B_BAD;
#ifdef HPDEBUG
	if (hpdebug)
		printf("hpecc, CONT: bn %D cn %d tn %d sn %d\n", bn,cn,tn,sn);
#endif
		break;
#endif	BADSECT
	}

	hptab.b_active++;
	if (wc == 0)
		return (0);

	/*
	 * Have to continue the transfer.  Clear the drive
	 * and compute the position where the transfer is to continue.
	 * We have completed npx sectors of the transfer already.
	 */
	hpaddr->hpcs2.w = unit;
	hpaddr->hpcs1.w = HP_TRE | HP_DCLR | HP_GO;

	addr = bb + ndone;
	hpaddr->hpdc = cn;
	hpaddr->hpda = (tn << 8) + sn;
	hpaddr->hpwc = wc;
	hpaddr->hpba = (caddr_t)addr;
	if (hptab.b_flags & B_RH70)
		hpaddr->hpbae = (int) (addr >> 16);
	hpaddr->hpcs1.w = ocmd;
	return (1);
}

#ifdef HP_DUMP
/*
 *  Dump routine for RP04/05/06.
 *  Dumps from dumplo to end of memory/end of disk section for minor(dev).
 *  It uses the UNIBUS map to dump all of memory if there is a UNIBUS map
 *  and this isn't an RH70.
 */
#define DBSIZE	16			/* number of blocks to write */

hpdump(dev)
dev_t	dev;
{
	register struct hpdevice *hpaddr = HPADDR;
	daddr_t	bn, dumpsize;
	long	paddr;
	register sn;
	register count;
	long mem;
	extern size_t physmem;	/* number of clicks of real memory */
	register struct ubmap *ubp;

	mem = ((long)physmem) * ctob(1);	/* real memory in bytes */
	if ((bdevsw[major(dev)].d_strategy != hpstrategy)	/* paranoia */
	    || ((dev=minor(dev)) > (NHP << 3)))
		return(EINVAL);
	dumpsize = hp_sizes[dev & 07].nblocks;
	if ((dumplo < 0) || (dumplo >= dumpsize))
		return(EINVAL);
	dumpsize -= dumplo;

	hpaddr->hpcs2.w = dev >> 3;
	if ((hpaddr->hpds & HPDS_VV) == 0) {
		hpaddr->hpcs1.w = HP_DCLR | HP_GO;
		hpaddr->hpcs1.w = HP_PRESET | HP_GO;
		hpaddr->hpof = HPOF_FMT22;
	}
	if ((hpaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL))
		return(EFAULT);
	dev &= 07;
	ubp = &UBMAP[0];
	for (paddr = 0L; dumpsize > 0; dumpsize -= count) {
		count = dumpsize>DBSIZE? DBSIZE: dumpsize;
		bn = dumplo + (paddr >> PGSHIFT);
		hpaddr->hpdc= bn / (HP_NSECT*HP_NTRAC) + hp_sizes[dev].cyloff;
		sn = bn % (HP_NSECT * HP_NTRAC);
		hpaddr->hpda = ((sn / HP_NSECT) << 8) | (sn % HP_NSECT);
		hpaddr->hpwc = -(count << (PGSHIFT - 1));
		/*
		 *  If map exists, use
		 *  the map, unless on an 11/70 with RH70.
		 */
		if (ubmap && ((hptab.b_flags & B_RH70) == 0)) {
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			hpaddr->hpba = 0;
			hpaddr->hpcs1.w = HP_WCOM | HP_GO;
		}
		else
			{
			/*
			 *  Non-UNIBUS map, or 11/70 RH70 (MASSBUS)
			 */
			hpaddr->hpba = loint(paddr);
			if (hptab.b_flags & B_RH70)
				hpaddr->hpbae = hiint(paddr);
			hpaddr->hpcs1.w = HP_WCOM | HP_GO | ((paddr >> 8) & (03 << 8));
		}
		while (hpaddr->hpcs1.w & HP_GO)
			;
		if (hpaddr->hpcs1.w & HP_TRE) {
			if (hpaddr->hpcs2.w & HPCS2_NEM)
				return(0);	/* made it to end of memory */
			return(EIO);
		}
		paddr += (DBSIZE << PGSHIFT);
		if (paddr >= mem)
			return(0);
	}
	return(0);		/* filled disk minor dev */
}
#endif HP_DUMP
#endif NHP

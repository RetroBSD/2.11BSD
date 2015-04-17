/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rm.c	1.2 (2.11BSD GTE) 1/2/93
 */

/*
 * RJM02/RWM03, RM02/03/05 disk driver
 * For simplicity we use hpreg.h instead of an rmreg.h.  The bits
 * are the same.  Only the register names have been changed to
 * protect the innocent.
 *
 * Added RM_RM05 #ifdef (RM_NTRAC gets updated) for RM05 type drive.
 * Put this #define in the rm.h file.
 */

#include "rm.h"
#if	NRM > 0
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "user.h"
#include "hpreg.h"
#ifdef	RM_DUMP
#include "seg.h"
#endif
#include "uba.h"
#include "dkbad.h"
#include "dk.h"
#include "xp.h"

struct	hpdevice *RMADDR = (struct hpdevice *)0176700;

#if NXPD > 0
extern struct size {
	daddr_t	nblocks;
	int	cyloff;
} rm_sizes[8];
#else !NXPD
struct size {
	daddr_t	nblocks;
	int	cyloff;
} rm_sizes[8] = {
	4800,	  0,		/* cyl   0 -  29 */
	4800,	 30,		/* cyl  30 -  59 */
	122080,	 60,		/* cyl  60 - 822 */
	62720,	 60,		/* cyl  60 - 451 */
	59360,	452,		/* cyl 452 - 822 */
	0,	  0,		/* Not Defined */
	0,	  0,		/* Not Defined */
	131680,	  0,		/* cyl   0 - 822 */
};
#endif NXPD

int	rm_offset[] =
{
	HPOF_P400,	HPOF_M400,	HPOF_P400,	HPOF_M400,
	HPOF_P800,	HPOF_M800,	HPOF_P800,	HPOF_M800,
	HPOF_P1200,	HPOF_M1200,	HPOF_P1200,	HPOF_M1200,
	0,		0,		0,		0
};

#define	RM_NSECT	32
#ifdef	RM_RM05
#define	RM_NTRAC	19		/* RM05 */
#else
#define	RM_NTRAC	5		/* RM02/03 */
#endif
#define RM_NCYL		823
#define	RM_SDIST	2
#define	RM_RDIST	6

struct	buf	rmtab;
#if	NRM > 1
struct	buf	rmutab[NRM];
#endif
#ifdef BADSECT
struct	dkbad	rmbad[NRM];
struct	buf	brmbuf[NRM];
bool_t	rm_init[NRM];
#endif

#if	NRM > 1
int	rmcc[NRM]; 	/* Current cylinder */
#endif

void
rmroot()
{
	rmattach(RMADDR, 0);
}

rmattach(addr, unit)
register struct hpdevice *addr;
{
	if (unit != 0)
		return(0);
	if ((addr != (struct hpdevice *) NULL) && (fioword(addr) != -1)) {
		RMADDR = addr;
		if (fioword(&(addr->hpbae)) != -1)
			rmtab.b_flags |= B_RH70;
		return(1);
	}
	RMADDR = (struct hpdevice *) NULL;
	return(0);
}

rmstrategy(bp)
register struct	buf *bp;
{
	register struct buf *dp;
	register unit;
	long	bn;

	unit = minor(bp->b_dev) & 077;
	if (unit >= (NRM << 3) || (RMADDR == (struct hpdevice *) NULL)) {
		bp->b_error = ENXIO;
		goto errexit;
	}
	if (bp->b_blkno < 0 ||
	    (bn = dkblock(bp)) + (long) ((bp->b_bcount + 511) >> 9)
	    > rm_sizes[unit & 07].nblocks) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if ((rmtab.b_flags & B_RH70) == 0)
		mapalloc(bp);
	bp->b_cylin = bn / (RM_NSECT * RM_NTRAC) + rm_sizes[unit & 07].cyloff;
	unit = dkunit(bp);
#if	NRM > 1
	dp = &rmutab[unit];
#else
	dp = &rmtab;
#endif
	(void) _spl5();
	disksort(dp, bp);
	if (dp->b_active == 0) {
#if	NRM > 1
		rmustart(unit);
		if (rmtab.b_active == 0)
			rmstart();
#else
		rmstart();
#endif
	}
	(void) _spl0();
}

#if	NRM > 1
rmustart(unit)
register unit;
{
	register struct hpdevice *rmaddr = RMADDR;
	register struct	buf *dp;
	struct	buf *bp;
	daddr_t	bn;
	int	sn, cn, csn;

	rmaddr->hpcs2.w = unit;
	rmaddr->hpcs1.c[0] = HP_IE;
	rmaddr->hpas = 1 << unit;

	if (unit >= NRM)
		return;
#ifdef	RM_DKN
	dk_busy &= ~(1 << (unit + RM_DKN));
#endif
	dp = &rmutab[unit];
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
	 * If drive has just come up,
	 * set up the pack.
	 */
#ifdef BADSECT
	if (((rmaddr->hpds & HPDS_VV) == 0) || (rm_init[unit] == 0))
#else
	if ((rmaddr->hpds & HPDS_VV) == 0)
#endif
	{
#ifdef BADSECT
		struct buf *bbp = &brmbuf[unit];
		rm_init[unit] = 1;
#endif
		/* SHOULD WARN SYSTEM THAT THIS HAPPENED */
		rmaddr->hpcs1.c[0] = HP_IE | HP_PRESET | HP_GO;
		rmaddr->hpof = HPOF_FMT22;
#ifdef BADSECT
		bbp->b_flags = B_READ | B_BUSY | B_PHYS;
		bbp->b_dev = bp->b_dev;
		bbp->b_bcount = sizeof(struct dkbad);
		bbp->b_un.b_addr = (caddr_t)&rmbad[unit];
		bbp->b_blkno = (daddr_t)RM_NCYL * (RM_NSECT*RM_NTRAC)
		    - RM_NSECT;
		bbp->b_cylin = RM_NCYL - 1;
		if ((rmtab.b_flags & B_RH70) == 0)
			mapalloc(bbp);
		dp->b_actf = bbp;
		bbp->av_forw = bp;
		bp = bbp;
#endif	BADSECT
	}
	/*
	 * If drive is offline, forget about positioning.
	 */
	if ((rmaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL))
		goto done;

	bn = dkblock(bp);
	cn = bp->b_cylin;
	sn = bn % (RM_NSECT * RM_NTRAC);
	sn = (sn + RM_NSECT - RM_SDIST) % RM_NSECT;

	if (rmcc[unit] != cn)
		goto search;
	csn = (rmaddr->hpla >> 6) - sn + RM_SDIST - 1;
	if (csn < 0)
		csn += RM_NSECT;
	if (csn > RM_NSECT - RM_RDIST)
		goto done;

search:
	rmaddr->hpdc = cn;
	rmaddr->hpda = sn;
	rmaddr->hpcs1.c[0] = HP_IE | HP_SEARCH | HP_GO;
	rmcc[unit] = cn;
	/*
	 * Mark unit busy for iostat.
	 */
#ifdef	RM_DKN
	unit += RM_DKN;
	dk_busy |= 1 << unit;
	dk_numb[unit]++;
#endif
	return;

done:
	/*
	 * Device is ready to go.
	 * Put it on the ready queue for the controller.
	 */
	dp->b_forw = NULL;
	if (rmtab.b_actf == NULL)
		rmtab.b_actf = dp;
	else
		rmtab.b_actl->b_forw = dp;
	rmtab.b_actl = dp;
}
#endif

/*
 * Start up a transfer on a drive.
 */
rmstart()
{
	register struct hpdevice *rmaddr = RMADDR;
	register struct	buf *bp;
	struct buf *dp;
	register unit;
	daddr_t	bn;
	int	dn, sn, tn, cn;

loop:
	/*
	 * Pull a request off the controller queue.
	 */
#if	NRM == 1
	dp = &rmtab;
	if ((bp = dp->b_actf) == NULL)
		return;
#else
	if ((dp = rmtab.b_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		rmtab.b_actf = dp->b_forw;
		goto loop;
	}
#endif
	/*
	 * Mark controller busy and
	 * determine destination of this request.
	 */
	rmtab.b_active++;
	unit = minor(bp->b_dev) & 077;
	dn = dkunit(bp);

	/*
	 * Select drive.
	 */
	rmaddr->hpcs2.w = dn;
#if	NRM == 1
	/*
	 * If drive has just come up,
	 * set up the pack.
	 */
#ifdef BADSECT
	if (((rmaddr->hpds & HPDS_VV) == 0) || (rm_init[dn] == 0))
#else
	if ((rmaddr->hpds & HPDS_VV) == 0)
#endif
	{
#ifdef BADSECT
		struct buf *bbp = &brmbuf[dn];
		rm_init[dn] = 1;
#endif
		/* SHOULD WARN SYSTEM THAT THIS HAPPENED */
		rmaddr->hpcs1.c[0] = HP_IE | HP_PRESET | HP_GO;
		rmaddr->hpof = HPOF_FMT22;
#ifdef BADSECT
		bbp->b_flags = B_READ | B_BUSY | B_PHYS;
		bbp->b_dev = bp->b_dev;
		bbp->b_bcount = sizeof(struct dkbad);
		bbp->b_un.b_addr = (caddr_t)&rmbad[dn];
		bbp->b_blkno = (daddr_t)RM_NCYL * (RM_NSECT*RM_NTRAC)
		    - RM_NSECT;
		bbp->b_cylin = RM_NCYL - 1;
		if ((rmtab.b_flags & B_RH70) == 0)
			mapalloc(bbp);
		dp->b_actf = bbp;
		bbp->av_forw = bp;
		bp = bbp;
#endif	BADSECT
	}
#endif
	bn = dkblock(bp);
	cn = bp->b_cylin;
	sn = bn % (RM_NSECT * RM_NTRAC);
	tn = sn / RM_NSECT;
	sn = sn % RM_NSECT;
	/*
	 * Check that it is ready and online.
	 */
	if ((rmaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL)) {
		rmtab.b_active = 0;
		rmtab.b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		goto loop;
	}
	if (rmtab.b_errcnt >= 16 && (bp->b_flags & B_READ)) {
		rmaddr->hpof = rm_offset[rmtab.b_errcnt & 017] | HPOF_FMT22;
		rmaddr->hpcs1.w = HP_OFFSET | HP_GO;
		while ((rmaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
			;
	}
	rmaddr->hpdc = cn;
	rmaddr->hpda = (tn << 8) + sn;
	rmaddr->hpba = bp->b_un.b_addr;
	if (rmtab.b_flags & B_RH70)
		rmaddr->hpbae = bp->b_xmem;
	rmaddr->hpwc = -(bp->b_bcount >> 1);
	/*
	 * Warning:  unit is being used as a temporary.
	 */
	unit = ((bp->b_xmem & 3) << 8) | HP_IE | HP_GO;
#ifdef	RM_FORMAT
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
	rmaddr->hpcs1.w = unit;

#ifdef	RM_DKN
	dk_busy |= 1 << (RM_DKN + NRM);
	dk_numb[RM_DKN + NRM]++;
	dk_wds[RM_DKN + NRM] += bp->b_bcount >> 6;
#endif
}

/*
 * Handle a disk interrupt.
 */
rmintr()
{
	register struct hpdevice *rmaddr = RMADDR;
	register struct	buf *bp, *dp;
	short	unit;
	int	as, i, j;

	as = rmaddr->hpas & 0377;
	if (rmtab.b_active) {
#ifdef	RM_DKN
		dk_busy &= ~(1 << (RM_DKN + NRM));
#endif
		/*
		 * Get device and block structures.  Select the drive.
		 */
#if	NRM > 1
		dp = rmtab.b_actf;
#else
		dp = &rmtab;
#endif
		bp = dp->b_actf;
#ifdef BADSECT
		if (bp->b_flags&B_BAD)
			if (rmecc(bp, CONT))
				return;
#endif
		unit = dkunit(bp);
		rmaddr->hpcs2.c[0] = unit;
		/*
		 * Check for and process errors.
		 */
		if (rmaddr->hpcs1.w & HP_TRE) {		/* error bit */
			while ((rmaddr->hpds & HPDS_DRY) == 0)
				;
			if (rmaddr->hper1 & HPER1_WLE) {
				/*
				 *	Give up on write locked devices
				 *	immediately.
				 */
				printf("rm%d: write locked\n", unit);
				bp->b_flags |= B_ERROR;
#ifdef	BADSECT
			} else if (rmaddr->rmer2 & RMER2_BSE) {
#ifdef	RM_FORMAT
				/*
				 * Allow this error on format devices.
				 */
				if (minor(bp->b_dev) & 0200)
					goto errdone;
#endif
				if (rmecc(bp, BSE))
					return;
				else
					goto hard;
#endif	BADSECT
			} else {
				/*
				 * After 28 retries (16 without offset and
				 * 12 with offset positioning), give up.
				 */
				if (++rmtab.b_errcnt > 28) {
hard:
				    harderr(bp, "rm");
				    printf("cs2=%b er1=%b\n", rmaddr->hpcs2.w,
					HPCS2_BITS, rmaddr->hper1, HPER1_BITS);
				    bp->b_flags |= B_ERROR;
				} else
				    rmtab.b_active = 0;
			}
			/*
			 * If soft ecc, correct it (continuing
			 * by returning if necessary).
			 * Otherwise, fall through and retry the transfer.
			 */
			if((rmaddr->hper1 & (HPER1_DCK|HPER1_ECH)) == HPER1_DCK)
				if (rmecc(bp, ECC))
					return;
errdone:
			rmaddr->hpcs1.w = HP_TRE | HP_IE | HP_DCLR | HP_GO;
			if ((rmtab.b_errcnt & 07) == 4) {
				rmaddr->hpcs1.w = HP_RECAL | HP_IE | HP_GO;
				while ((rmaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
					;
			}
#if	NRM > 1
			rmcc[unit] = -1;
#endif
		}
		if (rmtab.b_active) {
			if (rmtab.b_errcnt) {
				rmaddr->hpcs1.w = HP_RTC | HP_GO;
				while ((rmaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
					;
			}
			rmtab.b_errcnt = 0;
#if	NRM > 1
			rmtab.b_active = 0;
			rmtab.b_actf = dp->b_forw;
			rmcc[unit] = bp->b_cylin;
#endif
			dp->b_active = 0;
			dp->b_actf = bp->b_actf;
			bp->b_resid = - (rmaddr->hpwc << 1);
			iodone(bp);
			rmaddr->hpcs1.w = HP_IE;
#if	NRM > 1
			if (dp->b_actf)
				rmustart(unit);
#endif
		}
#if	NRM > 1
		as &= ~(1 << unit);
#endif
	} else {
		if (as == 0)
			rmaddr->hpcs1.w	= HP_IE;
		rmaddr->hpcs1.c[1] = HP_TRE >> 8;
	}
#if	NRM > 1
	for(unit = 0; unit < NRM; unit++)
		if (as & (1 << unit))
			rmustart(unit);
#endif
	rmstart();
}

#define	exadr(x,y)	(((long)(x) << 16) | (unsigned)(y))

/*
 * Correct an ECC error and restart the i/o to complete
 * the transfer if necessary.  This is quite complicated because
 * the correction may be going to an odd memory address base
 * and the transfer may cross a sector boundary.
 */
rmecc(bp, flag)
register struct	buf *bp;
{
	register struct hpdevice *rmaddr = RMADDR;
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
		wc = rmaddr->hpwc;
		ndone = (wc * NBPW) + bp->b_bcount;
		npx = ndone / NBPG;
	}
	ocmd = (rmaddr->hpcs1.w & ~HP_RDY) | HP_IE | HP_GO;
	bb = exadr(bp->b_xmem, bp->b_un.b_addr);
	bn = dkblock(bp);
	cn = bp->b_cylin - bn / (RM_NSECT * RM_NTRAC);
	bn += npx;
	cn += bn / (RM_NSECT * RM_NTRAC);
	sn = bn % (RM_NSECT * RM_NTRAC);
	tn = sn / RM_NSECT;
	sn %= RM_NSECT;

	switch (flag) {
	case ECC:
		printf("rm%d%c: soft ecc bn %D\n",
			unit, 'a' + (minor(bp->b_dev) & 07),
			bp->b_blkno + (npx - 1));
		wrong = rmaddr->hpec2;
		if (wrong == 0) {
			rmaddr->hpof = HPOF_FMT22;
			rmaddr->hpcs1.w |= HP_IE;
			return (0);
		}

		/*
		 *	Compute the byte/bit position of the err
		 *	within the last disk page transferred.
		 *	Hpec1 is origin-1.
		 */
		byte = rmaddr->hpec1 - 1;
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
		if ((bn = isbad(&rmbad[unit], cn, tn, sn)) < 0)
			return(0);
		bp->b_flags |= B_BAD;
		bp->b_error = npx + 1;
		bn = (daddr_t)RM_NCYL * (RM_NSECT * RM_NTRAC)
		    - RM_NSECT - 1 - bn;
		cn = bn/(RM_NSECT * RM_NTRAC);
		sn = bn%(RM_NSECT * RM_NTRAC);
		tn = sn/RM_NSECT;
		sn %= RM_NSECT;
#ifdef DEBUG
		printf("revector to cn %d tn %d sn %d\n", cn, tn, sn);
#endif
		wc = -(512 / NBPW);
		break;

	case CONT:
		bp->b_flags &= ~B_BAD;
#ifdef DEBUG
		printf("rmecc CONT: bn %D cn %d tn %d sn %d\n", bn, cn, tn, sn);
#endif
		break;
#endif	BADSECT
	}

	rmtab.b_active++;
	if (wc == 0)
		return (0);

	/*
	 * Have to continue the transfer.  Clear the drive
	 * and compute the position where the transfer is to continue.
	 * We have completed npx sectors of the transfer already.
	 */
	rmaddr->hpcs2.w = unit;
	rmaddr->hpcs1.w = HP_TRE | HP_DCLR | HP_GO;

	addr = bb + ndone;
	rmaddr->hpdc = cn;
	rmaddr->hpda = (tn << 8) + sn;
	rmaddr->hpwc = wc;
	rmaddr->hpba = (caddr_t)addr;
	if (rmtab.b_flags & B_RH70)
		rmaddr->hpbae = (short)(addr >> 16);
	rmaddr->hpcs1.w = ocmd;
	return (1);
}

#ifdef RM_DUMP
/*
 *  Dump routine for RM02/RM03.
 *  Dumps from dumplo to end of memory/end of disk section for minor(dev).
 */

#define DBSIZE	16			/* number of blocks to write */

rmdump(dev)
dev_t	dev;
{
	register struct hpdevice *rmaddr = RMADDR;
	daddr_t	bn, dumpsize;
	long	paddr;
	register sn;
	register count;
	register struct ubmap *ubp;

	if ((bdevsw[major(dev)].d_strategy != rmstrategy)	/* paranoia */
	    || ((dev=minor(dev)) > (NRM << 3)))
		return(EINVAL);
	dumpsize = rm_sizes[dev & 07].nblocks;
	if ((dumplo < 0) || (dumplo >= dumpsize))
		return(EINVAL);
	dumpsize -= dumplo;

	rmaddr->hpcs2.w = dev >> 3;
	if ((rmaddr->hpds & HPDS_VV) == 0) {
		rmaddr->hpcs1.w = HP_DCLR | HP_GO;
		rmaddr->hpcs1.w = HP_PRESET | HP_GO;
		rmaddr->hpof = HPOF_FMT22;
	}
	if ((rmaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL))
		return(EFAULT);
	dev &= 07;
	ubp = &UBMAP[0];
	for (paddr = 0L; dumpsize > 0; dumpsize -= count) {
		count = dumpsize>DBSIZE? DBSIZE: dumpsize;
		bn = dumplo + (paddr >> PGSHIFT);
		rmaddr->hpdc = bn / (RM_NSECT*RM_NTRAC) + rm_sizes[dev].cyloff;
		sn = bn % (RM_NSECT * RM_NTRAC);
		rmaddr->hpda = ((sn / RM_NSECT) << 8) | (sn % RM_NSECT);
		rmaddr->hpwc = -(count << (PGSHIFT - 1));
		if (ubmap && ((rmtab.b_flags & B_RH70) == 0)) {
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			rmaddr->hpba = 0;
			rmaddr->hpcs1.w = HP_WCOM | HP_GO;
		}
		else
			{
			/*
			 *  Non-UNIBUS map, or 11/70 RM03 (MASSBUS)
			 */
			rmaddr->hpba = loint(paddr);
			if (rmtab.b_flags & B_RH70)
				rmaddr->hpbae = hiint(paddr);
			rmaddr->hpcs1.w = HP_WCOM | HP_GO | ((paddr >> 8) & (03 << 8));
		}
		while (rmaddr->hpcs1.w & HP_GO)
			;
		if (rmaddr->hpcs1.w & HP_TRE) {
			if (rmaddr->hpcs2.w & HPCS2_NEM)
				return(0);	/* made it to end of memory */
			return(EIO);
		}
		paddr += (DBSIZE << PGSHIFT);
	}
	return(0);		/* filled disk minor dev */
}
#endif	RM_DUMP
#endif	NRM

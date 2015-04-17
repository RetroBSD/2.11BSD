/*
 *	SCCS id	@(#)dvhp.c	2.2 (2.11BSD GTE) 1/2/93
 */

/*
 *	Disk driver for Diva Comp V controller.
 */

#include "dvhp.h"

#if	NDVHP > 0
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "user.h"
#include "hpreg.h"

#ifndef INTRLVE
#include "inline.h"
#endif !INTRLVE

#include "uba.h"

#define	DVHP_NSECT	33
#define	DVHP_NTRAC	19
#define	DVHP_SDIST	2
#define	DVHP_RDIST	6

extern	struct	size dv_sizes[];
extern	struct	hpdevice *DVHPADDR;

int	dvhp_offset[] =
{
	HPOF_P400,	HPOF_M400,	HPOF_P400,	HPOF_M400,
	HPOF_P800,	HPOF_M800,	HPOF_P800,	HPOF_M800,
	HPOF_P1200,	HPOF_M1200,	HPOF_P1200,	HPOF_M1200,
	0,		0,		0,		0
};

struct	buf	dvhptab;
struct	buf	dvhputab[NDVHP];

#ifdef	INTRLVE
extern	daddr_t	dkblock();
#endif

void
dvhproot()
{
	dvhpattach(DVHPADDR, 0);
}

dvhpattach(addr, unit)
register struct hpdevice *addr;
{
	if (unit != 0)
		return(0);
	if ((addr != (struct hpdevice *) NULL) && (fioword(addr) != -1)) {
		DVHPADDR = addr;
		if (fioword(&(addr->hpbae)) != -1)
			dvhptab.b_flags |= B_RH70;
		return(1);
	}
	DVHPADDR = (struct hpdevice *) NULL;
	return(0);
}

dvhpstrategy(bp)
register struct buf *bp;
{
	register struct buf *dp;
	register unit;
	long bn;

	unit = minor(bp->b_dev) & 077;
	if (unit >= (NDVHP << 3) || (DVHPADDR == (struct hpdevice *) NULL)) {
		bp->b_error = ENXIO;
		goto errexit;
	}
	if (bp->b_blkno < 0 ||
	    (bn = dkblock(bp)) + (long) ((bp->b_bcount + 511) >> 9)
	    > dv_sizes[unit & 07].nblocks) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if ((dvhptab.b_flags & B_RH70) == 0)
		mapalloc(bp);
	bp->b_cylin = bn / (DVHP_NSECT * DVHP_NTRAC) + dv_sizes[unit & 07].cyloff;
	unit = dkunit(bp);
	dp = &dvhputab[unit];
	(void) _spl5();
	disksort(dp, bp);
	if (dp->b_active == 0) {
		dvhpustart(unit);
		if (dvhptab.b_active == 0)
			dvhpstart();
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
dvhpustart(unit)
register unit;
{
	register struct hpdevice *dvhpaddr = DVHPADDR;
	register struct buf *dp;
	struct	buf *bp;
	daddr_t	bn;
	int	sn, cn, csn;

	dvhpaddr->hpcs2.w = unit;
	dvhpaddr->hpcs1.c[0] = HP_IE;
	dvhpaddr->hpas = 1 << unit;

	if (unit >= NDVHP)
		return;
#ifdef	DVHP_DKN
	dk_busy &= ~(1 << (unit + DVHP_DKN));
#endif
	dp = &dvhputab[unit];
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
	if ((dvhpaddr->hpds & HPDS_VV) == 0) {
		/* SHOULD WARN SYSTEM THAT THIS HAPPENED */
		dvhpaddr->hpcs1.c[0] = HP_IE | HP_PRESET | HP_GO;
		dvhpaddr->hpof = HPOF_FMT22;
	}
#if	NDVHP > 1
	/*
	 * If drive is offline, forget about positioning.
	 */
	if ((dvhpaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL))
		goto done;
	/*
	 * Not on cylinder at correct position:  seek
	 */
	bn = dkblock(bp);
	cn = bp->b_cylin;
	sn = bn % (DVHP_NSECT * DVHP_NTRAC);
	sn = (sn + DVHP_NSECT - DVHP_SDIST) % DVHP_NSECT;

	if (dvhpaddr->hpcc != cn) {
		dvhpaddr->hpdc = cn;
		dvhpaddr->hpcs1.c[0] = HP_IE | HP_SEEK | HP_GO;
#ifdef	DVHP_DKN
		/*
		 * Mark unit busy for iostat.
		 */
		unit += DVHP_DKN;
		dk_busy	|= 1 << unit;
		dk_numb[unit]++;
#endif
		return;
	}
#endif	NDVHP > 1

done:
	/*
	 * Device is ready to go.
	 * Put it on the ready queue for the controller.
	 */
	dp->b_forw = NULL;
	if (dvhptab.b_actf == NULL)
		dvhptab.b_actf = dp;
	else
		dvhptab.b_actl->b_forw = dp;
	dvhptab.b_actl = dp;
}

/*
 * Start up a transfer on a drive.
 */
dvhpstart()
{
	register struct hpdevice *dvhpaddr = DVHPADDR;
	register struct buf *bp;
	struct	buf *dp;
	register unit;
	daddr_t bn;
	int	dn, sn, tn, cn;

loop:
	/*
	 * Pull a request off the controller queue.
	 */
	if ((dp = dvhptab.b_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		dvhptab.b_actf = dp->b_forw;
		goto loop;
	}
	/*
	 * Mark controller busy and
	 * determine destination of this request.
	 */
	dvhptab.b_active++;
	unit = minor(bp->b_dev) & 077;
	dn = dkunit(bp);
	bn = dkblock(bp);
	cn = bn / (DVHP_NSECT * DVHP_NTRAC) + dv_sizes[unit & 07].cyloff;
	sn = bn % (DVHP_NSECT * DVHP_NTRAC);
	tn = sn / DVHP_NSECT;
	sn = sn % DVHP_NSECT;

	/*
	 * Select drive.
	 */
	dvhpaddr->hpcs2.w = dn;

	/*
	 * Check that it is ready and online.
	 */
	if ((dvhpaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL)) {
		dvhptab.b_active = 0;
		dvhptab.b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		goto loop;
	}
	if (dvhptab.b_errcnt >= 16 && (bp->b_flags & B_READ)) {
		dvhpaddr->hpof = dvhp_offset[dvhptab.b_errcnt & 017] | HPOF_FMT22;
		dvhpaddr->hpcs1.w = HP_OFFSET | HP_GO;
		while ((dvhpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
			;
	}
	dvhpaddr->hpdc = cn;
	dvhpaddr->hpda = (tn << 8) + sn;
	dvhpaddr->hpba = bp->b_un.b_addr;
	if (dvhptab.b_flags & B_RH70)
		dvhpaddr->hpbae = bp->b_xmem;
	dvhpaddr->hpwc = -(bp->b_bcount >> 1);
	/*
	 * Warning:  unit is being used as a temporary.
	 */
	unit = ((bp->b_xmem & 3) << 8) | HP_IE | HP_GO;
	if (bp->b_flags & B_READ)
		unit |= HP_RCOM;
	else
		unit |= HP_WCOM;
	dvhpaddr->hpcs1.w = unit;

#ifdef	DVHP_DKN
	dk_busy	|= 1 << (DVHP_DKN + NDVHP);
	dk_numb[DVHP_DKN + NDVHP]++;
	dk_wds[DVHP_DKN + NDVHP] += bp->b_bcount >> 6;
#endif	DVHP_DKN
}

/*
 * Handle a disk interrupt.
 */
dvhpintr()
{
	register struct hpdevice *dvhpaddr = DVHPADDR;
	register struct buf *dp;
	struct	buf *bp;
	register unit;
	int as, i, j;

	as = dvhpaddr->hpas & 0377;
	if (dvhptab.b_active) {
#ifdef	DVHP_DKN
		dk_busy &= ~(1 << (DVHP_DKN + NDVHP));
#endif	DVHP_DKN
		/*
		 * Get device and block structures.  Select the drive.
		 */
		dp = dvhptab.b_actf;
		bp = dp->b_actf;
		unit = dkunit(bp);
		dvhpaddr->hpcs2.c[0] = unit;
		/*
		 * Check for and process errors.
		 */
		if (dvhpaddr->hpcs1.w & HP_TRE) {		/* error bit */
			while ((dvhpaddr->hpds & HPDS_DRY) == 0)
				;
			if (dvhpaddr->hper1 & HPER1_WLE) {
				/*
				 *	Give up on write locked devices
				 *	immediately.
				 */
				printf("dvhp%d: write locked\n", unit);
				bp->b_flags |= B_ERROR;
			} else {
				/*
				 * After 28 retries (16 without offset and
				 * 12 with offset positioning), give up.
				 */
				if (++dvhptab.b_errcnt > 28) {
					bp->b_flags |= B_ERROR;
#ifdef	UCB_DEVERR
					harderr(bp, "dvhp");
					printf("cs2=%b er1=%b\n",
					    dvhpaddr->hpcs2.w, HPCS2_BITS,
					    dvhpaddr->hper1, HPER1_BITS);
#else
					deverror(bp, dvhpaddr->hpcs2.w,
					    dvhpaddr->hper1);
#endif
				} else
					dvhptab.b_active = 0;
			}
#ifdef	UCB_ECC
			/*
			 * If soft ecc, correct it (continuing
			 * by returning if necessary).
			 * Otherwise, fall through and retry the transfer.
			 */
			if ((dvhpaddr->hper1 & (HPER1_DCK | HPER1_ECH)) == HPER1_DCK)
				if (dvhpecc(bp))
					return;
#endif
			dvhpaddr->hpcs1.w = HP_TRE | HP_IE | HP_DCLR | HP_GO;
			if ((dvhptab.b_errcnt & 07) == 4) {
				dvhpaddr->hpcs1.w = HP_RECAL | HP_IE | HP_GO;
				while ((dvhpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
					;
			}
		}
		if (dvhptab.b_active) {
			if (dvhptab.b_errcnt) {
				dvhpaddr->hpcs1.w = HP_RTC | HP_GO;
				while ((dvhpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
					;
			}
			dvhptab.b_active = 0;
			dvhptab.b_errcnt = 0;
			dvhptab.b_actf = dp->b_forw;
			dp->b_active = 0;
			dp->b_actf = bp->av_forw;
			bp->b_resid = -(dvhpaddr->hpwc << 1);
			iodone(bp);
			dvhpaddr->hpcs1.w = HP_IE;
			if (dp->b_actf)
				dvhpustart(unit);
		}
		as &= ~(1 << unit);
	} else
		{
		if (as == 0)
			dvhpaddr->hpcs1.w = HP_IE;
		dvhpaddr->hpcs1.c[1] = HP_TRE >> 8;
	}
	for (unit = 0; unit < NDVHP; unit++)
		if (as & (1 << unit))
			dvhpustart(unit);
	dvhpstart();
}

#ifdef	UCB_ECC
#define	exadr(x,y)	(((long)(x) << 16) | (unsigned)(y))

/*
 * Correct an ECC error and restart the i/o to complete
 * the transfer if necessary.  This is quite complicated because
 * the transfer may be going to an odd memory address base
 * and/or across a page boundary.
 */
dvhpecc(bp)
register struct	buf *bp;
{
	register struct	hpdevice *dvhpaddr = DVHPADDR;
	register unsigned byte;
	ubadr_t	bb, addr;
	long	wrong;
	int	bit, wc;
	unsigned ndone, npx;
	int	ocmd;
	int	cn, tn, sn;
	daddr_t	bn;
	struct	ubmap *ubp;

	/*
	 *	ndone is #bytes including the error
	 *	which is assumed to be in the last disk page transferred.
	 */
	wc = dvhpaddr->hpwc;
	ndone = (wc * NBPW) + bp->b_bcount;
	npx = ndone / PGSIZE;
	printf("dvhp%d%c: soft ecc bn %D\n",
		dkunit(bp), 'a' + (minor(bp->b_dev) & 07),
		bp->b_blkno + (npx - 1));
	wrong = dvhpaddr->hpec2;
	if(wrong == 0) {
		dvhpaddr->hpof = HPOF_FMT22;
		dvhpaddr->hpcs1.w |= HP_IE;
		return (0);
	}

	/*
	 *	Compute the byte/bit position of the err
	 *	within the last disk page transferred.
	 *	Hpec1 is origin-1.
	 */
	byte = dvhpaddr->hpec1 - 1;
	bit = byte & 07;
	byte >>= 3;
	byte += ndone - PGSIZE;
	bb = exadr(bp->b_xmem, bp->b_un.b_addr);
	wrong <<= bit;

	/*
	 *	Correct until mask is zero or until end of transfer,
	 *	whichever comes first.
	 */
	while (byte < bp->b_bcount && wrong != 0) {
		addr = bb + byte;
		if (bp->b_flags & (B_MAP | B_UBAREMAP)) {
			/*
			 * Simulate UNIBUS map if UNIBUS transfer.
			 */
			ubp = UBMAP + ((addr >> 13) & 037);
			addr = exadr(ubp->ub_hi, ubp->ub_lo) + (addr & 017777);
		}
		putmemc(addr, getmemc(addr) ^ (int) wrong);
		byte++;
		wrong >>= 8;
	}

	dvhptab.b_active++;
	if (wc == 0)
		return (0);

	/*
	 * Have to continue the transfer.  Clear the drive
	 * and compute the position where the transfer is to continue.
	 * We have completed npx sectors of the transfer already.
	 */
	ocmd = (dvhpaddr->hpcs1.w & ~HP_RDY) | HP_IE | HP_GO;
	dvhpaddr->hpcs2.w = dkunit(bp);
	dvhpaddr->hpcs1.w = HP_TRE | HP_DCLR | HP_GO;

	bn = dkblock(bp);
	cn = bp->b_cylin - bn / (DVHP_NSECT * DVHP_NTRAC);
	bn += npx;
	addr = bb + ndone;

	cn += bn / (DVHP_NSECT * DVHP_NTRAC);
	sn = bn % (DVHP_NSECT * DVHP_NTRAC);
	tn = sn / DVHP_NSECT;
	sn %= DVHP_NSECT;

	dvhpaddr->hpdc = cn;
	dvhpaddr->hpda = (tn << 8) + sn;
	dvhpaddr->hpwc = ((int)(ndone - bp->b_bcount)) / NBPW;
	dvhpaddr->hpba = (int) addr;
	if (dvhptab.b_flags & B_RH70)
		dvhpaddr->hpbae = (int) (addr >> 16);
	dvhpaddr->hpcs1.w = ocmd;
	return (1);
}
#endif	UCB_ECC
#endif	NDVHP

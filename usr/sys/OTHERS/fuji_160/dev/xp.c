/*
 *	SCCS id	@(#)xp.c	2.1 (Berkeley)	8/23/83
 */

/*
 *	RM02/03/05, RP04/05/06, DIVA disk driver.
 *	Fuji 160 added 11/2/84. r2 megatest
 *	SI Eagle added 9/20/83.
 *	This driver will handle most variants of SMD drives
 *	on one or more controllers.
 *	If XP_PROBE is defined, it includes a probe routine
 *	that will determine the number and type of drives attached
 *	to each controller; otherwise, the data structures must be
 *	initialized.
 *
 *	For simplicity we use hpreg.h instead of an xpreg.h.  The bits
 *	are the same.
 */

#include "xp.h"
#if	NXP > 0
#include "param.h"
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/seg.h>
#include <sys/hpreg.h>
#ifndef	INTRLVE
#include <sys/inline.h>
#endif
#include <sys/uba.h>

#define	XP_SDIST	2
#define	XP_RDIST	6

int	xp_offset[] =
{
	HPOF_P400,	HPOF_M400,	HPOF_P400,	HPOF_M400,
	HPOF_P800,	HPOF_M800,	HPOF_P800,	HPOF_M800,
	HPOF_P1200,	HPOF_M1200,	HPOF_P1200,	HPOF_M1200,
	0,		0,		0,		0
};

/*
 *  Xp_drive and xp_controller may be initialized in ioconf.c,
 *  or they may be filled in at boot time if XP_PROBE is enabled.
 *  Xp_controller address fields must be initialized for any boot devices,
 *  however.
 */
struct	xp_drive xp_drive[NXP];
struct	xp_controller xp_controller[NXP_CONTROLLER];

struct	buf	xptab;
struct	buf	xputab[NXP];

#ifdef	INTRLVE
extern	daddr_t	dkblock();
#endif

/*
 *  Attach controllers whose addresses are known at boot time.
 *  Stop at the first not found, so that the drive numbering
 *  won't get confused.
 */
xproot()
{
	register i;
	register struct hpdevice *xpaddr;

	for (i = 0; i < NXP_CONTROLLER; i++)
		if (((xpaddr = xp_controller[i].xp_addr) == 0)
		    || (xpattach(xpaddr, i) == 0))
			break;
}

/*
 *  Attach controller at xpaddr.
 *  Mark as nonexistent if xpaddr is 0;
 *  otherwise attach slaves if probing.
 *  NOTE: if probing for drives, this routine must be called
 *  once per controller, in ascending controller numbers.
 */
xpattach(xpaddr, unit)
register struct hpdevice *xpaddr;
{
	register struct xp_controller *xc = &xp_controller[unit];
#ifdef	XP_PROBE
	static int last_attached = -1;
#endif

	if ((unsigned) unit >= NXP_CONTROLLER)
		return(0);
	if ((xpaddr != 0) && (fioword(xpaddr) != -1)) {
		xc->xp_addr = xpaddr;
		if (fioword(&(xpaddr->hpbae)) != -1)
			xc->xp_flags |= XP_RH70;
#ifdef	XP_PROBE
		/*
		 *  If already attached, ignore (don't want to renumber drives)
		 */
		if (unit > last_attached) {
			last_attached = unit;
			xpslave(xpaddr, xc);
		}
#endif
		return(1);
	}
	xc->xp_addr = 0;
	return(0);
}

#ifdef	XP_PROBE
/*
 *  Determine what drives are attached to a controller;
 *  guess their types and fill in the drive structures.
 */
xpslave(xpaddr, xc)
register struct hpdevice *xpaddr;
struct xp_controller *xc;
{
	register struct xp_drive *xd;
	int j,  dummy;
	static int nxp = 0;
#ifdef MEGATEST
	extern	struct size dv_sizes[],hp_sizes[],rm_sizes[],rm5_sizes[],si_sizes[],rm2x_sizes[];
#else
	extern	struct size dv_sizes[],hp_sizes[],rm_sizes[],rm5_sizes[],si_sizes[];
#endif

	for (j = 0; j < 8; j++) {
		xpaddr->hpcs1.w = 0;
		xpaddr->hpcs2.w = j;
		dummy = xpaddr->hpds;
		if (xpaddr->hpcs2.w & HPCS2_NED) {
			xpaddr->hpcs2.w = HPCS2_CLR;
			continue;
		}
		if (nxp < NXP) {
			xd = &xp_drive[nxp++];
			xd->xp_ctlr = xc;
			xd->xp_unit = j;
			/*
			 * If drive type is initialized,
			 * believe it.
			 */
			if (xd->xp_type == 0)
				xd->xp_type = xpaddr->hpdt & 077;
			switch (xd->xp_type) {

			case RM02:
			case RM03:
				xd->xp_nsect = RM_SECT;
				xd->xp_ntrack = RM_TRAC;
				xd->xp_nspc = RM_SECT * RM_TRAC;
				xd->xp_sizes = &rm_sizes;
				xd->xp_ctlr->xp_flags |= XP_NOCC;
				break;
#ifdef MEGATEST
			case RM2X:
				xd->xp_nsect = RM2X_SECT;
				xd->xp_ntrack = RM2X_TRAC;
				xd->xp_nspc = RM2X_SECT * RM2X_TRAC;
				xd->xp_sizes = &rm2x_sizes;
				xd->xp_ctlr->xp_flags |= XP_NOCC;
				break;
#endif MEGATEST
			case RM05:
			case RM5X:
				if ((xpaddr->hpsn & SI_SN_MSK) == SI_SN_DT) {
				    xd->xp_nsect = SI_SECT;
				    xd->xp_ntrack = SI_TRAC;
				    xd->xp_nspc = SI_SECT * SI_TRAC;
				    xd->xp_sizes = &si_sizes;
				    xd->xp_ctlr->xp_flags |= XP_NOCC;
				} else {
				    xd->xp_nsect = RM5_SECT;
				    xd->xp_ntrack = RM5_TRAC;
				    xd->xp_nspc = RM5_SECT * RM5_TRAC;
				    xd->xp_sizes = &rm5_sizes;
				    xd->xp_ctlr->xp_flags |= XP_NOCC;
				}
				break;

			case RP:
				xd->xp_nsect = HP_SECT;
				xd->xp_ntrack = HP_TRAC;
				xd->xp_nspc = HP_SECT * HP_TRAC;
				xd->xp_sizes = &hp_sizes;
				break;

			case DV:
				xd->xp_nsect = DV_SECT;
				xd->xp_ntrack = DV_TRAC;
				xd->xp_nspc = DV_SECT * DV_TRAC;
				xd->xp_sizes = &dv_sizes;
				xd->xp_ctlr->xp_flags |= XP_NOSEARCH;
				break;

			default:
			    printf("xp%d: drive type %o unrecognized\n",
				nxp - 1, xd->xp_type);
				xd->xp_ctlr = NULL;
				break;
			}
		}
	}
}
#endif	XP_PROBE

xpstrategy(bp)
register struct buf *bp;
{
	register struct xp_drive *xd;
	register unit;
	struct buf *dp;
	short	pseudo_unit;
	int	s;
	long	bn;

	unit = dkunit(bp);
	pseudo_unit = minor(bp->b_dev) & 07;

	if ((unit >= NXP) || ((xd = &xp_drive[unit])->xp_ctlr == 0) ||
	   (xd->xp_ctlr->xp_addr == 0)) { 
		bp->b_error = ENXIO;
		goto errexit;
	}
	if ((bp->b_blkno < 0) ||
	   ((bn = dkblock(bp)) + ((bp->b_bcount + 511) >> 9)
	     > xd->xp_sizes[pseudo_unit].nblocks)) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if ((xd->xp_ctlr->xp_flags & XP_RH70) == 0)
		mapalloc(bp);
#ifdef EXT_LSI
	if(bp->b_flags & B_PHYS)
		if(Sofub_alloc(bp) == 0)
			return;
#endif
	bp->b_cylin = bn / xd->xp_nspc
		+ xd->xp_sizes[pseudo_unit].cyloff;
	dp = &xputab[unit];
	s = spl5();
	disksort(dp, bp);
	if (dp->b_active == 0) {
		xpustart(unit);
		if (xd->xp_ctlr->xp_active == 0)
			xpstart(xd->xp_ctlr);
	}
	splx(s);
}

/*
 * Unit start routine.
 * Seek the drive to where the data are
 * and then generate another interrupt
 * to actually start the transfer.
 * If there is only one drive
 * or we are very close to the data, don't
 * bother with the search.  If called after
 * searching once, don't bother to look
 * where we are, just queue for transfer (to avoid
 * positioning forever without transferring).
 */
xpustart(unit)
int unit;
{
	register struct xp_drive *xd;
	register struct hpdevice *xpaddr;
	register struct buf *dp;
	struct	buf *bp;
	daddr_t	bn;
	int	sn, cn, csn;

	xd = &xp_drive[unit];
	xpaddr = xd->xp_ctlr->xp_addr;
	xpaddr->hpcs2.w = xd->xp_unit;
	xpaddr->hpcs1.c[0] = HP_IE;
	xpaddr->hpas = 1 << xd->xp_unit;

	if (unit >= NXP)
		return;
#ifdef	XP_DKN
	dk_busy &= ~(1 << (unit + XP_DKN));
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
	 * If drive has just come up,
	 * set up the pack.
	 */
	if ((xpaddr->hpds & HPDS_VV) == 0) {
		/* SHOULD WARN SYSTEM THAT THIS HAPPENED */
		xpaddr->hpcs1.c[0] = HP_IE | HP_PRESET | HP_GO;
		xpaddr->hpof = HPOF_FMT22;
	}
#if	NXP > 1
	/*
	 * If drive is offline, forget about positioning.
	 */
	if ((xpaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL))
		goto done;

	/*
	 * Figure out where this transfer is going to
	 * and see if we are close enough to justify not searching.
	 */
	bn = dkblock(bp);
	cn = bp->b_cylin;
	sn = bn % xd->xp_nspc;
	sn += xd->xp_nsect - XP_SDIST;
	sn %= xd->xp_nsect;

	if (((xd->xp_ctlr->xp_flags & XP_NOCC) && (xd->xp_cc != cn))
	    || xpaddr->hpcc != cn)
		goto search;
	if (xd->xp_ctlr->xp_flags & XP_NOSEARCH)
		goto done;
	csn = (xpaddr->hpla >> 6) - sn + XP_SDIST - 1;
	if (csn < 0)
		csn += xd->xp_nsect;
	if (csn > xd->xp_nsect - XP_RDIST)
		goto done;

search:
	xpaddr->hpdc = cn;
	xpaddr->hpda = sn;
	xpaddr->hpcs1.c[0] = (xd->xp_ctlr->xp_flags & XP_NOSEARCH)?
		(HP_IE | HP_SEEK | HP_GO) : (HP_IE | HP_SEARCH | HP_GO);
	xd->xp_cc = cn;
#ifdef	XP_DKN
	/*
	 * Mark unit busy for iostat.
	 */
	unit += XP_DKN;
	dk_busy |= 1 << unit;
	dk_numb[unit]++;
#endif	XP_DKN
	return;
#endif	NXP > 1

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
	struct	buf *dp;
	int	unit;
	short	pseudo_unit;
	daddr_t	bn;
	int	sn, tn, cn;

	xpaddr = xc->xp_addr;
loop:
	/*
	 * Pull a request off the controller queue.
	 */
	if ((dp = xc->xp_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		xc->xp_actf = dp->b_forw;
		goto loop;
	}
	/*
	 * Mark controller busy and
	 * determine destination of this request.
	 */
	xc->xp_active++;
	pseudo_unit = minor(bp->b_dev) & 07;
	unit = dkunit(bp);
	xd = &xp_drive[unit];
	bn = dkblock(bp);
	cn = xd->xp_sizes[pseudo_unit].cyloff;
	cn += bn / xd->xp_nspc;
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
	if ((xpaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL)) {
		xc->xp_active = 0;
		dp->b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		goto loop;
	}
	if (dp->b_errcnt >= 16 && (bp->b_flags & B_READ)) {
		xpaddr->hpof = xp_offset[dp->b_errcnt & 017] | HPOF_FMT22;
		xpaddr->hpcs1.w = HP_OFFSET | HP_GO;
		while ((xpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
			;
	}
	xpaddr->hpdc = cn;
	xpaddr->hpda = (tn << 8) + sn;
	xpaddr->hpba = bp->b_un.b_addr;
	if (xc->xp_flags & XP_RH70)
		xpaddr->hpbae = bp->b_xmem;
	xpaddr->hpwc = -(bp->b_bcount >> 1);

	/*
	 * Warning:  unit is being used as a temporary.
	 */
	unit = ((bp->b_xmem & 3) << 8) | HP_IE | HP_GO;
	if (bp->b_flags & B_READ)
		unit |= HP_RCOM;
	else
		unit |= HP_WCOM;
	xpaddr->hpcs1.w = unit;

#ifdef	XP_DKN
	unit = xc - &xp_controller[0] + XP_DKN + NXP;
	dk_busy |= 1 << unit;
	dk_numb[unit]++;
	dk_wds[unit] += bp->b_bcount >> 6;
#endif
}

/*
 * Handle a disk interrupt.
 */
xpintr(dev)
int dev;
{
	register struct	hpdevice *xpaddr;
	register struct buf *dp;
	struct xp_controller *xc;
	struct xp_drive *xd;
	struct	buf *bp;
	register unit;
	int	as, i, j;

	xc = &xp_controller[dev];
	xpaddr = xc->xp_addr;
	as = xpaddr->hpas & 0377;
	if (xc->xp_active) {
#ifdef	XP_DKN
		dk_busy &= ~(1 << (dev + XP_DKN + NXP));
#endif
		/*
		 * Get device and block structures.  Select the drive.
		 */
		dp = xc->xp_actf;
		bp = dp->b_actf;
		unit = dkunit(bp);
		xd = &xp_drive[unit];
		xpaddr->hpcs2.c[0] = xd->xp_unit;
		/*
		 * Check for and process errors.
		 */
		if (xpaddr->hpcs1.w & HP_TRE) {
			while ((xpaddr->hpds & HPDS_DRY) == 0)
				;
			if (xpaddr->hper1 & HPER1_WLE) {
				/*
				 *	Give up on write locked deviced
				 *	immediately.
				 */
				printf("xp%d: write locked\n", unit);
				bp->b_flags |= B_ERROR;
			} else {
				/*
				 * After 28 retries (16 without offset and
				 * 12 with offset positioning), give up.
				 */
				if (++dp->b_errcnt > 28) {
#ifdef	UCB_DEVERR
				    harderr(bp, "xp");
				    printf("cs2=%b er1=%b\n", xpaddr->hpcs2.w,
					HPCS2_BITS, xpaddr->hper1, HPER1_BITS);
#else
				    deverror(bp, xpaddr->hpcs2.w, xpaddr->hper1);
#endif
				    bp->b_flags |= B_ERROR;
				} else
				    xc->xp_active = 0;
			}
#ifdef UCB_ECC
			/*
			 * If soft ecc, correct it (continuing
			 * by returning if necessary).
			 * Otherwise, fall through and retry the transfer.
			 */
			if((xpaddr->hper1 & (HPER1_DCK|HPER1_ECH)) == HPER1_DCK)
				if (xpecc(bp))
					return;
#endif
			xpaddr->hpcs1.w = HP_TRE | HP_IE | HP_DCLR | HP_GO;
			if ((dp->b_errcnt & 07) == 4) {
				xpaddr->hpcs1.w = HP_RECAL | HP_IE | HP_GO;
				while ((xpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
					;
			}
			xd->xp_cc = -1;
		}
		if (xc->xp_active) {
			if (dp->b_errcnt) {
				xpaddr->hpcs1.w = HP_RTC | HP_GO;
				while ((xpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY)
					;
			}
			xc->xp_active = 0;
			xc->xp_actf = dp->b_forw;
			dp->b_active = 0;
			dp->b_errcnt = 0;
			dp->b_actf = bp->b_actf;
			xd->xp_cc = bp->b_cylin;
			bp->b_resid = - (xpaddr->hpwc << 1);
#ifdef EXT_LSI
			if(bp->b_flags & B_PHYS)
				Sofub_relse(bp,bp->b_bcount);
#endif
			iodone(bp);
			xpaddr->hpcs1.w = HP_IE;
			if (dp->b_actf)
				xpustart(unit);
		}
		as &= ~(1 << xp_drive[unit].xp_unit);
	} else
		{
		if (as == 0)
			xpaddr->hpcs1.w = HP_IE;
		xpaddr->hpcs1.c[1] = HP_TRE >> 8;
	}
	for (unit = 0; unit < NXP; unit++)
		if ((xp_drive[unit].xp_ctlr == xc) && 
			(as & (1 << xp_drive[unit].xp_unit)))
				xpustart(unit);
	xpstart(xc);
}

#ifdef	UCB_ECC
#define	exadr(x,y)	(((long)(x) << 16) | (unsigned)(y))

/*
 * Correct an ECC error and restart the i/o to complete
 * the transfer if necessary.  This is quite complicated because
 * the correction may be going to an odd memory address base
 * and the transfer may cross a sector boundary.
 */
xpecc(bp)
register struct	buf *bp;
{
	register struct xp_drive *xd;
	register struct	hpdevice *xpaddr;
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
	xd = &xp_drive[unit];
	xpaddr = xd->xp_ctlr->xp_addr;
	wc = xpaddr->hpwc;
	ndone = (wc * NBPW) + bp->b_bcount;
	npx = ndone / PGSIZE;
	printf("xp%d%c: soft ecc bn %D\n",
		unit, 'a' + (minor(bp->b_dev) & 07),
		bp->b_blkno + (npx - 1));
	wrong = xpaddr->hpec2;
	if (wrong == 0) {
		xpaddr->hpof = HPOF_FMT22;
		xpaddr->hpcs1.w |= HP_IE;
		return (0);
	}

	/*
	 *	Compute the byte/bit position of the err
	 *	within the last disk page transferred.
	 *	Hpec1 is origin-1.
	 */
	byte = xpaddr->hpec1 - 1;
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
		if (bp->b_flags & (B_MAP|B_UBAREMAP)) {
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

	xd->xp_ctlr->xp_active++;
	if (wc == 0)
		return (0);

	/*
	 * Have to continue the transfer.  Clear the drive
	 * and compute the position where the transfer is to continue.
	 * We have completed npx sectors of the transfer already.
	 */
	ocmd = (xpaddr->hpcs1.w & ~HP_RDY) | HP_IE | HP_GO;
	xpaddr->hpcs2.w = xd->xp_unit;
	xpaddr->hpcs1.w = HP_TRE | HP_DCLR | HP_GO;

	bn = dkblock(bp);
	cn = bp->b_cylin - (bn / xd->xp_nspc);
	bn += npx;
	addr = bb + ndone;

	cn += bn / xd->xp_nspc;
	sn = bn % xd->xp_nspc;
/*	tn = sn / xd->xp_nsect;  CC complains about this ??? */
	tn = sn;
	tn /= xd->xp_nsect;
	sn %= xd->xp_nsect;

	xpaddr->hpdc = cn;
	xpaddr->hpda = (tn << 8) + sn;
	xpaddr->hpwc = ((int)(ndone - bp->b_bcount)) / NBPW;
	xpaddr->hpba = (int) addr;
	if (xd->xp_ctlr->xp_flags & XP_RH70)
		xpaddr->hpbae = (int) (addr >> 16);
	xpaddr->hpcs1.w = ocmd;
	return (1);
}
#endif	UCB_ECC

#if	defined(XP_DUMP) && defined(UCB_AUTOBOOT)
/*
 *  Dump routine.
 *  Dumps from dumplo to end of memory/end of disk section for minor(dev).
 */

#define DBSIZE	16			/* number of blocks to write */

xpdump(dev)
dev_t dev;
{
	/* ONLY USE 2 REGISTER VARIABLES, OR C COMPILER CHOKES */
	register struct xp_drive *xd;
	register struct hpdevice *xpaddr;
	daddr_t	bn, dumpsize;
	long	paddr;
	int	sn, count;
	extern	bool_t ubmap;
	struct ubmap *ubp;

	if ((bdevsw[major(dev)].d_strategy != xpstrategy)	/* paranoia */
	    || ((dev=minor(dev)) > (NXP << 3)))
		return(EINVAL);
	xd = &xp_drive[dev >> 3];
	dev &= 07;
	if (xd->xp_ctlr == 0)
		return(EINVAL);
	xpaddr = xd->xp_ctlr->xp_addr;
	dumpsize = xd->xp_sizes[dev].nblocks;
	if ((dumplo < 0) || (dumplo >= dumpsize))
		return(EINVAL);
	dumpsize -= dumplo;

	xpaddr->hpcs2.w = xd->xp_unit;
	if ((xpaddr->hpds & HPDS_VV) == 0) {
		xpaddr->hpcs1.w = HP_DCLR | HP_GO;
		xpaddr->hpcs1.w = HP_PRESET | HP_GO;
		xpaddr->hpof = HPOF_FMT22;
	}
	if ((xpaddr->hpds & (HPDS_DPR | HPDS_MOL)) != (HPDS_DPR | HPDS_MOL))
		return(EFAULT);
	ubp = &UBMAP[0];
	for (paddr = 0L; dumpsize > 0; dumpsize -= count) {
		count = dumpsize>DBSIZE? DBSIZE: dumpsize;
		bn = dumplo + (paddr >> PGSHIFT);
		xpaddr->hpdc = bn / xd->xp_nspc
			+ xd->xp_sizes[dev].cyloff;
		sn = bn % xd->xp_nspc;
		xpaddr->hpda = ((sn / xd->xp_nsect) << 8) | (sn % xd->xp_nsect);
		xpaddr->hpwc = -(count << (PGSHIFT - 1));
		if (ubmap && ((xd->xp_ctlr->xp_flags & XP_RH70) == 0)) {
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			xpaddr->hpba = 0;
			xpaddr->hpcs1.w = HP_WCOM | HP_GO;
		}
		else
			{
			/*
			 *  Non-UNIBUS map, or 11/70 RH70 (MASSBUS)
			 */
			xpaddr->hpba = loint(paddr);
			if (xd->xp_ctlr->xp_flags & XP_RH70)
				xpaddr->hpbae = hiint(paddr);
			xpaddr->hpcs1.w = HP_WCOM | HP_GO | ((paddr >> 8) & (03 << 8));
		}
		while (xpaddr->hpcs1.w & HP_GO)
			;
		if (xpaddr->hpcs1.w & HP_TRE) {
			if (xpaddr->hpcs2.w & HPCS2_NEM)
				return(0);	/* made it to end of memory */
			return(EIO);
		}
		paddr += (DBSIZE << PGSHIFT);
	}
	return(0);		/* filled disk minor dev */
}
#endif	XP_DUMP
#endif	NXP

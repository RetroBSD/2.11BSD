/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)xp.c	1.2 (2.11BSD GTE) 1/2/93
 */

/* $Header: xp.c,v 1.3 88/06/18 08:10:08 jbj Locked $ */
/* $Log:	xp.c,v $
 * Revision 1.3  88/06/18  08:10:08  jbj
 * Patch to fix bad sector forwarding (John Nelson, jack@cadre.dsl.pittsburgh.edu)
 * 
 * Revision 1.2  88/06/18  08:02:25  jbj
 * ioctl calls for SI bad sector forwarding.
 * 
 * Revision 1.1  88/06/18  07:54:42  jbj
 * Initial revision
 *
 */

/*
 * RM02/03/05, RP04/05/06, Ampex 9300, CDC 9766, DIVA, Fuji 160, and SI
 * Eagle.  This driver will handle most variants of SMD drives on one or more
 * controllers.  If XP_PROBE is defined, it includes a probe routine that
 * will determine the number and type of drives attached to each controller;
 * otherwise, the data structures must be initialized.
 *
 * For simplicity we use hpreg.h instead of an xpreg.h.
 * The bits are the same.
 */

#include "xp.h"
#if NXPD > 0

#include "param.h"
#include "../machine/seg.h"

#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "user.h"
#include "ioctl.h"
#include "dkio.h"
#include "hpreg.h"
#include "dkbad.h"
#include "dk.h"

#include "map.h"
#include "uba.h"

#define	XP_SDIST	2
#define	XP_RDIST	6
#define	xpunit(dev)	((minor(dev) >> 3) & 07)

int xp_offset[] = {
	HPOF_P400,	HPOF_M400,	HPOF_P400,	HPOF_M400,
	HPOF_P800,	HPOF_M800,	HPOF_P800,	HPOF_M800,
	HPOF_P1200,	HPOF_M1200,	HPOF_P1200,	HPOF_M1200,
	0,		0,		0,		0,
};

/*
 * xp_drive and xp_controller may be initialized here, or filled in at boot
 * time if XP_PROBE is enabled.  xp_controller address fields must be
 * initialized for any boot devices, however.
 *
 * xp_controller structure: one line per controller.  Only the address need
 * be initialized in the controller structure if XP_PROBE is defined (at least
 * the address for the root device); otherwise the flags must be here also.
 * The XP_NOCC flag is set for RM02/03/05's (with no current cylinder
 * register); XP_NOSEARCH is set for Diva's without the search command.  The
 * XP_RH70 flag need not be set here, the driver will always check that.
 */
#define XPADDR	((struct hpdevice *)0176700)
struct xp_controller	xp_controller[NXPC] = {
/*	0	0	addr	flags			0 */
#ifdef XP_PROBE
	0,	0,	XPADDR,	0,			0
#else
	0,	0,	XPADDR,	XP_NOCC|XP_NOSEARCH,	0
#endif
};

/*
 * xp_drive structure: one entry per drive.  The drive structure must be
 * initialized if XP_PROBE is not enabled.  Macros are provided in hpreg.h
 * to initialize entries according to drive type, and controller and drive
 * numbers.  See those for examples on how to set up other types of drives.
 * With XP_PROBE defined, xpslave will fill in this structure, and any
 * initialization will be overridden.  There is one exception; if the
 * drive-type field is set, it will be used instead of the drive-type register
 * to determine the drive's type.
 */
struct xp_drive	xp_drive[NXPD]
#ifndef XP_PROBE
= {
	RM02_INIT(0,0),		/* RM02, controller 0, drive 0 */
	RM02_INIT(0,1),		/* RM02, controller 0, drive 1 */
	RM2X_INIT(0,0)		/* Fuji 160, controller 0, drive 0 */
	RM2X_INIT(0,1)		/* Fuji 160, controller 0, drive 1 */
	RM05X_INIT(0,2)		/* 815-cyl RM05, controller 0, drive 2 */
}
#endif
;

/* THIS SHOULD BE READ OFF THE PACK, PER DRIVE */
struct size {
	daddr_t	nblocks;
	int	cyloff;
} rm_sizes[8] = { /* RM02/03 */
#ifdef FOXNPUP
	  32000,	  0,	/* a: cyl  0 -  199 */
	  32000,	200,	/* b: cyl 200 - 399 */
	  32000,	423,	/* c: cyl 423 - 622 */
	  31840,	623,	/* d: cyl 623 - 821 */
	   3520,	400,	/* e: cyl 400 - 422 */
	  63840,	423,	/* f: cyl 423 - 821, overlaps c & d */
	  99360,	200,	/* g: cyl 200 - 821, overlaps b thru end */
				/* CAUTION: Partition g should not be */
				/* used on the root device if swap is */
				/* defined on partition e. */
	  131680,	  0,	/* h: cyl   0 - 822 */
#else
	   4800,	  0,	/* a: cyl   0 -  29 */
	   4800,	 30,	/* b: cyl  30 -  59 */
	 122080,	 60,	/* c: cyl  60 - 822 */
	  62720,	 60,	/* d: cyl  60 - 451 */
	  59360,	452,	/* e: cyl 452 - 822 */
	   9600,	  0,	/* f: cyl   0 -  59, overlaps a & b */
	      0,	  0,	/* g: Not Defined */
	 131680,	  0,	/* h: cyl   0 - 822 */
#endif
}, rm5_sizes[8] = { /* RM05, CDC 9766 */
	   9120,	  0,	/* a: cyl   0 -  14 */
	   9120,	 15,	/* b: cyl  15 -  29 */
	 234080,	 30,	/* c: cyl  30 - 414 */
	 248064,	415,	/* d: cyl 415 - 822 */
	 164160,	 30,	/* e: cyl  30 - 299 */
	 152000,	300,	/* f: cyl 300 - 549 */
	 165984,	550,	/* g: cyl 550 - 822 */
	 500384,	  0,	/* h: cyl   0 - 822 */
}, si5_sizes[8] = { /* SI9775, direct mapping */
	  10240,	  0,	/* a: cyl   0 -   7 */
	  10240,	  8,	/* b: cyl   8 -  15 */
	 510720,	 16,	/* c: cyl  16 - 414 */
	 547840,	415,	/* d: cyl 415 - 842 */
	 363520,	 16,	/* e: cyl  16 - 299 */
	 320000,	300,	/* f: cyl 300 - 549 */
	 375040,	550,	/* g: cyl 550 - 842 */
	1079040,	  0,	/* h: cyl   0 - 842 */
}, hp_sizes[8] = { /* RP04/05/06 */
	   9614,	  0,	/* a: cyl   0 - 22 */
	   8778,	 23,	/* b: cyl  23 - 43 */
	 153406,	 44,	/* c: cyl  44 - 410, RP04/05 */
	 168872,	411,	/* d: cyl 411 - 814, RP06 */
	 322278,	 44,	/* e: cyl  44 - 814, RP06 */
	      0,	  0,	/* f: Not Defined */
	 171798,	  0,	/* g: cyl   0 - 410, whole RP04/05 */
	 340670,	  0	/* h: cyl   0 - 814, whole RP06 */
}, dv_sizes[8] = { /* Diva Comp V, Ampex 9300 in direct mode */
	   9405,	  0,	/* a: cyl   0 -  14 */
	   9405,	 15,	/* b: cyl  15 -  29 */
	 241395,	 30,	/* c: cyl  30 - 414 */
	 250800,	415,	/* d: cyl 415 - 814 */
	 169290,	 30,	/* e: cyl  30 - 299 */
	 156750,	300,	/* f: cyl 300 - 549 */
	 166155,	550,	/* g: cyl 550 - 814 */
	 511005,	  0	/* h: cyl   0 - 814 */
}, rm2x_sizes[8] = { /* Fuji 160 */
	   9600,	  0,	/* a: cyl   0 -  29 */
	   9600,	 30,	/* b: cyl  30 -  59 */
	 244160,	 60,	/* c: cyl  60 - 822 */
	 125440,	 60,	/* d: cyl  60 - 451 */
	 118720,	452,	/* e: cyl 452 - 822 */
	  59520,	452,	/* f: cyl 452 - 637 */
	  59200,	638,	/* g: cyl 638 - 822 */
	 263360,	  0	/* h: cyl   0 - 822 */
}, cap_sizes[8] = { /* SI Capricorn */
	  16384,	  0,	/* a: cyl   0 thru   31 */
	  33792,	 32,	/* b: cyl  32 thru   97 */
	 291840,	 98,	/* c: cyl  98 thru  667 */
	  16384,	668,	/* d: cyl 668 thru  699 */
	  56320,	700,	/* e: cyl 700 thru  809 */
	 109568,	810,	/* f: cyl 810 thru 1023 */
	 182272,	668,	/* g: cyl 668 thru 1023 */
	 524288,	  0,	/* h: cyl   0 thru 1023 */
}, si_sizes[8] = { /* SI Eagle */
	  11520,	  0,	/* a: cyl   0 -  11 */
	  11520,	 12,	/* b: cyl  12 -  23 */
	 474240,	 24,	/* c: cyl  24 - 517 */
	  92160,	518,	/* d: cyl 518 - 613 */
	 218880,	614,	/* e: cyl 614 - 841 */
	      0,	  0,	/* f: Not Defined */
	      0,	  0,	/* g: Not Defined */
	 808320,	  0	/* h: cyl   0 - 841 (everything) */
};
/* END OF STUFF WHICH SHOULD BE READ IN PER DISK */

#ifdef XP_PROBE
struct xpst {
	short	type;		/* value from controller type register */
	short	nsect;		/* number of sectors/track */
	short	ntrack;		/* number of tracks/cylinder */
	short	ncyl;		/* number of cylinders */
	struct	size *sizes;	/* partition tables */
	short	flags;		/* controller flags */
} xpst[] = {
	{ RM02, RM_SECT,   RM_TRAC,   RM_CYL,   rm_sizes,   XP_NOCC },
	{ RM2X, RM2X_SECT, RM2X_TRAC, RM2X_CYL, rm2x_sizes, XP_NOCC },
	{ RM03, RM_SECT,   RM_TRAC,   RM_CYL,   rm_sizes,   XP_NOCC },
	{ RM05, RM5_SECT,  RM5_TRAC,  RM5_CYL,  rm5_sizes,  XP_NOCC },
	{ RM5X, RM5X_SECT, RM5X_TRAC, RM5X_CYL, rm5_sizes,  XP_NOCC },
	{ SI5,  SI5_SECT,  SI5_TRAC,  SI5_CYL,  si5_sizes,  XP_NOCC },
	{ RP04, HP_SECT,   HP_TRAC,   RP04_CYL, hp_sizes,   0 },
	{ RP05, HP_SECT,   HP_TRAC,   RP04_CYL, hp_sizes,   0 },
	{ RP06, HP_SECT,   HP_TRAC,   RP06_CYL, hp_sizes,   0 },
	{ DV,   DV_SECT,   DV_TRAC,   DV_CYL,	dv_sizes,   XP_NOSEARCH },
	{ CAP,  CAP_SECT,  CAP_TRAC,  CAP_CYL,  cap_sizes,  XP_NOCC },
	{ SI,   SI_SECT,   SI_TRAC,   SI_CYL,   si_sizes,   XP_NOCC },
	{ 0,    0,         0,         0,        0 }
};
#endif

struct	buf	xptab;
struct	buf	xputab[NXPD];

#ifdef BADSECT
int maxbad = MAXBAD;		/* KLUDGE: no. of bad blocks in table */
struct	dkbad	xpbad[NXPD];	/* replacement block number */
struct	buf	bxpbuf[NXPD];
bool_t	xp_init[NXPD];		/* drive initialized */
bool_t	xp_rwhdr[8*NXPD];	/* next i/o includes header */
#endif

#ifdef UCB_METER
static	int		xp_dkn = -1;	/* number for iostat */
#endif

/*
 * Attach controllers whose addresses are known at boot time.  Stop at the
 * first not found, so that the drive numbering won't get confused.
 */
xproot()
{
	register int i;
	register struct hpdevice *xpaddr;

	for (i = 0; i < NXPC; i++)
		if (((xpaddr = xp_controller[i].xp_addr) == 0)
			|| (xpattach(xpaddr, i) == 0))
			break;
}

/*
 * Attach controller at xpaddr.  Mark as nonexistent if xpaddr is 0; otherwise
 * attach slaves if probing.  NOTE: if probing for drives, this routine must
 * be called once per controller, in ascending controller numbers.
 */
xpattach(xpaddr, unit)
	register struct hpdevice *xpaddr;
	int unit;
{
	register struct xp_controller *xc = &xp_controller[unit];
#ifdef XP_PROBE
	static int last_attached = -1;
#endif

#ifdef UCB_METER
	if (xp_dkn < 0) {
		dk_alloc(&xp_dkn, NXPD+NXPC, "xp", 0L);
#ifndef XP_PROBE
		/*
		 * Hard coded drive configuration - snag the number of
		 * sectors per track for each drive and compute drive
		 * transfer rate assuming 3600rpm (the Fujitsu Eagle 2351A
		 * (SI Eagle) is actually 3961rpm; it's just not worth the
		 * effort to fix the assumption.)  If XP_PROBE is defined we
		 * grab the number of sectors/track for each drive in
		 * xpslave.
		 */
		if (xp_dkn >= 0) {
			register int i;
			register long *lp;

			for (i = 0, lp = &dk_wps[xp_dkn]; i < NXPD; i++)
				*lp++ = (long)xp_drive[i].xp_nsect
 					* (60L * 256L);
		}
#endif
	}
#endif

	if ((unsigned)unit >= NXPC)
		return(0);
	if ((xpaddr != 0) && (fioword(xpaddr) != -1)) {
		xc->xp_addr = xpaddr;
		if (fioword(&xpaddr->hpbae) != -1)
			xc->xp_flags |= XP_RH70;
#ifdef XP_PROBE
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

#ifdef XP_PROBE
/*
 * Determine what drives are attached to a controller; guess their types and
 * fill in the drive structures.
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
		{
			int x = 6000;
			while (--x);		/* delay */
		}
		xpaddr->hpcs1.w = 0;
		xpaddr->hpcs2.w = j;
		xpaddr->hpcs1.w = HP_GO;	/* testing... */
		{
			int x = 6000;
			while (--x);		/* delay */
		}
		dummy = xpaddr->hpds;
		if (xpaddr->hpcs2.w & HPCS2_NED) {
			xpaddr->hpcs2.w = HPCS2_CLR;
			continue;
		}
		if (nxp < NXPD) {
			xd = &xp_drive[nxp++];
			xd->xp_ctlr = xc;
			xd->xp_unit = j;
			/* If drive type is initialized, believe it. */
			if (xd->xp_type == 0) {
				xd->xp_type = xpaddr->hpdt & 077;
				xd->xp_type = xpmaptype(xd, xpaddr->hpsn);
			}
			for (st = xpst; st->type; st++)
				if (st->type == xd->xp_type) {
					xd->xp_nsect = st->nsect;
					xd->xp_ntrack = st->ntrack;
					xd->xp_nspc = st->nsect * st->ntrack;
#ifdef BADSECT
					xd->xp_ncyl = st->ncyl;
#endif
					xd->xp_sizes = st->sizes;
					xd->xp_ctlr->xp_flags |= st->flags;
					break;
				}
			if (!st->type) {
				printf("xp%d: drive type %o unrecognized\n",nxp - 1, xd->xp_type);
				xd->xp_ctlr = NULL;
			}

#ifdef UCB_METER
			if (xp_dkn >= 0 && xd->xp_ctlr)
				dk_wps[xd - &xp_drive[0]]
 					= (long)xd->xp_nsect * (60L * 256L);
#endif
		}
	}
}

static
xpmaptype(xd, hpsn)
	register struct xp_drive *xd;
	register u_short hpsn;
{
	register u_short type = xd->xp_type;

	/*
	 * Model-byte processing for SI controllers.
	 * NB:  Only deals with RM02, RM03 and RM05 emulations.
	 */
	if ((type == RM02 || type == RM03 || type == RM05)
	    && (hpsn & SIMB_LU) == xd->xp_unit) {
		switch (hpsn & (SIMB_MB & ~(SIMB_S6|SIMB_XX|SIRM03|SIRM05))) {
		case SI9775D:
			type = SI5;
			break;

		case SI9775M:
			type = RM05;
			break;

		case SI9730D:
			type = RM2X;
			break;

		case SI9766:
			type = RM05;
			break;

		case SI9762:
			type = RM03;
			break;

		case SICAPD:
			type = CAP;
			break;

		case SI9751D:
			type = SI;
			break;
		}
	}
	return(type);
}
#endif XP_PROBE

xpopen(dev)
	dev_t dev;
{
	register struct xp_drive *xd;
	register int unit = xpunit(dev);

	if (unit >= NXPD || !(xd = &xp_drive[unit])->xp_ctlr ||
		!xd->xp_ctlr->xp_addr)
		return (ENXIO);
	return (0);
}

xpstrategy(bp)
register struct buf *bp;
{
	register struct xp_drive *xd;
	register unit;
	struct buf *dp;
	short pseudo_unit;
	int	s;
	long bn;

	unit = dkunit(bp);
	pseudo_unit = minor(bp->b_dev) & 07;

	if ((unit >= NXPD) || ((xd = &xp_drive[unit])->xp_ctlr == 0) ||
		(xd->xp_ctlr->xp_addr == 0)) {
		bp->b_error = ENXIO;
		goto errexit;
	}
	if ((bp->b_blkno < 0) || ((bn = dkblock(bp)) + ((bp->b_bcount + 511)
		>> 9) > xd->xp_sizes[pseudo_unit].nblocks)) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if ((xd->xp_ctlr->xp_flags & XP_RH70) == 0)
		mapalloc(bp);
	bp->b_cylin = bn / xd->xp_nspc + xd->xp_sizes[pseudo_unit].cyloff;
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
	struct buf *bp;
	daddr_t bn;
	int	sn, cn, csn;

	xd = &xp_drive[unit];
	xpaddr = xd->xp_ctlr->xp_addr;
	xpaddr->hpcs2.w = xd->xp_unit;
	xpaddr->hpcs1.c[0] = HP_IE;
	xpaddr->hpas = 1 << xd->xp_unit;
	if (unit >= NXPD)
		return;
#ifdef UCB_METER
	if (xp_dkn >= 0)
		dk_busy &= ~(1 << (xp_dkn + unit));
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
#ifdef BADSECT
	if (((xpaddr->hpds & HPDS_VV) == 0) || (xp_init[unit] == 0)) {
		struct buf *bbp = &bxpbuf[unit];
#else
	if ((xpaddr->hpds & HPDS_VV) == 0) {
#endif
	/* SHOULD WARN SYSTEM THAT THIS HAPPENED */
#ifdef XP_DEBUG
		printf("preset-unit=%d\n", unit);
#endif
		xpaddr->hpcs1.c[0] = HP_IE | HP_PRESET | HP_GO;
#ifdef XP_DEBUG
		printf("preset done\n");
#endif
		xpaddr->hpof = HPOF_FMT22;
#ifdef BADSECT
		xp_init[unit] = 1;
		bbp->b_flags = B_READ | B_BUSY | B_PHYS;
		bbp->b_dev = bp->b_dev | 7;	/* "h" partition whole disk */
		bbp->b_bcount = sizeof(struct dkbad);
		bbp->b_un.b_addr = (caddr_t)&xpbad[unit];
		bbp->b_blkno = (daddr_t)xd->xp_ncyl * xd->xp_nspc - xd->xp_nsect;
		bbp->b_cylin = xd->xp_ncyl - 1;
		if ((xd->xp_ctlr->xp_flags & XP_RH70) == 0)
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
	xpaddr->hpcs1.c[0] = (xd->xp_ctlr->xp_flags & XP_NOSEARCH) ?
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
	short pseudo_unit;
	daddr_t bn;
	int	unit, sn, tn, cn, cmd;

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
	 * Mark controller busy and determine destination of this request.
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
		while ((xpaddr->hpds & (HPDS_PIP | HPDS_DRY)) != HPDS_DRY);
	}
	xpaddr->hpdc = cn;
	xpaddr->hpda = (tn << 8) + sn;
	xpaddr->hpba = bp->b_un.b_addr;
	if (xc->xp_flags & XP_RH70)
		xpaddr->hpbae = bp->b_xmem;
	xpaddr->hpwc = -(bp->b_bcount >> 1);
	/*
	 * Initiate i/o command.
	 */
	cmd = ((bp->b_xmem & 3) << 8) | HP_IE | HP_GO;
#ifdef XP_FORMAT
	if (minor(bp->b_dev) & 0200 || xp_rwhdr[unit]) {
		cmd |= bp->b_flags & B_READ ? HP_RHDR : HP_WHDR;
	} else
#endif
		cmd |= bp->b_flags & B_READ ? HP_RCOM : HP_WCOM;
	xpaddr->hpcs1.w = cmd;
#ifdef UCB_METER
	if (xp_dkn >= 0) {
		int dkn = xp_dkn + NXPD + (xc - &xp_controller[0]);

		dk_busy |= 1<<dkn;
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
#ifdef UCB_METER
		if (xp_dkn >= 0)
			dk_busy &= ~(1 << (xp_dkn + NXPD + dev));
#endif
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
		unit = dkunit(bp);
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
				printf("xp%d: write locked\n", unit);
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
#ifdef XP_DEBUG
					/*
					 * for RM drives
					 */
					printf("cs2=%b er1=%b er2=%b %s\n",
						xpaddr->hpcs2.w, HPCS2_BITS,
						xpaddr->hper1, HPER1_BITS,
						xpaddr->rmer2, RMER2_BITS,
					((xp_rwhdr[unit]) ? " (hdr i/o)" : ""));
#else
					printf("cs2=%b er1=%b %s\n",
						xpaddr->hpcs2.w, HPCS2_BITS,
						xpaddr->hper1, HPER1_BITS,
					((xp_rwhdr[unit]) ? " (hdr i/o)" : ""));
#endif
					bp->b_flags |= B_ERROR;
					bp->b_flags &= ~B_BAD;
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
			xp_rwhdr[unit] = 0;
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

xpioctl(dev, cmd, data, flag)
	dev_t	dev;
	u_int	cmd;
	caddr_t	data;
{
	int unit = xpunit(dev);

	switch(cmd) {
	case DKIOCHDR:	/* do header read/write */
		xp_rwhdr[unit] = 1;
		return(0);
	case DKREINIT:	/* reread BBF table */
		xp_init[unit] = 0;
		return(0);
	case DKINFO:	/* get drive info */
		{	struct drv_info *xo = (struct drv_info *)data;
			register struct xp_drive *xd = &xp_drive[unit];

			xo->type = xd->xp_type;
			xo->model = 0;
			xo->trk = xd->xp_ntrack;
			xo->sec = xd->xp_nsect;
			xo->nspc = xd->xp_nspc;
			xo->ncyl = xd->xp_ncyl;
			xo->name[0] = '\0';
			bcopy(xd->xp_sizes, xo->fs, 48);
		}
		return(0);
	}
	return(EINVAL);
}

#define exadr(x,y)	(((long)(x) << 16) | (unsigned)(y))

/*
 * Correct an ECC error and restart the i/o to complete the transfer if
 * necessary.  This is quite complicated because the correction may be going
 * to an odd memory address base and the transfer may cross a sector boundary.
 */
xpecc(bp, flag)
register struct	buf *bp;
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
	unit = dkunit(bp);
	xd = &xp_drive[unit];
	xpaddr = xd->xp_ctlr->xp_addr;
#ifdef BADSECT
	if (flag == CONT) {
		npx = bp->b_error;
		bp->b_error = 0;
		ndone = npx * NBPG;
		wc = -((short)(bp->b_bcount - ndone) / (short)NBPW);
	}
	else {
#endif
		wc = xpaddr->hpwc;
		ndone = bp->b_bcount - ((unsigned)(-wc) * NBPW);
		npx = ndone / NBPG;
#ifdef BADSECT
	}
#endif
	ocmd = (xpaddr->hpcs1.w & ~HP_RDY) | HP_IE | HP_GO;
	bb = exadr(bp->b_xmem, bp->b_un.b_addr);
	bn = dkblock(bp);
	cn = bp->b_cylin - (bn / xd->xp_nspc);
	bn += npx;
	cn += bn / xd->xp_nspc;
	sn = bn % xd->xp_nspc;
	tn = sn;
	tn /= xd->xp_nsect;
	sn %= xd->xp_nsect;
	switch (flag) {
		case ECC:
			printf("xp%d%c: soft ecc sn%D\n",unit, 'a' + (minor(bp->b_dev) & 07), bp->b_blkno + (npx - 1));
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
#ifdef XP_DEBUG
			printf("revector to cn %d tn %d sn %d\n", cn, tn, sn);
#endif
			wc = -(512 / (short)NBPW);
			break;
		case CONT:
			bp->b_flags &= ~B_BAD;
#ifdef XP_DEBUG
			printf("xpecc CONT: bn %D cn %d tn %d sn %d\n", bn, cn, tn, sn);
#endif
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
	if (xd->xp_ctlr->xp_flags & XP_RH70)
		xpaddr->hpbae = (int)(addr >> 16);
	xpaddr->hpcs1.w = ocmd;
	return (1);
}

#ifdef XP_DUMP
/*
 * Dump routine.  Dumps from dumplo to end of memory/end of disk section for
 * minor(dev).
 */
#define DBSIZE	16			/* unit of transfer, same number */

xpdump(dev)
	dev_t dev;
{
	/*
	 * ONLY USE 2 REGISTER VARIABLES, OR C COMPILER CHOKES
	 */
	register struct xp_drive *xd;
	register struct hpdevice *xpaddr;
	daddr_t bn, dumpsize;
	long paddr;
	int	sn, count;
	struct ubmap *ubp;

	if ((bdevsw[major(dev)].d_strategy != xpstrategy)	/* paranoia */
		|| ((dev=minor(dev)) > (NXPD << 3)))
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
		xpaddr->hpdc = bn / xd->xp_nspc + xd->xp_sizes[dev].cyloff;
		sn = bn % xd->xp_nspc;
		xpaddr->hpda = ((sn / xd->xp_nsect) << 8) | (sn % xd->xp_nsect);
		xpaddr->hpwc = -(count << (PGSHIFT - 1));
		if (ubmap && ((xd->xp_ctlr->xp_flags & XP_RH70) == 0)) {
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			xpaddr->hpba = 0;
			xpaddr->hpcs1.w = HP_WCOM | HP_GO;
		}
		else {
			/*
			 * Non-UNIBUS map, or 11/70 RH70 (MASSBUS)
			 */
			xpaddr->hpba = loint(paddr);
			if (xd->xp_ctlr->xp_flags & XP_RH70)
				xpaddr->hpbae = hiint(paddr);
			xpaddr->hpcs1.w = HP_WCOM | HP_GO | ((paddr >> 8) & (03 << 8));
		}
		while (xpaddr->hpcs1.w & HP_GO);
		if (xpaddr->hpcs1.w & HP_TRE) {
			if (xpaddr->hpcs2.w & HPCS2_NEM)
				return(0);	/* made it to end of memory */
			return(EIO);
		}
		paddr += (DBSIZE << PGSHIFT);
	}
	return(0);		/* filled disk minor dev */
}
#endif XP_DUMP
#endif NXPD

/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)xp.c	2.3 (2.11BSD) 1996/3/8
 */

/*
 * SMD disk driver
 */
#include "../h/param.h"
#include "../pdpuba/hpreg.h"
#include "../machine/iopage.h"
#include "saio.h"

#define	NXP	2

	struct	hpdevice *XPcsr[NXP + 1] =
		{
		(struct hpdevice *)0176700,
		(struct hpdevice *)0,
		(struct hpdevice *)-1
		};

	static	char	xpinit[NXP][8];		/* XXX */

xpopen(io)
	register struct iob *io;
	{
	register struct disklabel *lp = &io->i_label;
	register struct	hpdevice *xpaddr;
	int	dummy;

	if	(genopen(NXP, io) < 0)
		return(-1);
/*
 * If this is the first access for the drive check to see if it is
 * present.  If the drive is present then read the label.
*/
	if	(xpinit[io->i_ctlr][io->i_unit] == 0)
		{
		xpaddr = XPcsr[io->i_ctlr];
		xpaddr->hpcs1.w = HP_NOP;
		xpaddr->hpcs2.w = io->i_unit;
		xpaddr->hpcs1.w = HP_GO;
		delay(6000);
		dummy = xpaddr->hpds;
		if	(xpaddr->hpcs2.w & HPCS2_NED)
			{
			xpaddr->hpcs2.w = HPCS2_CLR;
			return(-1);
			}
		if	(devlabel(io, READLABEL) == -1)
			return(-1);
		xpinit[io->i_ctlr][io->i_unit] = 1;
		}
	io->i_boff = lp->d_partitions[io->i_part].p_offset;
	return(0);
	}

xpclose(io)
	struct	iob	*io;
	{

	xpinit[io->i_ctlr][io->i_unit] = 0;
	return(0);
	}

xpstrategy(io, func)
	register struct iob *io;
{
	int i;
	daddr_t bn;
	int sn, cn, tn, bae, lo16;
	register struct hpdevice *xpaddr = XPcsr[io->i_ctlr];
	register struct disklabel *lp = &io->i_label;

	i = deveovchk(io);
	if	(i <= 0)
		return(i);

	bn = io->i_bn;
	xpaddr->hpcs2.w = io->i_unit;

	if ((xpaddr->hpds & HPDS_VV) == 0) {
		xpaddr->hpcs1.c[0] = HP_PRESET|HP_GO;
		xpaddr->hpof = HPOF_FMT22;
	}
	cn = bn / lp->d_secpercyl;
	sn = bn % lp->d_secpercyl;
	tn = sn / lp->d_nsectors;
	sn = sn % lp->d_nsectors;

	iomapadr(io->i_ma, &bae, &lo16);
	xpaddr->hpdc = cn;
	xpaddr->hpda = (tn << 8) + sn;
	xpaddr->hpba = (caddr_t)lo16;
	xpaddr->hpwc = -(io->i_cc>>1);
	i = (bae << 8) | HP_GO;
	if (func == READ)
		i |= HP_RCOM;
	else if (func == WRITE)
		i |= HP_WCOM;
	xpaddr->hpcs1.w = i;
	while ((xpaddr->hpcs1.w & HP_RDY) == 0)
			continue;
	if (xpaddr->hpcs1.w & HP_TRE) {
		printf("%s err cy=%d tr=%d sc=%d cs2=%o er1=%o\n",
		    devname(io), cn, tn, sn, xpaddr->hpcs2,
		    xpaddr->hper1);
		return(-1);
	}
	return(io->i_cc);
}

/*
 * ALL drive type identification has been removed from the kernel's XP 
 * driver and placed here.
 *
 * These tables are used to provide "the best guess" of the geometry 
 * of the drive.  DEC RP0{4,5,6,7} and RM0{3,5} drives will match exactly.
 * Non DEC controllers (performing an emulation) often use the DEC drive 
 * type to mean completely different geometry/capacity drives.
*/

	struct	xpst
		{
		int	flags;		/* flags: XP_CC, XP_NOSEARCH */
		int	ncyl;		/* number of cylinders */
		int	nspc;		/* number of sectors per cylinder */
		int	ntpc;		/* number of tracks per cylinder */
		int	nspt;		/* number of sectors per track */
		daddr_t	nspd;		/* number of sectors per drive */
		};

static struct	xpst rp04_st = { XP_CC, 411, 22 * 19, 19, 22, 411L * 22 * 19 };
/* rp05 uses same table as rp04 */
static struct	xpst rp06_st = { XP_CC, 815, 22 * 19, 19, 22, 815L * 22 * 19 };
static struct	xpst rp07_st = { XP_CC, 630, 50 * 32, 32, 50, 630L * 50 * 32 };

static struct	xpst rm02_st;	/* filled in dynamically */
static struct	xpst rm03_st = { 0, 823, 32 *  5,  5, 32, 823L * 32 *  5 };
static struct	xpst rm05_st = { 0, 823, 32 * 19, 19, 32, 823L * 32 * 19 };
static struct	xpst rm80_st = { 0, 559, 31 * 14, 14, 31, 559L * 31 * 14 };

/* 
 * SI controller stuff - likely not used any longer and will probably go away
 * eventually (when the D space is needed for something more important).
*/

static struct	xpst cdc9775_st = { 0, 843, 32 * 40, 40, 32, 843L * 32 * 40 };
static struct	xpst cdc9730_st = { 0, 823, 32 * 10, 10, 32, 823L * 32 * 10 };
static struct	xpst cdc9766_st = { 0, 823, 32 * 19, 19, 32, 823L * 32 * 19 };
static struct	xpst cdc9762_st = { 0, 823, 32 *  5,  5, 32, 823L * 32 *  5 };
static struct	xpst capric_st = { 0, 1024, 32 * 16, 16, 32, 1024L * 32 * 16 };
static struct	xpst eagle_st   = { 0, 842, 48 * 20, 20, 48, 842L * 48 * 20 };

/*
 * A "default".  If none of the above are useable then a default geometry
 * suitable for writing a label (sector 1) is used.
*/

static struct	xpst default_st = { 0, 1, 2 * 1, 1, 2, 2 };

/*
 * This routine does not return an error (-1).  If the drive is totally
 * unrecognizeable then the default above is used.
*/

xplabel(io)
	struct iob *io;
	{
	register struct	xpst	*st = NULL;
	int	type, xpsn;
	register struct	hpdevice *xpaddr = XPcsr[io->i_ctlr];
	register struct disklabel *lp;

	type = xpaddr->hpdt & 077;
	switch	(type)
		{
		case	HPDT_RP04:	/* 020 */
		case	HPDT_RP05:	/* 021 */
			st = &rp04_st;
			break;
		case	HPDT_RP06:	/* 022 */
			st = &rp06_st;
			break;
		case	HPDT_RP07:	/* 042 */
			st = &rp07_st;
			break;
		case	HPDT_RM80:	/* 026 */
			st = &rm80_st;
			break;
		case	HPDT_RM05:	/* 027 */
			st = &rm05_st;
			break;
		case	HPDT_RM03:	/* 024 */
			st = &rm03_st;
			break;
		case	HPDT_RM02:	/* 025 */
/*
 * Borrowing a quote from 4BSD:  
 *  "We know this isn't a dec controller, so we can assume sanity."
*/
			st = &rm02_st;
			xpaddr->hpcs1.w = HP_NOP;
			xpaddr->hpcs2.w = io->i_unit;
	
			xpaddr->rmhr = HPHR_MAXTRAK;
			st->ntpc = xpaddr->rmhr + 1;

			xpaddr->rmhr = HPHR_MAXSECT;
			st->nspt = xpaddr->rmhr + 1;

			xpaddr->rmhr = HPHR_MAXCYL;
			st->ncyl = xpaddr->rmhr + 1;

			xpaddr->hpcs1.w = HP_DCLR | HP_GO;

			st->nspc = st->nspt * st->ntpc;
			st->nspd = (long)st->nspc * st->ncyl;
			printf("type: RM02 c=%d t/c=%d s/t=%d\n", st->ncyl,
				st->ntpc, st->nspt);
			break;
		default:
			printf("%s unknown drive type: %d -- ", 
				devname(io), type);
			printf("using 1 cyl, 1 trk, 2 sec/trk\n");
			st = &default_st;
			break;
		}

/*
 * Handle SI model byte stuff when we think it's an RM05 or RM03.  Is any
 * one still using one of these?  Is it worth all this code?
*/
	if	(type == HPDT_RM05 || type == HPDT_RM03)
		{
		xpsn = xpaddr->hpsn;
		if	((xpsn & SIMB_LU) != io->i_unit)
			goto notsi;
		switch	((xpsn & SIMB_MB) &~ (SIMB_S6|SIRM03|SIRM05))
			{
			case	SI9775D:
				st = &cdc9775_st;
				break;
			case	SI9730D:
				st = &cdc9730_st;
				break;
			case	SI9766:
				st = &cdc9766_st;
				break;
			case	SI9762:
				st = &cdc9762_st;
				break;
			case	SICAPD:
				st = &capric_st;
				break;
			case	SI9751D:
				st = &eagle_st;
				break;
			default:
				printf("%s unknown SI drive model %d -- ", 
					devname(io), xpsn);
				printf("using 1 cyl, 1 trk, 2 sec/trk\n");
				st = &default_st;
				break;
			}
		}
notsi:
	lp = &io->i_label;
	lp->d_type = DTYPE_SMD;
	lp->d_secsize = 512;	/* XXX */
	lp->d_secperunit = st->nspd;
	lp->d_partitions[0].p_size = st->nspd;
	lp->d_nsectors = st->nspt;
	lp->d_ntracks = st->ntpc;
	lp->d_secpercyl = st->nspc;
	lp->d_ncylinders = st->ncyl;
	lp->d_drivedata[0] = st->flags;
	strcpy(lp->d_typename, "SMD");
	return(0);
	}

/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)hk.c	2.3 (2.11BSD) 1997/11/7
 */

/*
 *	RK06/07 disk driver for standalone
 */

#include "../h/param.h"
#include "../pdpuba/hkreg.h"
#include "saio.h"

#define	NHK	2
#define	NSECT	22
#define	NTRAC	3
#define	N7CYL	815
#define	N6CYL	411

	struct	hkdevice *HKcsr[NHK + 1] =
		{
		(struct hkdevice *)0177440,
		(struct hkdevice *)0,
		(struct hkdevice *)-1
		};

	u_char	hk_mntflg[NHK];		/* Type known bitmap, 0 = unknown */
	u_char	hk_drvtyp[NHK];		/* Drive type.  0 = RK06, 1 = RK07 */

hkstrategy(io, func)
	register struct iob *io;
	{
	int unit, com;
	register struct hkdevice *hkaddr;
	daddr_t bn;
	int	hktyp, drvbit, i;
	int sn, cn, tn, ctlr, bae, lo16;

	i = deveovchk(io);
	if	(i < 0)
		return(i);
	unit = io->i_unit;
	ctlr = io->i_ctlr;
	hkaddr = HKcsr[ctlr];

	drvbit = 1 << unit;
	if	(hk_drvtyp[ctlr] & drvbit)
		hktyp = 02000;
	else
		hktyp = 0;

	bn = io->i_bn;
	hkaddr->hkcs2 = HKCS2_SCLR;
	while ((hkaddr->hkcs1 & HK_CRDY) == 0)
		continue;
	hkaddr->hkcs2 = unit;
	hkaddr->hkcs1 = hktyp|HK_SELECT|HK_GO;
	while ((hkaddr->hkcs1 & HK_CRDY) == 0)
		continue;

	if	((hkaddr->hkds & HKDS_VV) == 0)
		{
		hkaddr->hkcs1 = hktyp|HK_PACK|HK_GO;
		while	((hkaddr->hkcs1 & HK_CRDY) == 0)
			continue;
		}
	cn = bn/(NSECT*NTRAC);
	sn = bn%(NSECT*NTRAC);
	tn = sn/NSECT;
	sn = sn%NSECT;

	iomapadr(io->i_ma, &bae, &lo16);
	hkaddr->hkcyl = cn;
	hkaddr->hkda = (tn<<8) | sn;
	hkaddr->hkba = (caddr_t)lo16;
	hkaddr->hkwc = -(io->i_cc>>1);
	com = hktyp|(bae << 8) | HK_GO;
	if	(func == READ)
		com |= HK_READ;
	else if (func == WRITE)
		com |= HK_WRITE;
	hkaddr->hkcs1 = com;

	while	((hkaddr->hkcs1 & HK_CRDY) == 0)
		continue;

	if	(hkaddr->hkcs1 & HK_CERR)
		{
		printf("%s err: cy=%d tr=%d sc=%d cs2=%d er=%o\n",
			devname(io), cn, tn, sn, hkaddr->hkcs2, hkaddr->hker);
		return(-1);
		}
	return(io->i_cc);
	}

hkopen(io)
	register struct iob *io;
	{
	register struct hkdevice *hkaddr;
	int	drvbit;
	int	unit, ctlr;

	drvbit = 1 << io->i_unit;

	if	(genopen(NHK, io) < 0)
		return(-1);

	if	((hk_mntflg[ctlr] & drvbit) == 0)
		{
		hkaddr = HKcsr[ctlr];
		hkaddr->hkcs2 = unit;
		hkaddr->hkcs1 = HK_SELECT|HK_GO;
		while	((hkaddr->hkcs1 & HK_CRDY) == 0)
			continue;
		if	(hkaddr->hkcs1 & HK_CERR && hkaddr->hker & HKER_DTYE)
			hk_drvtyp[ctlr] |= drvbit;
		else
			hk_drvtyp[ctlr] &= ~drvbit;
		hk_mntflg[ctlr] |= drvbit;
		}
	if	(devlabel(io, READLABEL) < 0)
		return(-1);
	io->i_boff = io->i_label.d_partitions[io->i_part].p_offset;
	return(0);
	}

/*
 * This generates a default label.  'hkopen' has already been called and
 * determined the drive type.
*/

hklabel(io)
	register struct iob *io;
	{
	register struct disklabel *lp = &io->i_label;
	u_short ncyl, nblks;

	if	(hk_drvtyp[io->i_ctlr] && (1 << io->i_unit))
		ncyl = N7CYL;
	else
		ncyl = N6CYL;
	lp->d_partitions[0].p_size = lp->d_secperunit = NSECT * NTRAC * ncyl;
	lp->d_type = DTYPE_DEC;
	lp->d_nsectors = NSECT;
	lp->d_ntracks = NTRAC;
	lp->d_secpercyl = NSECT * NTRAC;
	lp->d_ncylinders = ncyl;
	return(0);
	}

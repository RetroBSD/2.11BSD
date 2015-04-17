/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)conf.c	2.7 (2.11BSD) 1997/11/7
 */

#include "../h/param.h"
#include "saio.h"

	int	nullsys();

extern	int	xpstrategy(), xpopen(), xpclose(), xplabel();
extern	int	brstrategy(), bropen();
extern	int	rkstrategy(), rkopen();
extern  int	rxstrategy(), rxopen();
extern	int	hkstrategy(), hkopen(), hklabel();
extern	int	rlstrategy(), rlopen(), rllabel();
extern	int	sistrategy(), siopen();
extern	int	rastrategy(), raopen(), raclose(), ralabel();
extern	int	tmstrategy(), tmopen(), tmclose(), tmseek();
extern	int	htstrategy(), htopen(), htclose(), htseek();
extern	int	tsstrategy(), tsopen(), tsclose(), tsseek();
extern	int	tmscpstrategy(), tmscpopen(), tmscpclose(), tmscpseek();

extern	caddr_t	*XPcsr[], *BRcsr[], *RKcsr[], *HKcsr[], *RLcsr[], *RXcsr[];
extern	caddr_t	*SIcsr[], *RAcsr[], *TMcsr[], *HTcsr[], *TScsr[], *TMScsr[];

/*
 * NOTE!  This table must be in major device number order.  See /sys/pdp/conf.c
 *	  for the major device numbers.
*/

struct devsw devsw[] = {
	"ht",	htstrategy,	htopen,		htclose,	HTcsr, /* 0 */
	nullsys, htseek,
	"tm",	tmstrategy,	tmopen,		tmclose,	TMcsr, /* 1 */
	nullsys, tmseek,
	"ts",	tsstrategy,	tsopen,		tsclose,	TScsr, /* 2 */
	nullsys, tsseek,
	"ram",	nullsys,	nullsys,	nullsys,	0,     /* 3 */
	nullsys, nullsys,
	"hk",	hkstrategy,	hkopen,		nullsys,	HKcsr, /* 4 */
	hklabel, nullsys,
	"ra",	rastrategy,	raopen,		raclose,	RAcsr, /* 5 */
	ralabel, nullsys,
	"rk",	rkstrategy,	rkopen,		nullsys,	RKcsr, /* 6 */
	nullsys, nullsys,
	"rl",	rlstrategy,	rlopen,		nullsys,	RLcsr, /* 7 */
	rllabel, nullsys,
	"rx",	rxstrategy,	rxopen,		nullsys,	RXcsr, /* 8 */
	nullsys, nullsys,
	"si",	sistrategy,	siopen,		nullsys,	SIcsr, /* 9 */
	nullsys, nullsys,
	"xp",	xpstrategy,	xpopen,		xpclose,	XPcsr, /* 10 */
	xplabel, nullsys,
	"br",	brstrategy,	bropen,		nullsys,	BRcsr, /* 11 */
	nullsys, nullsys,
	"tms",  tmscpstrategy,	tmscpopen,	tmscpclose,	TMScsr,/* 12 */
	nullsys, tmscpseek,
	0,	0,		0,		0,		0,
	nullsys, nullsys,
};

	int	ndevsw = (sizeof (devsw) / sizeof (devsw[0])) - 1;

	char	ADJcsr[] =
		{
		0,	/* HT = 0 */
		2,	/* TM = 1 */
		2,	/* TS = 2 */
		0,	/* RAM = 3 */
		0,	/* HK = 4 */
		0,	/* RA = 5 */
		4,	/* RK = 6 */
		0,	/* RL = 7 */
		0,	/* RX = 8 */
		0,	/* XP/SI = 9 */
		0,	/* XP = 10 */
		4,	/* BR =11 */
		0,	/* TMS = 12 */
		};

devread(io)
	register struct iob *io;
{

	return((*devsw[io->i_ino.i_dev].dv_strategy)(io, READ));
}

devwrite(io)
	register struct iob *io;
{
	return((*devsw[io->i_ino.i_dev].dv_strategy)(io, WRITE));
}

devopen(io)
	register struct iob *io;
{
	return((*devsw[io->i_ino.i_dev].dv_open)(io));
}

devclose(io)
	register struct iob *io;
{
	(*devsw[io->i_ino.i_dev].dv_close)(io);
}

/*
 * Call the 'seek' entry for a tape device.  Seeking only works for 1kb
 * records - which is how the executables are stored - not for the dump
 * or tar files on a boot tape.
*/
devseek(io, space)
	register struct iob *io;
	int	space;
	{
	return((*devsw[io->i_ino.i_dev].dv_seek)(io, space));
	}

devlabel(io, fnc)
	register struct iob *io;
	int	fnc;
	{
	int	(*dvlab)() = devsw[io->i_ino.i_dev].dv_label;
	int	(*strat)() = devsw[io->i_ino.i_dev].dv_strategy;
	register struct disklabel *lp;
	register struct partition *pi;
	
	switch	(fnc)
		{
		case	WRITELABEL:
			return(writelabel(io, strat));
		case	READLABEL:
			return(readlabel(io, strat));
		case	DEFAULTLABEL:
/*
 * Zero out the label buffer and then assign defaults common to all drivers.
 * Many of these are rarely (if ever) changed.  The 'a' partition is set up
 * to be one sector past the label sector - the driver is expected to change
 * this to span the volume once the size is known.
*/
			lp = &io->i_label;
			pi = &lp->d_partitions[0];
			bzero(lp, sizeof (struct disklabel));
			lp->d_npartitions = 1;
			pi->p_offset = 0;
			pi->p_size = LABELSECTOR + 1;
			pi->p_fsize = DEV_BSIZE;
			pi->p_frag = 1;
			pi->p_fstype = FS_V71K;
			strcpy(lp->d_packname, "DEFAULT");
			lp->d_secsize = 512;
			lp->d_interleave = 1;
			lp->d_rpm = 3600;
/*
 * param.h declares BBSIZE to be DEV_BSIZE which is 1kb.  This is _wrong_,
 * the boot block size (what the bootroms read) is 512.  The disklabel(8)
 * program explicitly sets d_bbsize to 512 so we do the same thing here.
 *
 * What a mess - when the 1k filesystem was created there should have been
 * a (clearer) distinction made between '(hardware) sectors' and 
 * '(filesystem) blocks'.  Sigh.
*/
			lp->d_bbsize = 512;
			lp->d_sbsize = SBSIZE;
			return((*dvlab)(io));
		default:
			printf("devlabel: bad fnc %d\n");
			return(-1);
		}
	}

/*
 * Common routine to print out the full device name in the form:
 *
 *	dev(ctlr,unit,part)
 * 
 * Have to do it the hard way since there's no sprintf to call.  Register
 * oriented string copies are small though.
*/

char	*
devname(io)
	register struct iob *io;
	{
	static	char	dname[16];
	register char *cp, *dp;

	cp = dname;
	dp = devsw[io->i_ino.i_dev].dv_name;
	while	(*cp = *dp++)
		cp++;
	*cp++ = '(';
	dp = itoa(io->i_ctlr);
	while	(*cp = *dp++)
		cp++;
	*cp++ = ',';
	dp = itoa(io->i_unit);
	while	(*cp = *dp++)
		cp++;
	*cp++ = ',';
	dp = itoa(io->i_part);
	while	(*cp = *dp++)
		cp++;
	*cp++ = ')';
	*cp++ = '\0';
	return(dname);
	}
/*
 * Check for end of volume.  Actually this checks for end of partition.
 * Since this is almost always called when reading unlabeled disks (treating
 * a floppy as a short tape for example) it's effectively an EOV check.
*/

deveovchk(io)
	register struct iob *io;
	{
	register struct partition *pi;
	daddr_t  sz, eov;

	pi = &io->i_label.d_partitions[io->i_part];
	sz = io->i_cc / 512;
/*
 * i_bn already has the p_offset added in, thus we have to add in the partition
 * offset when calculating the end point. 
*/
	eov = pi->p_offset + pi->p_size;
	if	(io->i_bn + sz > eov)
		{
		sz = eov - io->i_bn;
		if	(sz == 0)
			return(0);	/* EOF */
/*
 * Probably should call this EOF too since there is no 'errno' to specify
 * what type of error has happened.
*/
		if	(sz < 0)
			return(-1);
		io->i_cc = dbtob(sz);
		}
	return(1);
	}

nullsys()
{
	return(-1);
}

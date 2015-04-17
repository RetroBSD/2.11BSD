/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dvhp.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * RP04/RP06 disk driver
 * Modified for Diva Comp V Controller - 33 SEC/TRACK
 */

#include <sys/param.h>
#include <sys/inode.h>
#include "saio.h"

struct	device
{
	union {
		int	w;
		char	c[2];
	} dvhpcs1;		/* Control and Status register 1 */
	int	dvhpwc;		/* Word count register */
	caddr_t	dvhpba;		/* UNIBUS address register */
	int	dvhpda;		/* Desired address register */
	union {
		int	w;
		char	c[2];
	} dvhpcs2;		/* Control and Status register 2*/
	int	dvhpds;		/* Drive Status */
	int	dvhper1;		/* Error register 1 */
	int	dvhpas;		/* Attention Summary */
	int	dvhpla;		/* Look ahead */
	int	dvhpdb;		/* Data buffer */
	int	dvhpmr;		/* Maintenance register */
	int	dvhpdt;		/* Drive type */
	int	dvhpsn;		/* Serial number */
	int	dvhpof;		/* Offset register */
	int	dvhpdc;		/* Desired Cylinder address register*/
	int	dvhpcc;		/* Current Cylinder */
	int	dvhper2;		/* Error register 2 */
	int	dvhper3;		/* Error register 3 */
	int	dvhpec1;		/* Burst error bit position */
	int	dvhpec2;		/* Burst error bit pattern */
	int	dvhpbae;		/* 11/70 bus extension */
	int	dvhpcs3;
};

int 	cyloff[] =		/* Cylinder offsets of logical drives */
{
	0,
	15,
	215,
	415,
	615,
	15,
	415,
	15,
};

#define	DVHPADDR	((struct device *)0176700)
#define	NSECT	33
#define	NTRAC	19
#define	SDIST	2
#define	RDIST	6

#define	P400	020
#define	M400	0220
#define	P800	040
#define	M800	0240
#define	P1200	060
#define	M1200	0260

#define	GO	01
#define	PRESET	020
#define	RTC	016
#define	OFFSET	014
#define	SEARCH	030
#define	RECAL	06
#define DCLR	010
#define	WCOM	060
#define	RCOM	070

#define	IE	0100
#define	PIP	020000
#define	DRY	0200
#define	ERR	040000
#define	TRE	040000
#define	DCK	0100000
#define	WLE	04000
#define	ECH	0100
#define VV	0100
#define FMT22	010000

dvhpstrategy(io, func)
register struct iob *io;
{
	register unit;
	register i;
	daddr_t bn;
	int sn, cn, tn;

	if (((unit = io->i_unit) & 0100) == 0)
		bn = io->i_bn;
	else {
		unit &= 03;
		bn = io->i_bn;
		bn -= io->i_boff;
		i = unit + 1;
		unit = bn%i;
		bn /= i;
		bn += io->i_boff;
	}

	DVHPADDR->dvhpcs2.w = (unit&070)>>3;

	if((DVHPADDR->dvhpds & VV) == 0) {
		DVHPADDR->dvhpcs1.c[0] = PRESET|GO;
		DVHPADDR->dvhpof = FMT22;
	}
	cn = bn/(NSECT*NTRAC) + cyloff[unit&07];
	sn = bn%(NSECT*NTRAC);
	tn = sn/NSECT;
	sn = sn%NSECT;

	DVHPADDR->dvhpdc = cn;
	DVHPADDR->dvhpda = (tn << 8) + sn;
	DVHPADDR->dvhpba = io->i_ma;
	DVHPADDR->dvhpwc = -(io->i_cc>>1);
	unit = (segflag << 8) | GO;
	if (func == READ)
		unit |= RCOM;
	else if (func == WRITE)
		unit |= WCOM;
	DVHPADDR->dvhpcs1.w = unit;
	while ((DVHPADDR->dvhpcs1.w&DRY) == 0)
			;
	if (DVHPADDR->dvhpcs1.w & TRE) {
		printf("disk error: cyl=%d track=%d sect=%d cs2=%o, er1=%o\n",
		    cn, tn, sn, DVHPADDR->dvhpcs2, DVHPADDR->dvhper1);
		return(-1);
	}
	return(io->i_cc);
}

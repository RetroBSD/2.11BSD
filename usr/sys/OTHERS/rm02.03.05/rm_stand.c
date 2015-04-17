/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rm.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * RM02/3 disk driver
 */

#include <sys/param.h>
#include <sys/inode.h>
#include "saio.h"

struct  device
{
	union {
		int     w;
		char    c[2];
	} rmcs1;		/* Control and Status register 1 */
	int     rmwc;	   /* Word count register */
	caddr_t rmba;	   /* UNIBUS address register */
	int     rmda;	   /* Desired address register */
	union {
		int     w;
		char    c[2];
	} rmcs2;		/* Control and Status register 2*/
	int     rmds;	   /* Drive Status */
	int     rmer1;	  /* Error register 1 */
	int     rmas;	   /* Attention Summary */
	int     rmla;	   /* Look ahead */
	int     rmdb;	   /* Data buffer */
	int     rmmr1;	  /* Maintenance register 1 */
	int     rmdt;	   /* Drive type */
	int     rmsn;	   /* Serial number */
	int     rmof;	   /* Offset register */
	int     rmdc;	   /* Desired Cylinder register*/
	int     rmhr;	   /* Holding register - unused*/
	int     rmmr2;	  /* Maintenence register 2 */
	int     rmer2;	  /* Error register 2 */
	int     rmec1;	  /* Burst error bit position */
	int     rmec2;	  /* Burst error bit pattern */
	int     rmbae;	  /* 11/70 bus extension */
	int     rmcs3;
};

#define RMADDR  ((struct device *)0176700)
#define NSECT   32
#define NTRAC   5
#define SDIST   2
#define RDIST   6

#define P400    020
#define M400    0220
#define P800    040
#define M800    0240
#define P1200   060
#define M1200   0260

#define GO      01
#define PRESET  020
#define RTC     016
#define OFFSET  014
#define SEARCH  030
#define RECAL   06
#define DCLR    010
#define WCOM    060
#define RCOM    070

#define IE      0100
#define PIP     020000
#define DRY     0200
#define ERR     040000
#define TRE     040000
#define DCK     0100000
#define WLE     04000
#define ECH     0100
#define VV      0100
#define FMT16   010000

rmstrategy(io, func)
register struct iob *io;
{
	register unit;
	register i;
	daddr_t bn;
	int sn, cn, tn;

	if (((unit = io->i_unit) & 04) == 0)
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

	RMADDR->rmcs2.w = unit;

	if((RMADDR->rmds & VV) == 0) {
		RMADDR->rmcs1.c[0] = PRESET|GO;
		RMADDR->rmof = FMT16;
	}
	cn = bn/(NSECT*NTRAC);
	sn = bn%(NSECT*NTRAC);
	tn = sn/NSECT;
	sn = sn%NSECT;

	RMADDR->rmdc = cn;
	RMADDR->rmda = (tn << 8) + sn;
	RMADDR->rmba = io->i_ma;
	RMADDR->rmwc = -(io->i_cc>>1);
	unit = (segflag << 8) | GO;
	if (func == READ)
		unit |= RCOM;
	else if (func == WRITE)
		unit |= WCOM;
	RMADDR->rmcs1.w = unit;
	while ((RMADDR->rmcs1.w&DRY) == 0)
			;
	if (RMADDR->rmcs1.w & TRE) {
		printf("disk error: cyl=%d track=%d sect=%d cs2=%o, er1=%o\n",
		    cn, tn, sn, RMADDR->rmcs2, RMADDR->rmer1);
		return(-1);
	}
	return(io->i_cc);
}

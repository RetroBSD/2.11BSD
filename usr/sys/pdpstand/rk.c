/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rk.c	2.2 (2.11BSD) 1996/3/8
 */

/*
 * RK disk driver
 */

#include "../h/param.h"
#include "../pdpuba/rkreg.h"
#include "saio.h"

#define	NRK	2

	struct	rkdevice *RKcsr[NRK + 1] =
		{
		(struct rkdevice *)0177400,
		(struct	rkdevice *)0,
		(struct rkdevice *)-1
		};

rkstrategy(io, func)
	register struct iob *io;
{
	register com;
	register struct rkdevice *rkaddr;
	daddr_t bn;
	int dn, cn, sn, bae, lo16;

	bn = io->i_bn;
	dn = io->i_unit;
	cn = bn/12;
	sn = bn%12;
	iomapadr(io->i_ma, &bae, &lo16);
	rkaddr = RKcsr[io->i_ctlr];
	rkaddr->rkda = (dn<<13) | (cn<<4) | sn;
	rkaddr->rkba = (caddr_t)lo16;
	rkaddr->rkwc = -(io->i_cc>>1);
	com = (bae<<4)|RKCS_GO;
	if (func == READ)
		com |= RKCS_RCOM;
	else
		com |= RKCS_WCOM;
	rkaddr->rkcs = com;
	while ((rkaddr->rkcs & RKCS_RDY) == 0)
		continue;
	if (rkaddr->rkcs<0) {	/* error bit */
		printf("%s err cy=%d sc=%d, er=%o, ds=%o\n",
		    devname(io), cn, sn, rkaddr->rker, rkaddr->rkds);
		return(-1);
	}
	return(io->i_cc);
}

rkopen(io)
	struct iob *io;
{
	return(genopen(NRK, io));
}

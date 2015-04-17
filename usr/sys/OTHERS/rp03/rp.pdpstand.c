/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rp.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * rp03 disk driver
 */

#include "../h/param.h"
#include "../h/inode.h"
#include "../pdpuba/rpreg.h"
#include "saio.h"

#define RPADDR ((struct rpdevice *) 0176710)


rpstrategy(io, func)
	register struct iob *io;
{
	int com, cn, tn, sn;


	cn = io->i_bn/(20*10);
	sn = io->i_bn%(20*10);
	tn = sn/10;
	sn = sn%10;
	RPADDR->rpcs.w = (io->i_unit<<8);
	RPADDR->rpda = (tn<<8) | sn;
	RPADDR->rpca = cn;
	RPADDR->rpba = io->i_ma;
	RPADDR->rpwc = -(io->i_cc>>1);
	com = (segflag<<4)|RP_GO;
	if (func == READ)
		com |= RP_RCOM;
	else
		com |= RP_WCOM;
	
	RPADDR->rpcs.w |= com;
	while ((RPADDR->rpcs.w & RP_RDY) == 0)
		continue;
	if (RPADDR->rpcs.w < 0) {	/* error bit */
		printf("disk error: cyl=%d track=%d sect=%d er=%o ds=%o\n",
		    cn, tn, sn, RPADDR->rper, RPADDR->rpds);
		return(-1);
	}
	return(io->i_cc);
}

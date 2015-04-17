/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)br.c	2.3 (2.11BSD) 1996/3/8
 */

/*
 * rp03-like disk driver
 *  	modified to handle BR 1537 and 1711 controllers with
 *	T300, T200, T80 and T50 drives.
 */

#include "../h/param.h"
#include "../pdpuba/brreg.h"
#include "saio.h"

#define	NBR	2
#define	SEC22	02400	/* T200 or T50 */
#define CYL5	01400	/* T80 or T50 */

	struct	brdevice *BRcsr[NBR + 1] =
		{
		(struct brdevice *)0176710,
		(struct brdevice *)0,
		(struct brdevice *)-1
		};

int	brsctrk[NBR][8], brtrkcyl[NBR][8];

brstrategy(io, func)
	register struct iob *io;
{
	register struct brdevice *braddr;
	register int ctlr;
	int com, cn, tn, sn, unit, sectrk, trkcyl, ctr, bae, lo16;

	unit = io->i_unit;
	ctlr = io->i_ctlr;
	braddr = BRcsr[ctlr];

	/* if we haven't gotten the characteristics yet, do so now. */
	trkcyl = brtrkcyl[ctlr][unit];
	if (!(sectrk = brsctrk[ctlr][unit])) {
		/* give a home seek command, then wait for complete */
		braddr->brcs.w = (unit << 8) | BR_HSEEK | BR_GO;
		ctr = 0;
		while ((braddr->brcs.w & BR_RDY) == 0 && --ctr)
			continue;
		if (braddr->brcs.w & BR_HE) {
			printf("%s !ready\n", devname(io));
			return(-1);
		}
		com = braddr->brae;
		if (com & SEC22)
			sectrk = 22;
		else
			sectrk = 32;
		if (com & CYL5)
			trkcyl = 5;
		else
			trkcyl = 19;
		brsctrk[ctlr][unit] = sectrk;
		brtrkcyl[ctlr][unit] = trkcyl;
	}
	cn = io->i_bn/(sectrk * trkcyl);
	sn = io->i_bn%(sectrk * trkcyl);
	tn = sn/sectrk;
	sn = sn%sectrk;

	iomapadr(io->i_ma, &bae, &lo16);
	braddr->brcs.w = (unit<<8);
	braddr->brda = (tn<<8) | sn;
	braddr->brca = cn;
	braddr->brba = (caddr_t)lo16;
	braddr->brwc = -(io->i_cc>>1);
	braddr->brae = bae;
	com = (bae<<4)|BR_GO;
	if (func == READ)
		com |= BR_RCOM;
	else
		com |= BR_WCOM;
	braddr->brcs.w |= com;
	while ((braddr->brcs.w& BR_RDY)==0)
		continue;
	if (braddr->brcs.w < 0) {	/* error bit */
		printf("%s err: cy=%d tr=%d sc=%d er=%o ds=%o\n",
		    devname(io), cn, tn, sn, braddr->brer, braddr->brds);
		return(-1);
	}
	return(io->i_cc);
}

bropen(io)
	struct iob *io;
{
	return(genopen(NBR, io));
}

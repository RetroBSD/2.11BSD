/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)si.c	2.2 (2.11BSD) 1996/3/8
 *
 *	SI 9500 CDC 9766 Stand Alone disk driver
 */

#include "../h/param.h"
#include "../pdpuba/sireg.h"
#include "saio.h"

#define	NSI	2

	struct	sidevice *SIcsr[NSI + 1] =
		{
		(struct sidevice *)0176700,
		(struct sidevice *)0,
		(struct sidevice *)-1
		};

/* defines for 9766 */
#define NHEADS	19
#define NSECT	32

#define NSPC	NSECT*NHEADS
static u_char dualsi[NSI];

sistrategy(io, func)
	register struct iob *io;
{
	int unit = io->i_unit;
	register int ctlr = io->i_ctlr;
	register struct sidevice *siaddr = SIcsr[ctlr];
	int ii;
	daddr_t bn;
	int sn, cn, tn, bae, lo16;

	/*
	 * weirdness with bit 2 (04) removed - see xp.c for comments
	 * about this
	*/
	bn = io->i_bn;
	cn = bn / (NSPC);
	sn = bn % (NSPC);
	tn = sn / (NSECT);
	sn = sn % (NSECT);

	if (!dualsi[ctlr]) {
		if (siaddr->siscr != 0)
			dualsi[ctlr]++;
		else
			if ((siaddr->sierr & (SIERR_ERR | SIERR_CNT)) == (SIERR_ERR | SIERR_CNT))
				dualsi[ctlr]++;
	}
	if (dualsi[ctlr])
		while (!(siaddr->siscr & 0200)) {
			siaddr->sicnr = SI_RESET;
			siaddr->siscr = 1;
		}
	iomapadr(io->i_ma, &bae, &lo16);
	siaddr->sipcr = cn + (unit <<10);
	siaddr->sihsr = (tn << 5) + sn;
	siaddr->simar = (caddr_t)lo16;
	siaddr->siwcr = io->i_cc >> 1;
	ii = (bae << 4) | SI_GO;
	if (func == READ)
		ii |= SI_READ;
	else if (func == WRITE)
		ii |= SI_WRITE;
	
	siaddr->sicnr = ii;

	while ((siaddr->sicnr & SI_DONE) == 0)
		continue;

	if (siaddr->sierr & SIERR_ERR) {
		printf("%s err cy=%d hd=%d sc=%d cnr=%o, err=%o\n",
			devname(io), cn, tn, sn, siaddr->sicnr, siaddr->sierr);
		return(-1);
	}
	return(io->i_cc);
}

siopen(io)
	struct iob *io;
{
	return(genopen(NSI, io));
}

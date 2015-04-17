/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ht.c	2.4 (2.11BSD) 1997/1/20
 */

/*
 * TM02/3 - TU16/TE16/TU77 standalone tape driver
 */

#include "../h/param.h"
#include "../pdpuba/htreg.h"
#include "saio.h"

#define	NHT		2
#define	H_NOREWIND	004		/* not used in stand alone driver */
#define	H_1600BPI	010

	struct	htdevice *HTcsr[NHT + 1] =
		{
		(struct htdevice *)0172440,
		(struct htdevice *)0,
		(struct htdevice *)-1
		};
extern int tapemark;	/* flag to indicate tapemark encountered
			   (see sys.c as to how it's used) */

htopen(io)
	register struct iob *io;
{
	register skip;
	register int ctlr = io->i_ctlr;

	if (genopen(NHT, io) < 0)
		return(-1);
	io->i_flgs |= F_TAPE;
	htstrategy(io, HT_REW);
	skip = io->i_part;
	while (skip--) {
		io->i_cc = 0;
		while (htstrategy(io, HT_SFORW))
			continue;
		delay(30000);
		htstrategy(io, HT_SENSE);
	}
	return(0);
}

htclose(io)
	struct iob *io;
{
	htstrategy(io, HT_REW);
}

/*
 * Copy the space logic from the open routine but add the check for spacing
 * backwards.
*/

htseek(io, space)
	struct	iob	*io;
	register int	space;
	{
	register int	fnc;

	if	(space < 0)
		{
		space = -space;
		fnc = HT_SREV;
		}
	else
		fnc = HT_SFORW;
	io->i_cc = space;
	htstrategy(io, fnc);
	delay(30000);
	htstrategy(io, HT_SENSE);
	}

/*
 * Returns 0 (and sets 'tapemark') if tape mark was seen.  Returns -1 on fatal
 * error.  Otherwise the length of data tranferred is returned.
*/

htstrategy(io, func)
	register struct iob *io;
{
	int unit, com;
	int errcnt, ctlr, bae, lo16, fnc;
	register struct htdevice *htaddr;

	unit = io->i_unit;
	ctlr = io->i_ctlr;
	htaddr = HTcsr[ctlr];
	errcnt = 0;
retry:
	while ((htaddr->htcs1 & HT_RDY) == 0)
		continue;
	while (htaddr->htfs & HTFS_PIP)
		continue;

	iomapadr(io->i_ma, &bae, &lo16);
	htaddr->httc =
		((io->i_unit&H_1600BPI) ? HTTC_1600BPI : HTTC_800BPI)
		| HTTC_PDP11 | unit;
	htaddr->htba = (caddr_t) lo16;
	htaddr->htfc = -io->i_cc;
	htaddr->htwc = -(io->i_cc >> 1);
	com = (bae << 8) | HT_GO;
	switch	(func)
		{
		case	READ:
			fnc = HT_RCOM;
			break;
		case	WRITE:
			fnc = HT_WCOM;
			break;
		default:
			fnc = func;
			break;
		}
	com |= fnc;
	htaddr->htcs1 = com;
	while ((htaddr->htcs1 & HT_RDY) == 0)
		continue;
	if (htaddr->htfs & HTFS_TM) {
		tapemark = 1;
		htinit(htaddr);
		return(0);
	}
	if (htaddr->htcs1 & HT_TRE) {
		if (errcnt == 0)
			printf("\n%s err: cs2=%o, er=%o",
			    devname(io), htaddr->htcs2, htaddr->hter);
		htinit(htaddr);
		if (errcnt++ == 10) {
			printf("\n(FATAL ERROR)\n");
			return(-1);
		}
		htstrategy(io, HT_SREV);
		goto retry;
	}
	return(io->i_cc+htaddr->htfc);
}

htinit(htaddr)
	register struct htdevice *htaddr;
{
	register int omt, ocs2;

	omt = htaddr->httc & 03777;
	ocs2 = htaddr->htcs2 & 07;

	htaddr->htcs2 = HTCS2_CLR;
	htaddr->htcs2 = ocs2;
	htaddr->httc = omt;
	htaddr->htcs1 = HT_DCLR|HT_GO;
}

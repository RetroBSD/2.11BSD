/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tm.c	2.3 (2.11BSD) 1997/1/19
 */

/*
 * TM11 - TU10/TE10/TS03 standalone tape driver
 */

#include "../h/param.h"
#include "../pdpuba/tmreg.h"
#include "saio.h"

#define	NTM	2

	struct	tmdevice *TMcsr[NTM + 1] =
		{
		(struct tmdevice *)0172520,
		(struct tmdevice *)0,
		(struct tmdevice *)-1
		};

extern int tapemark;	/* flag to indicate tapemark 
			has been encountered (see sys.c) 	*/

/*
 * Bits in device code.
 */
#define	T_NOREWIND	04		/* not used in stand alone driver */
#define	TMDENS(dev)	(((dev) & 030) >> 3)

tmclose(io)
	struct iob *io;
{
	tmstrategy(io, TM_REW);
}

tmopen(io)
	register struct iob *io;
{
	register skip;

	if (genopen(NTM, io) < 0)
		return(-1);
	io->i_flgs |= F_TAPE;
	tmstrategy(io, TM_REW);
	skip = io->i_part;
	while (skip--) {
		io->i_cc = 0;
		while (tmstrategy(io, TM_SFORW))
			continue;
	}
	return(0);
}

u_short tmdens[4] = { TM_D800, TM_D1600, TM_D6250, TM_D800 };

tmstrategy(io, func)
	register struct iob *io;
{
	int com, unit = io->i_unit;
	register struct tmdevice *tmaddr;
	int errcnt = 0, ctlr = io->i_ctlr, bae, lo16, fnc;

	tmaddr = TMcsr[ctlr];
retry:
	while ((tmaddr->tmcs&TM_CUR) == 0)
		continue;
	while ((tmaddr->tmer&TMER_TUR) == 0)
		continue;
	while ((tmaddr->tmer&TMER_SDWN) != 0)
		continue;
	iomapadr(io->i_ma, &bae, &lo16);
	com = (unit<<8)|(bae<<4) | tmdens[TMDENS(unit)];
	tmaddr->tmbc = -io->i_cc;
	tmaddr->tmba = (caddr_t)lo16;
	switch	(func)
		{
		case	READ:
			fnc = TM_RCOM;
			break;
		case	WRITE:
			fnc = TM_WCOM;
			break;
/*
 * Just pass all others thru - all other functions are TM_* opcodes and
 * had better be valid.
*/
		default:
			fnc = func;
			break;
		}
	tmaddr->tmcs = com | fnc | TM_GO;
	while ((tmaddr->tmcs&TM_CUR) == 0)
		continue;
	if (tmaddr->tmer&TMER_EOF) {
		tapemark=1;
		return(0);
	}
	if (tmaddr->tmer & TM_ERR) {
		if (errcnt == 0)
			printf("\n%s err: er=%o cs=%o",
				devname(io), tmaddr->tmer, tmaddr->tmcs);
		if (errcnt++ == 10) {
			printf("\n(FATAL ERROR)\n");
			return(-1);
		}
		tmstrategy(io, TM_SREV);
		goto retry;
	}
	return(io->i_cc+tmaddr->tmbc);
}

tmseek(io, space)
	register struct iob *io;
	int	space;
	{
	int	fnc;

	if	(space < 0)
		{
		fnc = TM_SREV;
		space = -space;
		}
	else
		fnc = TM_SFORW;
	io->i_cc = space;
	tmstrategy(io, fnc);
	return(0);
	}

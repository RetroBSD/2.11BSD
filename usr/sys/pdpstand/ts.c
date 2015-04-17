/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ts.c	2.3 (2.11BSD) 1996/3/8
 */

/*
 *	Stand-alone TS11/TU80/TS05/TK25 magtape driver.
 */

#include "../h/param.h"
#include "../pdpuba/tsreg.h"
#include "saio.h"

extern int tapemark;	/* flag to indicate tapemark encountered
			   (see sys.c as to how its used) */

#define	NTS	2

	struct	tsdevice *TScsr[NTS + 1] =
		{
		(struct tsdevice *)0172520,
		(struct tsdevice *)0,
		(struct tsdevice *)-1
		};

caddr_t tsptr[NTS];
struct	ts_char	chrbuf[NTS];		/* characteristics buffer */
struct	ts_sts	mesbuf[NTS];		/* message buffer */
struct	ts_cmd	*combuf[NTS];		/* command packet buffer */
char	softspace[(NTS * sizeof(struct ts_cmd)) + 3];

	/* bit definitions for Command mode field during read command */
#define	TS_RPREV	0400	/* read previous (reverse) */

tsopen(io)
	register struct iob *io;
{
	int	skip, bae, lo16;
	register struct tsdevice *tsaddr;
	register struct ts_char *chrb;
	struct ts_cmd *cmb;
	int ctlr = io->i_ctlr;
	char *cp;

	if (genopen(NTS, io) < 0)
		return(-1);
	io->i_flgs |= F_TAPE;
	tsaddr = TScsr[ctlr];

	/* combuf must be aligned on a mod 4 byte boundary */
	cp = (char *)((u_short)softspace + 3 & ~3);
	cp += (ctlr * sizeof (struct ts_cmd));
	cmb = combuf[ctlr] = (struct ts_cmd *)cp;

	iomapadr(cmb, &bae, &lo16);
	tsptr[ctlr] = (caddr_t)(lo16 | bae);
	cmb->c_cmd = (TS_ACK|TS_CVC|TS_INIT);
	tsaddr->tsdb = (u_short) tsptr[ctlr];
	while ((tsaddr->tssr & TS_SSR) == 0)
		continue;

	chrb = &chrbuf[ctlr];
	iomapadr(&mesbuf, &bae, &lo16);
	chrb->char_bptr = lo16;
	chrb->char_bae = bae;
	chrb->char_size = 016;
	chrb->char_mode = 0;

	cmb->c_cmd = (TS_ACK|TS_CVC|TS_SETCHR);
	iomapadr(&chrbuf, &bae, &lo16);
	cmb->c_loba = lo16;
	cmb->c_hiba = bae;
	cmb->c_size = 010;
	tsaddr->tsdb = (u_short) tsptr[ctlr];
	while ((tsaddr->tssr & TS_SSR) == 0)
		continue;

	tsstrategy(io, TS_REW);
	skip = io->i_part;
	while (skip--) {
		io->i_cc = 0;
		while (tsstrategy(io, TS_SFORWF))
			continue;
	}
	return(0);
}

tsclose(io)
	struct iob *io;
{
	tsstrategy(io, TS_REW);
}

tsstrategy(io, func)
	register struct iob *io;
{
	register int ctlr = io->i_ctlr;
	int	errcnt, unit = io->i_unit, bae, lo16;
	register struct tsdevice *tsaddr = TScsr[ctlr];

	errcnt = 0;
	iomapadr(io->i_ma, &bae, &lo16);
	combuf[ctlr]->c_loba = lo16;
	combuf[ctlr]->c_hiba = bae;
	combuf[ctlr]->c_size = io->i_cc;
	if (func == TS_SFORW || func == TS_SFORWF || func == TS_SREV || 
	    func == TS_SREVF)
		combuf[ctlr]->c_repcnt = 1;
	if (func == READ)
		combuf[ctlr]->c_cmd = TS_ACK|TS_RCOM;
	else if (func == WRITE)
		combuf[ctlr]->c_cmd = TS_ACK|TS_WCOM;
	else
		combuf[ctlr]->c_cmd = TS_ACK|func;
	tsaddr->tsdb = (u_short) tsptr[ctlr];
retry:
	while ((tsaddr->tssr & TS_SSR) == 0)
		continue;
	if (mesbuf[ctlr].s_xs0 & TS_TMK) {
		tapemark = 1;
		return(0);
	}
	if (tsaddr->tssr & TS_SC) {
		if (errcnt == 0)
		    printf("\n%s err sr=%o xs0=%o xs1=%o xs2=%o xs3=%o",
			devname(io), tsaddr->tssr,
			mesbuf[ctlr].s_xs0, mesbuf[ctlr].s_xs1,
			mesbuf[ctlr].s_xs2, mesbuf[ctlr].s_xs3);
		if (errcnt++ == 10) {
			printf("\n(FATAL ERROR)\n");
			return(-1);
		}
		if (func == READ)
			combuf[ctlr]->c_cmd = (TS_ACK|TS_RPREV|TS_RCOM);
		else if (func == WRITE)
			combuf[ctlr]->c_cmd = (TS_ACK|TS_RETRY|TS_WCOM);
		else {
			putchar('\n');
			return(-1);
		}
		tsaddr->tsdb = (u_short) tsptr[ctlr];
		goto retry;
	}
	return (io->i_cc+mesbuf[ctlr].s_rbpcr);
}

tsseek(io, space)
	register struct iob *io;
	int	space;
	{
	int	fnc;

	if	(space < 0)
		{
		fnc = TS_SREV;
		space = -space;
		}
	else
		fnc = TS_SFORW;
	while	(space--)
		{
		io->i_cc = 0;
		tsstrategy(io, fnc);
		}
	return(0);
	}

/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)lp.c	1.3 (2.11BSD GTE) 1997/2/14
 */

#include "lp.h"
#if NLP > 0
/*
 * LP-11 Line printer driver
 *
 * This driver has been modified to work on printers where
 * leaving LP_IE set would cause continuous interrupts.
 */
#include "param.h"
#include "systm.h"
#include "user.h"
#include "ioctl.h"
#include "tty.h"
#include "uio.h"
#include "kernel.h"

#define	LPPRI		(PZERO + 8)
#define	LP_IE		0100		/* interrupt enable */
#define	LP_RDY		0200		/* ready */
#define	LP_ERR		0100000		/* error */
#define	LPLWAT		40
#define	LPHWAT		400

#define	LPBUFSIZE	512
#define CAP		1
#ifndef LP_MAXCOL
#define LP_MAXCOL	132
#endif

#define LPUNIT(dev)	(minor(dev) >> 3)

struct lpdevice {
	short	lpcs;
	short	lpdb;
};

struct lp_softc {
	struct	clist sc_outq;
	int	sc_state;
	int	sc_physcol;
	int	sc_logcol;
	int	sc_physline;
	char	sc_flags;
	int	sc_lpchar;
} lp_softc[NLP];

struct lpdevice *lp_addr[NLP];

int lptout();

/* bits for state */
#define	OPEN		1	/* device is open */
#define	TOUT		2	/* timeout is active */
#define	MOD		4	/* device state has been modified */
#define	ASLP		8	/* awaiting draining of printer */

lpattach(addr, unit)
	struct lpdevice *addr;
	register u_int unit;
{
	if (unit >= NLP)
		return (0);
	lp_addr[unit] = addr;
	return (1);
}

/*ARGSUSED*/
lpopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct lp_softc *sc;
	register int unit, s;

	if ((unit = LPUNIT(dev)) >= NLP ||
	   (sc = &lp_softc[unit])->sc_state&OPEN ||
	   lp_addr[unit] == 0)
		return (ENXIO);
	if (lp_addr[unit]->lpcs&LP_ERR)
		return (EIO);
	sc->sc_state |= OPEN;
	sc->sc_flags = minor(dev) & 07;
	s = spl4();
	if ((sc->sc_state&TOUT) == 0) {
		sc->sc_state |= TOUT;
		timeout(lptout, (caddr_t)dev, 10*hz);
	}
	splx(s);
	lpcanon(dev, '\f');
	return (0);
}

/*ARGSUSED*/
lpclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct lp_softc *sc = &lp_softc[LPUNIT(dev)];

	lpcanon(dev, '\f');
	sc->sc_state &= ~OPEN;
}

lpwrite(dev, uio, flag)
	register dev_t dev;
	register struct uio *uio;
	int flag;
{
	register int n;
	register char *cp;
	char inbuf[LPBUFSIZE];
	int error;

	while (n = MIN(LPBUFSIZE, uio->uio_resid)) {
		cp = inbuf;
		error = uiomove(cp, (int)n, uio);
		if (error)
			return (error);
		do
			lpcanon(dev, *cp++);
		while (--n);
	}
	return (0);
}

lpcanon(dev, c)
	dev_t dev;
	register int c;
{
	struct lp_softc *sc = &lp_softc[LPUNIT(dev)];
	register int logcol, physcol, s;

	if (sc->sc_flags&CAP) {
		register c2;

		if (c >= 'a' && c <= 'z')
			c += 'A'-'a'; else
		switch (c) {

		case '{':
			c2 = '(';
			goto esc;

		case '}':
			c2 = ')';
			goto esc;

		case '`':
			c2 = '\'';
			goto esc;

		case '|':
			c2 = '!';
			goto esc;

		case '~':
			c2 = '^';

		esc:
			lpcanon(dev, c2);
			sc->sc_logcol--;
			c = '-';
		}
	}
	logcol = sc->sc_logcol;
	physcol = sc->sc_physcol;
	if (c == ' ')
		logcol++;
	else switch(c) {

	case '\t':
		logcol = (logcol + 8) & ~7;
		break;

	case '\f':
		if (sc->sc_physline == 0 && physcol == 0)
			break;
		/* fall into ... */

	case '\n':
		lpoutput(dev, c);
		if (c == '\f')
			sc->sc_physline = 0;
		else
			sc->sc_physline++;
		physcol = 0;
		/* fall into ... */

	case '\r':
		s = spl4();
		logcol = 0;
		lpintr(LPUNIT(dev));
		splx(s);
		break;

	case '\b':
		if (logcol > 0)
			logcol--;
		break;

	default:
		if (logcol < physcol) {
			lpoutput(dev, '\r');
			physcol = 0;
		}
		if (logcol < LP_MAXCOL) {
			while (logcol > physcol) {
				lpoutput(dev, ' ');
				physcol++;
			}
			lpoutput(dev, c);
			physcol++;
		}
		logcol++;
	}
	if (logcol > 1000)	/* ignore long lines  */
		logcol = 1000;
	sc->sc_logcol = logcol;
	sc->sc_physcol = physcol;
}

lpoutput(dev, c)
	dev_t dev;
	int c;
{
	register struct	lp_softc *sc = &lp_softc[LPUNIT(dev)];
	int s;

	if (sc->sc_outq.c_cc >= LPHWAT) {
		s = spl4();
		lpintr(LPUNIT(dev));				/* unchoke */
		while (sc->sc_outq.c_cc >= LPHWAT) {
			sc->sc_state |= ASLP;		/* must be LP_ERR */
			sleep((caddr_t)sc, LPPRI);
		}
		splx(s);
	}
	while (putc(c, &sc->sc_outq))
		sleep((caddr_t)&lbolt, LPPRI);
}

lpintr(lp11)
	int lp11;
{
	register int n;
	register struct lp_softc *sc = &lp_softc[lp11];
	register struct lpdevice *lpaddr = lp_addr[lp11];

	lpaddr->lpcs &= ~LP_IE;
	n = sc->sc_outq.c_cc;
	if (sc->sc_lpchar < 0)
		sc->sc_lpchar = getc(&sc->sc_outq);
	while ((lpaddr->lpcs & LP_RDY) && sc->sc_lpchar >= 0) {
		lpaddr->lpdb = sc->sc_lpchar;
		sc->sc_lpchar = getc(&sc->sc_outq);
	}
	sc->sc_state |= MOD;
	if (sc->sc_outq.c_cc > 0 && (lpaddr->lpcs&LP_ERR) == 0)
		lpaddr->lpcs |= LP_IE;		/* ok and more to do later */
	if (n>LPLWAT && sc->sc_outq.c_cc<=LPLWAT && sc->sc_state&ASLP) {
		sc->sc_state &= ~ASLP;
		wakeup((caddr_t)sc);		/* top half should go on */
	}
}

lptout(dev)
	dev_t dev;
{
	register struct lp_softc *sc;
	register struct lpdevice *lpaddr;

	sc = &lp_softc[LPUNIT(dev)];
	lpaddr = lp_addr[LPUNIT(dev)];
	if ((sc->sc_state&MOD) != 0) {
		sc->sc_state &= ~MOD;		/* something happened */
						/* so don't sweat */
		timeout(lptout, (caddr_t)dev, 2*hz);
		return;
	}
	if ((sc->sc_state&OPEN) == 0 && sc->sc_outq.c_cc == 0) {
		sc->sc_state &= ~TOUT;		/* no longer open */
		lpaddr->lpcs = 0;
		return;
	}
	if (sc->sc_outq.c_cc && (lpaddr->lpcs&LP_RDY) &&
	    (lpaddr->lpcs&LP_ERR)==0)
		lpintr(LPUNIT(dev));			/* ready to go */
	timeout(lptout, (caddr_t)dev, 10*hz);
}
#endif

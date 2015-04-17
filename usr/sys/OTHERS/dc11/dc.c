/*
 *	SCCS id	@(#)dc.c	2.1 (Berkeley)	8/5/83
 */

#include "dc.h"
#if	NDC > 0
#include "param.h"
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/dcreg.h>

struct	tty	dc11[NDC];
extern	struct	dcdevice *DCADDR;

/*
 * Input-side speed and control bit table.
 * Each DC11 has 4 speeds which correspond to the 4 non-zero entries.
 * The table index is the same as the speed-selector
 * number for the DH11.
 * Attempts to set the speed to a zero entry are ignored.
 */
short	dcrstab[] = {
	0,  			 				/* 0 baud */
	0,							/* 50 baud */
	0,							/* 75 baud */
	0,							/* 110 baud */
	DC_7BITS | DC_IE | DC_SPEED0 | DC_DTR,			/* 134.5 baud */
	DC_8BITS | DC_IE | DC_SPEED1 | DC_DTR,			/* 150 baud */
	0,							/* 200 baud */
	DC_8BITS | DC_IE | DC_SPEED2 | DC_DTR,			/* 300 baud */
	0,							/* 600 baud */
	DC_8BITS | DC_IE | DC_SPEED3 | DC_DTR,			/* 1200 baud */
	0,							/* 1800 baud */
	0,							/* 2400 baud */
	0,							/* 4800 baud */
	0,							/* 9600 baud */
	0,							/* X0 */
	0							/* X1 */
};

/*
 * Transmitter speed and control bit table
 */
short	dctstab[]	= {
	0,							/* 0 baud */
	0,							/* 50 baud */
	0,							/* 75 baud */
	0,							/* 110 baud */
	DCTCSR_STOP1 | DCTCSR_TIE | DC_SPEED0 | DCTCSR_RTS,	/* 134.5 baud */
	DCTCSR_STOP1 | DCTCSR_TIE | DC_SPEED1 | DCTCSR_RTS,	/* 150 baud */
	0,							/* 200 baud */
	DCTCSR_STOP1 | DCTCSR_TIE | DC_SPEED2 | DCTCSR_RTS,	/* 300 baud */
	0,							/* 600 baud */
	DCTCSR_STOP1 | DCTCSR_TIE | DC_SPEED3 | DCTCSR_RTS,	/* 1200 baud */
	0,							/* 1800 baud */
	0,							/* 2400 baud */
	0,							/* 4800 baud */
	0,							/* 9600 baud */
	0,							/* X0 */
	0							/* X1 */
};

/*
 * Open a DC11, waiting until carrier is established.
 * Default initial conditions are set up on the first open.
 * T_state's CARR_ON bit is a pure copy of the hardware
 * DC_CAR bit, and is only used to regularize
 * carrier tests in general tty routines.
 */
/*ARGSUSED*/
dcopen(dev, flag)
register dev_t	dev;
{
	register struct tty *tp;
	register struct dcdevice *dcaddr;
	extern klstart();
	int s;

	if (minor(dev) >= NDC) {
		u.u_error = ENXIO;
		return;
	}
	tp = &dc11[minor(dev)];
	dcaddr = DCADDR + minor(dev);
	tp->t_addr = (caddr_t)dcaddr;
	tp->t_state |= WOPEN;
	s = spl5();
	dcaddr->dcrcsr |= DC_IE | DC_DTR;
	if ((tp->t_state & ISOPEN) == 0) {
		tp->t_iproc = NULL;
		tp->t_oproc = klstart;	/* Yes, I know, but it works. */
		ttychars(tp);
		tp->t_ispeed = B300;
		tp->t_ospeed = B300;
		tp->t_flags = ODDP | EVENP | ECHO;
		dcaddr->dcrcsr = dcrstab[B300];
		dcaddr->dctcsr = dctstab[B300];
	}
	if (dcaddr->dcrcsr & DC_CAR)
		tp->t_state |= CARR_ON;
	splx(s);
	while ((tp->t_state & CARR_ON) == 0)
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	ttyopen(dev, tp);
}

/*
 * Close a DC11
 */
dcclose(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &dc11[minor(dev)];
	if (tp->t_state & HUPCLS)
		((struct dcdevice *) (tp->t_addr))->dcrcsr  & = ~DC_DTR;
	ttyclose(tp);
}

/*
 * Read a DC11
 */
dcread(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &dc11[minor(dev)];
	(*linesw[tp->t_line].l_read)(tp);
}

/*
 * Write a DC11
 */
dcwrite(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &dc11[minor(dev)];
	(*linesw[tp->t_line].l_write)(tp);
}

/*
 * DC11 transmitter interrupt.
 */
dcxint(dev)
dev_t	dev;
{
	register struct tty *tp;
	register struct clist *cp;

	tp = &dc11[minor(dev)];
	cp = &(tp->t_outq);
	ttstart(tp);
	if ((cp->c_cc == 0) || (cp->c_cc == TTLOWAT(tp)))
		wakeup((caddr_t) cp);
}

/*
 * DC11 receiver interrupt.
 */
dcrint(dev)
dev_t	dev;
{
	register struct tty *tp;
	register int c, csr;

	tp = &dc11[minor(dev)];
	c = ((struct dcdevice *) (tp->t_addr))->dcrbuf;

	/*
	 * If carrier is off, and an open is not in progress,
	 * knock down the CD lead to hang up the local dataset
	 * and signal a hangup.
	 */
	if (((csr = ((struct dcdevice *)(tp->t_addr))->dcrcsr) & DC_CAR) == 0) {
		if ((tp->t_state & WOPEN) == 0) {
			((struct dcdevice *) (tp->t_addr))->dcrcsr &= ~DC_DTR;
			if (tp->t_state & CARR_ON)
				signal(tp->t_pgrp, SIGHUP);
			flushtty(tp, FREAD|FWRITE);
		}
		tp->t_state &= ~CARR_ON;
		return;
	}
	if ((tp->t_state & ISOPEN) == 0) {
		if ((tp->t_state & WOPEN) && (csr & DC_CAR))
			tp->t_state |= CARR_ON;
		wakeup((caddr_t) tp);
		return;
	}
	if (csr & DC_BRK)
		if (tp->t_flags & RAW)
			c = 0;
		else
			c = 0177;
	csr &= DC_PCHK;
	if ((csr && ((tp->t_flags & (EVENP | ODDP)) == ODDP)) ||
	   (!csr && ((tp->t_flags & (EVENP | ODDP)) == EVENP)))
		(*linesw[tp->t_line].l_input)(c, tp);
}

/*
 * DC11 stty/gtty.
 * Perform general functions and set speeds.
 */
dcioctl(dev, cmd, addr, flag)
dev_t	dev;
caddr_t	addr;
{
	register struct tty *tp;
	register r;

	tp = &dc11[minor(dev)];
	if (ttioctl(cmd, tp, addr, dev, flag) == 0) {
		u.u_error = ENOTTY;
		return;
	}
	if (cmd == TIOCSETP) {
		r = dcrstab[tp->t_ispeed];
		if (r)
			((struct dcdevice *) (tp->t_addr))->dcrcsr = r;
		else
			((struct dcdevice *) (tp->t_addr))->dcrcsr &= ~DC_DTR;
		r = dctstab[tp->t_ospeed];
		((struct dcdevice *) (tp->t_addr))->dctcsr = r;
	}
	else
		u.u_error = ENOTTY;
}
#endif	NDC

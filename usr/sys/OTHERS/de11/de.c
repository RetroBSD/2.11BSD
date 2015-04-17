#include "de.h"
#if	NDE > 0
#include "param.h"
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/dereg.h>

#define	FREAD	01
#define	FWRITE	02
struct	tty	de11[NDE];
extern	struct	dedevice *DEADDR;
extern	char	partab[];
int destart();
int ttrstrt();

/*
 * DLV11E speed and control bit table.
 * The table index is the same as the speed-selector
 * number for the DH11.
 * Attempts to set the speed to a zero entry are ignored.
 */
short	derstab[] = {
	0,		 		 	/* 0 baud */
	0x0000  | DE_PBREN,			/* 50 baud */
	0x1000  | DE_PBREN,			/* 75 baud */
	0x2000  | DE_PBREN,			/* 110 baud */
	0x3000  | DE_PBREN,			/* 134.5 baud */
	0x4000  | DE_PBREN,			/* 150 baud */
	0x0000,					/* 200 baud-Not Available */
	0x5000  | DE_PBREN,			/* 300 baud */
	0x6000  | DE_PBREN,			/* 600 baud */
	0x7000  | DE_PBREN,			/* 1200 baud */
	0x8000  | DE_PBREN,			/* 1800 baud */
	0xA000  | DE_PBREN,			/* 2400 baud */
	0xC000  | DE_PBREN,			/* 4800 baud */
	0xE000	| DE_PBREN,			/* 9600 baud */
	0xF000	| DE_PBREN,			/* 19200 baud */
	0					/* X1 */
};

/*
 * Open a DLV11E, waiting until carrier is established.
 * Default initial conditions are set up on the first open.
 * T_state's CARR_ON bit is a copy of the hardware
 * DE_CAR bit, and is only used to regularize
 * carrier tests in general tty routines.
 */

/*ARGSUSED*/
deopen(dev, flag)
register dev_t	dev;
{
	register struct tty *tp;
	register struct dedevice *deaddr;
	extern klstart();
	int s;

	if (minor(dev) >= NDE) {
		u.u_error = ENXIO;
		return;
	}
	tp = &de11[minor(dev)];
	deaddr = DEADDR + minor(dev);
	if (tp->t_state & XCLUDE && u.u_uid != 0) {
		u.u_error = EBUSY;
		return;
	}
	tp->t_addr = (caddr_t)deaddr; /* Needed for klstart */
	if ((tp->t_state & (ISOPEN | WOPEN)) == 0) {
 /*					 Is dev already or being open(ed) ? */
		tp->t_iproc = NULL;
		tp->t_oproc = destart;
		ttychars(tp);		/* Sets default cntrl chars */
		tp->t_ispeed = B1200;
		tp->t_ospeed = B1200;
		tp->t_flags = ODDP | EVENP | ECHO;
		tp->t_line = DFLT_LDISC;
		deaddr->detcsr = derstab[B1200];
	}
#ifdef NODEMODEM
	if(dev & NODEMODEM) tp->t_state |= CARR_ON;
	else
#endif NODEMODEM
	{
	s = spl5();
	tp->t_state |= WOPEN;
	deaddr->dercsr |= DE_DSIE | DE_DTR; /* Enable int. for carrier det. */
	if (deaddr->dercsr & DE_CAR)		/* Copy the sofware state */
		tp->t_state |= CARR_ON; 	/* if carr. det already on */
	splx(s);
	}
	while ((tp->t_state & CARR_ON) == 0) {
/*
				If not on, wait for carrier-interrupt
*/
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
						}
	deaddr->dercsr |= DE_RIE;
	deaddr->detcsr |= DE_TIE;
	ttyopen(dev, tp);
}

/*
 * Close a DE11
 */
declose(dev)
dev_t	dev;
{
	register struct tty *tp;
	int s;

	tp = &de11[minor(dev)];
/*
				We can shut it down completely if
				was exclusive open
*/
if(tp->t_state & XCLUDE && u.u_uid != 0) {
		s = spl5();
		((struct dedevice *) (tp->t_addr))->dercsr = DE_DTR;
		((struct dedevice *) (tp->t_addr))->detcsr = 0;
		splx(s);
	}
	if (tp->t_state & HUPCLS) {
		((struct dedevice *) (tp->t_addr))->dercsr  &= ~DE_DTR;
	}
	ttyclose(tp);
}

/*
 * Read a DE11
 */
deread(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &de11[minor(dev)];
	(*linesw[tp->t_line].l_read)(tp);
}


/*
 * Write a DE11
 */
dewrite(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &de11[minor(dev)];
	(*linesw[tp->t_line].l_write)(tp);
}

/*
 * DE11 transmitter interrupt.
 */
dexint(dev)
dev_t	dev;
{
	register struct tty *tp;
	register struct clist *cp;
	tp = &de11[minor(dev)];
	ttstart(tp);
	if (tp->t_state & ASLEEP && tp->t_outq.c_cc <= TTLOWAT(tp)) {
		tp->t_state &= ~ASLEEP;
#ifdef MPX_FILS
/* to be added later */
HELP
#endif MPX_FILS
		wakeup((caddr_t) &tp->t_outq);
	}
}

/*
 * DE11 receiver interrupt.
 */
derint(dev)
dev_t	dev;
{
	register struct tty *tp;
	register int c, csr;

	tp = &de11[minor(dev)];
	c = ((struct dedevice *) (tp->t_addr))->derbuf;

	/*
	 * If carrier is off, and an open is not in progress,
	 * knock down the CD lead to hang up the local dataset
	 * and signal a hangup.
	 */
	if ((((csr = ((struct dedevice *)(tp->t_addr))->dercsr) & DE_CAR) == 0)
#ifdef NODEMODEM
	&& ( (tp->t_dev & NODEMODEM) == 0)	/* Skip it if NODEMODEM here */
#endif NODEMODEM
	)
	 {
		if ((tp->t_state & WOPEN) == 0) {
			((struct dedevice *) (tp->t_addr))->dercsr &= ~DE_DTR;
			if (tp->t_state & CARR_ON)
				gsignal(tp->t_pgrp, SIGHUP);
			flushtty(tp, FREAD|FWRITE);
		}
		tp->t_state &= ~CARR_ON;
		return;
	}
	if ((tp->t_state & ISOPEN) == 0) {
		if ((tp->t_state & WOPEN) && (csr & DE_CAR))
			tp->t_state |= CARR_ON;
		wakeup((caddr_t) tp);	/* Turn on software carr. if */
					/* back in deopen wait loop */
		return;
	}
/* Parity check to be fixed
	*	csr &= ((struct dedevice *)(tp->t_addr))->derbuf & 0400;
	*	if ((csr && ((tp->t_flags & (EVENP | ODDP)) == ODDP)) ||
	*	   (!csr && ((tp->t_flags & (EVENP | ODDP)) == EVENP)))
*/
	(*linesw[tp->t_line].l_input)(c, tp);
}

/*
 * DE11 stty/gtty.
 * Perform general functions and set speeds.
 */
deioctl(dev, cmd, addr, flag)
dev_t	dev;
caddr_t	addr;
int cmd, flag;
{
	register struct tty *tp;
	register r;

	tp = &de11[minor(dev)];
	switch (ttioctl(tp, cmd, addr, flag)) {
		case TIOCSETP:
		case TIOCSETN:
			r = derstab[tp->t_ispeed];
			if(tp->t_ispeed){	/* Hang up line */
			((struct dedevice *) (tp->t_addr))->detcsr = r;
	 		((struct dedevice *) (tp->t_addr))->dercsr |= DE_RIE;
			((struct dedevice *) (tp->t_addr))->detcsr |= DE_TIE;
			}
			else
			((struct dedevice *) (tp->t_addr))->dercsr &= ~DE_DTR;
			break;
#ifdef	DE_IOCTL
		case TIOCSBRK:
			((struct dedevice *) (tp->t_addr))->detcsr |= DE_BRK;
			break;
		case TIOCCBRK:
			((struct dedevice *) (tp->t_addr))->detcsr &= ~DE_BRK;
			break;
		case TIOCSDTR:
			((struct dedevice *) (tp->t_addr))->dercsr &= DE_DTR;
			break;
		case TIOCCDTR:
			((struct dedevice *) (tp->t_addr))->dercsr &= ~DE_DTR;
			break;
#endif DE_IOCTL
		default:
			u.u_error = ENOTTY;
		case 0:
			break;
	}
}

destart(tp)
register struct tty *tp;
{
	register c;
	register struct dedevice *addr;

	addr = (struct dedevice *) tp->t_addr;
	if ((addr->detcsr & DETCSR_RDY) == 0)
		return;
	if ((c=getc(&tp->t_outq)) >= 0) {
		if (tp->t_flags & RAW)
			addr->detbuf = c;
		else if (c<=0177)
			addr->detbuf = c
/* dlf terminal parity mod */
#ifdef OUT_PAR
 | (partab[c] & 0200)
#endif
;
/* end of dlf termnal parity mod */
		else {
			timeout(ttrstrt, (caddr_t)tp, (c & 0177) + DEDELAY);
			tp->t_state |= TIMEOUT;
		}
	}
}
#endif	NDE



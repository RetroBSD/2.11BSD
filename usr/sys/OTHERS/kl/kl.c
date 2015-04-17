/*
 *   KL/DL-11 driver
 *
 */
#include "kl.h"
#include "param.h"
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/klreg.h>

/*
 *	SCCS id	@(#)kl.c	2.1 (Berkeley)	9/1/83
 *
 *	modified 12/84 by keith packard to make a new
 *	minor device type that doesn't wait for
 *	carrier on open; useful for dial out modem lines
 *
 *	modified 11/84 by keith packard to handle modem control
 *	on dlv11e cards several more constants added to 
 *	/usr/include/sys/klreg.h
 *
 *	modified 9/84 by Keith Packard for use with dl11-e cards
 *	which have programmable baud rate selection.  Also see
 *	the modified /usr/include/sys/klreg.h which has another
 *	constant added for this change.  You must define
 *	MIN_SPEED, MAX_SPEED and MIN_MODEM, MAX_MODEM to select
 *	which devices have this feature by editing kl.h.
 */

#ifdef	MIN_MODEM
# define	KLNOWAIT	0100
# define	klunit(d)	(minor(d) & ~KLNOWAIT)
# define	klignore(d)	(minor(d) & KLNOWAIT)
int	klwait[MAX_MODEM - MIN_MODEM + 1];
/*
 *	KLIGNORE is set when the ignore-carrier device
 *	is opened.
 */
# define	KLIGNORE	1
/*
 *	KLSOFTCAR is set while the ignore-carrier device
 *	is actually ignoring carrier.  It stops ignoring
 *	carrier when carrier is detected - this way
 *	when carrier drops, the dial-out program can
 *	detect it.
 */
# define	KLSOFTCAR	2
/*
 *	KLWAIT is set when a process is waiting for the
 *	modem to become dial-in again
 */
# define	KLWAIT		4
#else	MIN_MODEM
# define	klunit(d)	minor(d)
#endif	MIN_MODEM

extern	struct	dldevice *KLADDR;
/*
 * Normal addressing:
 * minor 0 addresses KLADDR
 * minor 1 thru n-1 address from KLBASE (0176600),
 *    where n is the number of additional KL11's
 * minor n on address from DLBASE (0176500)
 */

struct	tty kl11[NKL];
int	nkl11	= NKL;		/* for pstat */
int	klstart();
int	ttrstrt();
extern	char	partab[];

klattach(addr, unit)
struct dldevice *addr;
{
	if ((unsigned) unit <= NKL) {
		kl11[unit].t_addr = addr;
		return(1);
	}
	return(0);
}

/*ARGSUSED*/
klopen(dev, flag)
dev_t	dev;
{
	register struct dldevice *addr;
	register struct tty *tp;
	register d;

	d = klunit(dev);
	tp = &kl11[d];
	if ((d == 0) && (tp->t_addr == 0))
		tp->t_addr = KLADDR;
	if ((d >= NKL) || ((addr = tp->t_addr) == 0)) {
		u.u_error = ENXIO;
		return;
	}
	tp->t_oproc = klstart;
	tp->t_state |= WOPEN;
	/*
	 *	set up default if the device is not open yet
	 */
	if ((tp->t_state & ISOPEN) == 0) {
		tp->t_flags = ODDP | EVENP | ECHO | XTABS | CRMOD;
		tp->t_line = DFLT_LDISC;
		tp->t_ispeed = B1200;
		tp->t_ospeed = B1200;
		ttychars(tp);
		klparam(d);
#ifdef	MIN_MODEM
	/*
	 *	don't allow both kinds of open (non-modem and modem)
	 *	on modem lines at the same time.
	 */
	} else if ((tp->t_state & XCLUDE || (tp->t_dev && dev != tp->t_dev))
			&& u.u_uid != 0) {
#else
	} else if (tp->t_state & XCLUDE && u.u_uid != 0) {
#endif
		u.u_error = EBUSY;
		return;
	}
#ifdef	MIN_MODEM
	addr->dlrcsr = DL_RIE | DL_DTR | DL_DIE | DL_RTS;
	kleopen (dev);
#else	MIN_MODEM
	tp->t_state |= CARR_ON;
	addr->dlrcsr |= DL_RIE | DL_DTR | DL_RE;
#endif	MIN_MODEM
	addr->dlxcsr |= DLXCSR_TIE;
	ttyopen(dev, tp);
}

/*ARGSUSED*/
klclose(dev, flag)
dev_t	dev;
int	flag;
{
	register struct tty *tp;
	register int unit;
	register struct dldevice *addr;
	int	s, j;

	unit = klunit(dev);
#ifdef	MIN_MODEM
	tp = &kl11[unit];
	addr = tp->t_addr;
	s = spl5 ();
	/*
	 *	wake up sleeping processes
	 *	that await the modem controlled line
	 *	line and don't reset the device if
	 *	someone is there waiting
	 */
	if (klignore (tp->t_dev) && MIN_MODEM <= unit && unit <= MAX_MODEM) {
		klwait [unit - MIN_MODEM] &= ~KLIGNORE;
		if ((klwait [unit - MIN_MODEM] & KLWAIT) != 0) {
			wakeup ((caddr_t) &klwait[unit - MIN_MODEM]);
			addr->dlrcsr = DL_RIE | DL_DTR | DL_DIE | DL_RTS;
			goto noreset;
		}
	}
	if (tp->t_state & HUPCLS)
		addr->dlrcsr &= ~(DL_DTR | DL_DIE | DL_RIE | DL_RTS);
	else
		addr->dlrcsr &= ~(DL_DIE | DL_RIE | DL_RTS);
	j = addr->dlrbuf;
noreset:	;
	splx (s);
	tp->t_dev = 0;
#endif	MIN_MODEM
	ttyclose(&kl11[unit]);
}

klread(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &kl11[klunit(dev)];
	(*linesw[tp->t_line].l_read)(tp);
}

klwrite(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &kl11[klunit(dev)];
	(*linesw[tp->t_line].l_write)(tp);
}

klxint(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &kl11[klunit(dev)];
	ttstart(tp);
	if (tp->t_state & ASLEEP && tp->t_outq.c_cc <= TTLOWAT(tp))
#ifdef	MPX_FILS
		if (tp->t_chan)
			mcstart(tp->t_chan, (caddr_t) &tp->t_outq);
		else
#endif
			wakeup((caddr_t) &tp->t_outq);
}

klrint(dev)
dev_t	dev;
{
	register int c;
	register struct dldevice *addr;
	register struct tty *tp;
	int unit, flags;
	int	s;

	unit = klunit(dev);
	tp = &kl11[unit];
	addr = (struct dldevice *) tp->t_addr;
#ifdef MIN_MODEM
	flags = addr->dlrcsr;
	c = addr->dlrbuf;
	if (MIN_MODEM <= unit && unit <= MAX_MODEM && (flags & DL_DSI)) {
		wakeup ((caddr_t)&tp->t_rawq);
		if (!(flags & DL_CARDET) && (tp->t_state & CARR_ON)) {
			/*
			 *	carrier dropped
			 */
#ifdef	UCB_NTTY
			if ((tp->t_state & WOPEN) == 0
			    && (tp->t_local & LMDMBUF)) {
				tp->t_state &= ~CARR_ON;
				tp->t_state |= TTSTOP;
			} else
#endif	UCB_NTTY
			if ((tp->t_state & WOPEN)==0
#ifdef	UCB_NTTY
			    && (tp->t_local & LNOHANG)==0
#endif	UCB_NTTY
			    /*
			     *	send this signal only if carrier
			     *	used to be up.
			     */
			    && (!(klwait[unit-MIN_MODEM] & KLSOFTCAR))) {
				tp->t_state &= ~CARR_ON;
				gsignal(tp->t_pgrp, SIGHUP);
				flushtty(tp, FREAD|FWRITE);
			}
		} else if ((flags & DL_CARDET) && !(tp->t_state & CARR_ON)) {
			/*
			 *	carrier up
			 */
			tp->t_state |= CARR_ON;
			/*
			 *	once we have seen a carrier,
			 *	stop ignoring it's transitions
			 */
			klwait[unit-MIN_MODEM] &= ~KLSOFTCAR;
#ifdef	UCB_NTTY
			if ((tp->t_state & WOPEN) == 0
			    && (tp->t_local & LMDMBUF)) {
				tp->t_state &= ~TTSTOP;
				ttstart (tp);
			}
#endif	UCB_NTTY
		} else if ((flags & DL_CTS) && (tp->t_state & TTSTOP)) {
			tp->t_state &= ~TTSTOP;
			ttstart (tp);
		} else if (!(flags & DL_CTS) && !(tp->t_state & TTSTOP))
			tp->t_state |= TTSTOP;
	}
	if (flags & DL_RDONE)
		(*linesw[tp->t_line].l_input)(c, tp);
#else	MIN_MODEM
	c = addr->dlrbuf;
	addr->dlrcsr |= DL_RE;
	(*linesw[tp->t_line].l_input)(c, tp);
#endif	MIN_MODEM
}

klioctl(dev, cmd, addr, flag)
caddr_t	addr;
dev_t	dev;
{
	register int	unit;
	register struct dldevice	*dl;
	int	s;

	unit = klunit(dev);
	switch (ttioctl(&kl11[unit], cmd, addr, flag)) {
#ifdef MIN_SPEED
		case TIOCSETN:
		case TIOCSETP:
			if (MIN_SPEED <= unit && unit <= MAX_SPEED)
				klparam (unit);
			break;
#endif MIN_SPEED
#ifdef MIN_MODEM
		case TIOCSDTR:
			if (MIN_MODEM <= unit && unit <= MAX_MODEM) {
				dl = (struct dldevice *) kl11[unit].t_addr;
				s = spl5 ();
				dl->dlrcsr |= DL_DTR;
				splx (s);
			} else
				u.u_error = ENOTTY;
			break;
		case TIOCCDTR:
			if (MIN_MODEM <= unit && unit <= MAX_MODEM) {
				dl = (struct dldevice *) kl11[unit].t_addr;
				s = spl5 ();
				dl->dlrcsr &= ~DL_DTR;
				splx (s);
			} else
				u.u_error = ENOTTY;
			break;
#endif MIN_MODEM
		case 0:
			break;
		default:
			u.u_error = ENOTTY;
	}
}

#ifdef MIN_SPEED
/*
 *	handle baud rate selection for DLV11-E boards
 *
 *	map dh baud rate selection to DLV11-E baud rate 
 *	selection
 */

int klbaudmap [] = {
	7,		/* 0    B0  = 1200*/
	0,		/* 1   B50  =   50*/
	1,		/* 2   B75  =   75*/
	2,		/* 3  B110  =  110*/
	3,		/* 4  B134  =  134*/
	4,		/* 5  B150  =  150*/
	7,		/* 6  B200  = 1200*/
	5,		/* 7  B300  =  300*/
	6,		/* 8  B600  =  600*/
	7,		/* 9  B1200 = 1200*/
	8,		/* 10 B1800 = 1800*/
	10,		/* 11 B2400 = 2400*/
	12,		/* 12 B4800 = 4800*/
	14,		/* 13 B9600 = 9600*/
	15,		/* 14 B19200=19200*/
	11,		/* 15 EXTB  = 3600*/
};

klparam (unit)
int	unit;
{
	register struct tty	*tp;
	register struct dldevice	*dl;
	int			s;
	register int		speed;

	tp = &kl11[unit];
	if (tp->t_ispeed != tp->t_ospeed) {
		u.u_error = ENOTTY;
		return;
	}
	dl = (struct dldevice *) tp->t_addr;
	speed = klbaudmap [tp->t_ospeed & 017];
	s = spl5();
	dl->dlxcsr = DLXCSR_TIE | DLXCSR_PSE | speed << 12;
	splx (s);
	return;
}
#endif MIN_SPEED

klstart(tp)
register struct tty *tp;
{
	register c;
	register struct dldevice *addr;
	int	s;

	addr = (struct dldevice *) tp->t_addr;
	if ((addr->dlxcsr & DLXCSR_TRDY) == 0)
		return;
	if ((c=getc(&tp->t_outq)) >= 0) {
		if (tp->t_state & CARR_ON) {
			if (c <= 0177 || (tp->t_flags & RAW) ||
			   (tp->t_local & LLITOUT))
				addr->dlxbuf = c;
			else {
				timeout(ttrstrt, (caddr_t)tp,
					(c & 0177) + DLDELAY);
				s = spl5 ();
				tp->t_state |= TIMEOUT;
				splx (s);
			}
		}
	}
}

char *msgbufp = msgbuf;		/* Next saved printf character */
/*
 * Print a character on console (or users terminal if touser).
 * Attempts to save and restore device
 * status.
 * If the last character input from the console was a break
 * (null or del), all printing is inhibited.
 *
 * Whether or not printing is inhibited,
 * the last MSGBUFS characters
 * are saved in msgbuf for inspection later.
 */

#ifdef	UCB_UPRINTF
putchar(c, touser)
#else
putchar(c)
#endif
register c;
{
	register s;
	register struct	dldevice *kladdr = KLADDR;
	long	timo;
	extern	char *panicstr;

#ifdef	UCB_UPRINTF
	if (touser) {
		register struct tty *tp = u.u_ttyp;
		if (tp && (tp->t_state & CARR_ON)) {
			register s = spl6();

			if (c == '\n')
				(*linesw[tp->t_line].l_output)('\r', tp);
			(*linesw[tp->t_line].l_output)(c, tp);
			ttstart(tp);
			splx(s);
		}
		return;
	}
#endif
	if (c != '\0' && c != '\r' && c != 0177) {
		*msgbufp++ = c;
		if (msgbufp >= &msgbuf[MSGBUFS])
			msgbufp	= msgbuf;
	}

	/*
	 *  If last char was a break or null, don't print
	 */
	if (panicstr == (char *) 0)
		if ((kladdr->dlrbuf & 0177) == 0)
			return;
	timo = 60000L;
	/*
	 * Try waiting for the console tty to come ready,
	 * otherwise give up after a reasonable time.
	 */
	while((kladdr->dlxcsr & DLXCSR_TRDY) == 0)
		if (--timo == 0L)
			break;
	if (c == 0)
		return;
	s = kladdr->dlxcsr;
	kladdr->dlxcsr = 0;
	kladdr->dlxbuf = c;
	if (c == '\n') {
#ifdef	UCB_UPRINTF
		putchar('\r', 0);
		putchar(0177, 0);
		putchar(0177, 0);
#else
		putchar('\r');
		putchar(0177);
		putchar(0177);
#endif
	}
#ifdef	UCB_UPRINTF
	putchar(0, 0);
#else
	putchar(0);
#endif
	kladdr->dlxcsr = s;
}

#ifdef	MIN_MODEM
/*
 * Turn on the line associated with the kl device dev.
 */
kleopen(dev)
dev_t	dev;
{
	register struct tty *tp;
	register struct dldevice *addr;
	register unit;
	int s, ignore;

	unit = klunit(dev);
	ignore = klignore (dev);
	tp = &kl11[unit];
	s = spl5();
	if (unit < MIN_MODEM || MAX_MODEM < unit) {
		tp->t_state |= CARR_ON;
		splx (s);
		return;
	}
	if (ignore)
		klwait[unit - MIN_MODEM] |= KLIGNORE;
	addr = tp->t_addr;
	if (addr->dlrcsr & DL_CARDET) {
carup:		;
		tp->t_state |= CARR_ON;
		splx (s);
		return;
	} else if (ignore) {
		/*
		 *	set the soft carrier bit,
		 *	while the carrier remains
		 *	down this bit will let
		 *	us talk to the device anyway
		 */
		klwait[unit - MIN_MODEM] |= KLSOFTCAR;
		goto carup;
	}
	tp->t_state &= ~CARR_ON;
	while ((tp->t_state & CARR_ON)==0) {
		sleep((caddr_t) &tp->t_rawq, TTIPRI);
		/*
		 *	wait for the line that doesn't ignore
		 *	carrier to open up, setting a bit
		 *	so that we get woken up when
		 *	the non-hanging line is closed
		 */
		while ((klwait [unit - MIN_MODEM] & KLIGNORE) != 0) {
			klwait [unit - MIN_MODEM] |= KLWAIT;
			sleep ((caddr_t) &klwait[unit - MIN_MODEM], TTIPRI);
			klwait [unit - MIN_MODEM] &= ~KLWAIT;
		}
	}
	splx(s);
}
#endif	MIN_MODEM

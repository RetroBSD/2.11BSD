/*
 *	SCCS id	@(#)dj.c	2.1 (Berkeley)	8/5/83
 */

#include "dj.h"
#if	NDJ > 0
#include "param.h"
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/tty.h>
#include <sys/djreg.h>

struct	tty	dj11[NDJ];
extern	struct	djdevice *dj_addr[];

/*ARGSUSED*/
djopen(dev, flag)
dev_t	dev;
{
	register d;
	register struct tty *tp;
	register struct	djdevice *djaddr;
	extern	djstart();

	d = minor(dev);
	if (d >= NDJ) {
		u.u_error = ENXIO;
		return;
	}
	tp = &dj11[d];
	if ((tp->t_state & XCLUDE) && (u.u_uid != 0)) {
		u.u_error = EBUSY;
		return;
	}
	tp->t_oproc = djstart;
	tp->t_iproc = NULL;
	tp->t_state |= WOPEN | CARR_ON;
	djaddr = dj_addr[d >> 4];
	djaddr->djcsr |= DJ_TIE | DJ_MTSE | DJ_RIE | DJ_RE;
	if ((tp->t_state & ISOPEN) == 0) {
		ttychars(tp);
		tp->t_ispeed = B300;
		tp->t_ospeed = B300;
		tp->t_flags = ODDP | EVENP | ECHO;
	}
	(*linesw[tp->t_line].l_open)(dev, tp);
}

djclose(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &dj11[minor(dev)];
	ttyclose(tp);
}

djread(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &dj11[minor(dev)];
	(*linesw[tp->t_line].l_read)(tp);
}

djwrite(dev)
dev_t	dev;
{
	register struct tty *tp;

	tp = &dj11[minor(dev)];
	(*linesw[tp->t_line].l_write)(tp);
}

djioctl(dev, cmd, addr, flag)
dev_t	dev;
caddr_t	addr;
{
	register struct tty *tp;

	tp = &dj11[minor(dev)];
	if (ttioctl(cmd, tp, addr, dev, flag) == 0)
		u.u_error = ENOTTY;
}

djrint(unit)
{
	register struct tty *tp;
	register struct djdevice *djaddr;
	register c;
	struct tty *tp0;

	djaddr = dj_addr[unit];
	tp0 = &dj11[unit << 4];
	while ((c = djaddr->djrbuf) < 0) {
		tp = tp0 + ((c >> 8) & 017);
		if (tp >= &dj11[NDJ])
			continue;
		if ((tp->t_state & ISOPEN) == 0) {
			wakeup((caddr_t) tp);
			continue;
		}
		if (c & DJRBUF_RDPE)
			if ((tp->t_flags & (EVENP | ODDP)) == EVENP ||
			   (tp->t_flags & (EVENP | ODDP)) == ODDP)
				continue;
		if (c & DJRBUF_FE)
			if (tp->t_flags & RAW)
				c = 0;
			else
				c = 0177;
		(*linesw[tp->t_line].l_input)(c, tp);
	}
}

djxint(unit)
{
	register struct tty *tp, *tp0;
	register struct djdevice *djaddr;

	djaddr = dj_addr[unit];
	tp0 = &dj11[unit << 4];
	while (djaddr->djcsr < 0) {
		tp = &tp0[djaddr->dj_un.djtbufh];
		djaddr->dj_un.djtbufl = tp->t_char;
		tp->t_state &= ~BUSY;
		djstart(tp);
	}
}

djstart(tp)
register struct tty *tp;
{
	register struct djdevice *djaddr;
	register unit;
	int	c, s;
	extern	ttrstrt();

	unit = tp - dj11;
	djaddr = dj_addr[unit >> 4];
	unit = 1 << (unit & 017);
	s = spl5();
	if (tp->t_state & (TIMEOUT | BUSY)) {
		splx(s);
		return;
	}
	if (tp->t_state & TTSTOP) {
		djaddr->djtcr &= ~unit;
		splx(s);
		return;
	}
	if ((c = getc(&tp->t_outq)) >= 0) {
		if (c >= 0200 && ((tp->t_flags & RAW) == 0)) {
			djaddr->djtcr &= ~unit;
			tp->t_state |= TIMEOUT;
			timeout(ttrstrt, (caddr_t) tp, (c & 0177) + 6);
		} else
			{
			tp->t_char = c;
			tp->t_state |= BUSY;
			djaddr->djtcr |= unit;
		}
		if ((tp->t_outq.c_cc <= TTLOWAT(tp)) && (tp->t_state & ASLEEP)) {
			tp->t_state &= ~ASLEEP;
				wakeup((caddr_t) &tp->t_outq);
		}
	} else
		djaddr->djtcr &= ~unit;
	splx(s);
}
#endif	NDJ

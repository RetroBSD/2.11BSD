/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dh.c	1.5 (2.11BSD GTE) 1997/6/12
 */

/*
 * DH11, DM11 device drivers
 */

#include "dh.h"
#if NDH > 0
/*
 * DH-11/DM-11 driver
 */
#include "param.h"
#include "conf.h"
#include "user.h"
#include "file.h"
#include "ioctl.h"
#include "tty.h"
#include "dhreg.h"
#include "dmreg.h"
#include "map.h"
#include "uba.h"
#include "ubavar.h"
#include "clist.h"
#include "systm.h"
#include "vm.h"
#include "kernel.h"
#include "syslog.h"
#include "proc.h"

int	dhtimer();
struct	uba_device dhinfo[NDH];
struct	uba_device dminfo[NDH];

#define	IFLAGS	(EVENP|ODDP|ECHO)

/*
 * Use 2 ticks rather than doing a divide of 'hz' by 30.  The old method
 * would produce a scan rate of 1 tick if the lineclock was 50hz but 2 ticks
 * if the lineclock was 60hz.
*/
#define	FASTTIMER	2	/* scan rate with silos on */

/*
 * Local variables for the driver
 */
short	dhsar[NDH];			/* software copy of last bar */

struct	tty dh11[NDH*16];
u_int	dh_overrun[NDH*16];		/* count of silo overruns, cleared on
					 * close.
					 */
int	ndh11	= NDH*16;
int	dhact;				/* mask of active dh's */
int	dhsilos;			/* mask of dh's with silo in use */
int	dhchars[NDH];			/* recent input count */
int	dhrate[NDH];			/* smoothed input count */
int	dhhighrate = 100;		/* silo on if dhchars > dhhighrate */
int	dhlowrate = 75;			/* silo off if dhrate < dhlowrate */
static short timerstarted;
int	dhstart();
static	int	dmtodh(), dhtodm();

#if defined(UCB_CLIST)
extern	ubadr_t	clstaddr;
#define	cpaddr(x)	(clstaddr + (ubadr_t)((x) - (char *)cfree))
#else
#define	cpaddr(x)	(x)
#endif

#define	UNIT(x)	(x & 0x3f)
#define	SOFTCAR	0x80
#define	HWFLOW	0x40

/*
 * Routine called to attach a dh.
 */
dhattach(addr, unit)
	register caddr_t addr;
	register u_int unit;
{
	register struct uba_device *ui;

	if (addr && unit < NDH && !dhinfo[unit].ui_addr) {
		ui = &dhinfo[unit];
		ui->ui_unit = unit;
		ui->ui_addr = addr;
		ui->ui_alive = 1;
		return (1);
	}
	return (0);
}

/*ARGSUSED*/
dmattach(addr, unit)
	register caddr_t addr;
	register u_int unit;
{
	register struct uba_device *ui;

	if (addr && unit < NDH && !dminfo[unit].ui_addr) {
		ui = &dminfo[unit];
		ui->ui_unit = unit;
		ui->ui_addr = addr;
		ui->ui_alive = 1;
		return (1);
	}
	return (0);
}

/*
 * Open a DH11 line.  Turn on this dh if this is
 * the first use of it.
 */
/*ARGSUSED*/
dhopen(dev, flag)
	dev_t dev;
	{
	register struct tty *tp;
	register struct dhdevice *addr;
	register int unit;
	struct	uba_device *ui;
	int	dh, s, error;

	unit = UNIT(dev);
	dh = unit >> 4;
	if	(unit >= NDH*16 || (ui = &dhinfo[dh])->ui_alive == 0)
		return(ENXIO);
	tp = &dh11[unit];
	addr = (struct dhdevice *)ui->ui_addr;
	tp->t_addr = (caddr_t)addr;
	tp->t_oproc = dhstart;

	if	(timerstarted == 0)
		{
		timerstarted++;
		timeout(dhtimer, (caddr_t) 0, hz);
		}
	if	((dhact&(1<<dh)) == 0)
		{
		addr->un.dhcsr |= DH_IE;
		dhact |= (1<<dh);
		addr->dhsilo = 0;
		}
	s = spltty();
	if	((tp->t_state & TS_ISOPEN) == 0)
		{
		tp->t_state |= TS_WOPEN;
		if	(tp->t_ispeed == 0)
			{
			tp->t_state |= TS_HUPCLS;
			tp->t_ispeed = B9600;
			tp->t_ospeed = B9600;
			tp->t_flags = IFLAGS;
			}
		ttychars(tp);
		tp->t_dev = dev;
		if	(dev & HWFLOW)
			tp->t_flags |= RTSCTS;
		else
			tp->t_flags &= ~RTSCTS;
		dhparam(unit);
		}
	else if	((tp->t_state & TS_XCLUDE) && u.u_uid)
		{
		error = EBUSY;
		goto out;
		}
	dmopen(dev);
	if	((dmctl(unit, 0, DMGET) & DML_CAR) || (dev & SOFTCAR))
		tp->t_state |= TS_CARR_ON;
	while	((tp->t_state & TS_CARR_ON) == 0 && !(flag & O_NONBLOCK))
		{
		tp->t_state |= TS_WOPEN;
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
		}
	error = (*linesw[tp->t_line].l_open)(dev, tp);
out:
	splx(s);
	return(error);
	}

/*
 * Close a DH line, turning off the DM11.
 */
dhclose(dev, flag)
	dev_t	dev;
	int	flag;
	{
	register struct tty *tp;
	register int	unit;

	unit = UNIT(dev);
	tp = &dh11[unit];
	if	((tp->t_state & (TS_WOPEN | TS_ISOPEN)) == 0)
		return(EBADF);		/* XXX */
	(*linesw[tp->t_line].l_close)(tp, flag);
	((struct dhdevice *)(tp->t_addr))->dhbreak &= ~(1<<(unit&017));
	dmctl(unit, DML_OFF, DMSET);
	ttyclose(tp);
	if	(dh_overrun[unit])
		{
		log(LOG_NOTICE, "dh%d %d overruns\n", dh_overrun[unit]);
		dh_overrun[unit] = 0;
		}
	return(0);
	}

dhselect(dev, rw)
	dev_t	dev;
	int	rw;
	{
	return(ttyselect(&dh11[UNIT(dev)], rw));
	}

dhread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
	{
	register struct tty *tp = &dh11[UNIT(dev)];

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
	}

dhwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	{
	register struct tty *tp = &dh11[UNIT(dev)];

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
	}

/*
 * DH11 receiver interrupt.
 */
dhrint(dh)
	int dh;
{
	register struct tty *tp;
	register int c;
	register struct dhdevice *addr;
	struct tty *tp0;
	int	line, p;

	addr = (struct dhdevice *)dhinfo[dh].ui_addr;
	if	(addr == 0)		/* Can't happen? */
		return;
	tp0 = &dh11[dh<<4];
	/*
	 * Loop fetching characters from the silo for this
	 * dh until there are no more in the silo.
	 */
	while ((c = addr->dhrcr) < 0) {
		line = (c >> 8) & 0xf;
		tp = tp0 + line;
		dhchars[dh]++;
		if ((tp->t_state&TS_ISOPEN)==0) {
			wakeup((caddr_t)&tp->t_rawq);
			continue;
		}
		if	(c & DH_PE)
			{
			p =  tp->t_flags & (EVENP|ODDP);
			if	(p == EVENP || p == ODDP)
				continue;
			}
		if	(c & DH_DO)
			{
			dh_overrun[(dh << 4) + line]++;
			continue;
			}
		if (c & DH_FE)
			/*
			 * At framing error (break) generate
			 * a null (in raw mode, for getty), or an
			 * interrupt (in cooked/cbreak mode).
			 */
			if (tp->t_flags & RAW)
				c = 0;
			else
#ifdef	OLDWAY
				c = tp->t_intrc;
#else
				c = tp->t_brkc;	/* why have brkc if not used? */
#endif
#if NBK > 0
		if (tp->t_line == NETLDISC) {
			c &= 0177;
			BKINPUT(c, tp);
		} else
#endif
			(*linesw[tp->t_line].l_rint)(c, tp);
	}
}

/*
 * Ioctl for DH11.
 */
/*ARGSUSED*/
dhioctl(dev, cmd, data, flag)
	dev_t	dev;
	u_int cmd;
	caddr_t data;
	int	flag;
	{
	register struct tty *tp;
	register unit = UNIT(dev);
	int	error, brkline;

	tp = &dh11[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if	(error >= 0)
		return(error);
	error = ttioctl(tp, cmd, data, flag);
	if	(error >= 0)
		{
		if	(cmd == TIOCSETP || cmd == TIOCSETN || 
			 cmd == TIOCLBIS || cmd == TIOCLBIC || cmd == TIOCLSET)
			dhparam(unit);
		return(error);
		}
	brkline = 1 << (unit & 0xf);
	switch	(cmd)
		{
		case	TIOCSBRK:
			((struct dhdevice *)(tp->t_addr))->dhbreak |= brkline;
			break;
		case	TIOCCBRK:
			((struct dhdevice *)(tp->t_addr))->dhbreak &= ~brkline;
			break;
		case	TIOCSDTR:
			(void)dmctl(unit, DML_DTR|DML_RTS, DMBIS);
			break;
		case	TIOCCDTR:
			(void)dmctl(unit, DML_DTR|DML_RTS, DMBIC);
			break;
		case	TIOCMSET:
			(void)dmctl(unit, dmtodh(*(int *)data, DMSET));
			break;
		case	TIOCMBIS:
			(void)dmctl(unit, dmtodh(*(int *)data, DMBIS));
			break;
		case	TIOCMBIC:
			(void)dmctl(unit, dmtodh(*(int *)data, DMBIC));
			break;
		case	TIOCMGET:
			*(int *)data = dhtodm(dmctl(unit, 0, DMGET));
			break;
		default:
			return(ENOTTY);
		}
	return(0);
	}

static	int
dmtodh(bits)
	register int bits;
	{
	register int	b = 0;

	if	(bits & TIOCM_RTS) b |= DML_RTS;
	if	(bits & TIOCM_DTR) b |= DML_DTR;
	if	(bits & TIOCM_LE) b |= DML_LE;
	return(b);
	}

static	int
dhtodm(bits)
	register int bits;
	{
	register int b = 0;

	if	(bits & DML_RNG) b |= TIOCM_RNG;
	if	(bits & DML_CAR) b |= TIOCM_CAR;
	if	(bits & DML_CTS) b |= TIOCM_CTS;
	if	(bits & DML_RTS) b |= TIOCM_RTS;
	if	(bits & DML_DTR) b |= TIOCM_DTR;
	if	(bits & DML_LE)  b |= TIOCM_LE;
	return(b);
	}

/*
 * Set parameters from open or stty into the DH hardware
 * registers.
 */
dhparam(unit)
	register int unit;
	{
	register struct tty *tp;
	register struct dhdevice *addr;
	register int lpar;
	int s;

	tp = &dh11[unit];
	addr = (struct dhdevice *)tp->t_addr;
	/*
	 * Block interrupts so parameters will be set
	 * before the line interrupts.
	 */
	s = spltty();
	addr->un.dhcsrl = (unit&0xf)|DH_IE;
	if	((tp->t_ispeed)==0)
		{
		tp->t_state |= TS_HUPCLS;
		dmctl(unit, DML_OFF, DMSET);
		goto out;
		}
	lpar = ((tp->t_ospeed)<<10) | ((tp->t_ispeed)<<6);
	if ((tp->t_ispeed) == B134)
		lpar |= BITS6|PENABLE|HDUPLX;
	else if (tp->t_flags & (RAW|LITOUT|PASS8))
		lpar |= BITS8;
	else
		lpar |= BITS7|PENABLE;
	if ((tp->t_flags&EVENP) == 0)
		lpar |= OPAR;
	if ((tp->t_ospeed) == B110)
		lpar |= TWOSB;
	addr->dhlpr = lpar;
out:
	splx(s);
	return(0);
	}

/*
 * DH transmitter interrupt.
 * Restart each line which used to be active but has
 * terminated transmission since the last interrupt.
 */
dhxint(dh)
	int dh;
{
	register struct tty *tp;
	register struct dhdevice *addr;
	short ttybit, bar, *sbar;
	register int unit;
	u_short cntr;
	ubadr_t car;
	struct dmdevice *dmaddr;

	addr = (struct dhdevice *)dhinfo[dh].ui_addr;
	if (addr->un.dhcsr & DH_NXM) {
		addr->un.dhcsr |= DH_CNI;
		log(LOG_NOTICE, "dh%d NXM\n", dh);
	}
	sbar = &dhsar[dh];
	bar = *sbar & ~addr->dhbar;
	unit = dh * 16; ttybit = 1;
	addr->un.dhcsr &= (short)~DH_TI;
	for(; bar; unit++, ttybit <<= 1) {
		if(bar & ttybit) {
			*sbar &= ~ttybit;
			bar &= ~ttybit;
			tp = &dh11[unit];
			tp->t_state &= ~TS_BUSY;
			if (tp->t_state&TS_FLUSH)
				tp->t_state &= ~TS_FLUSH;
			else {
				addr->un.dhcsrl = (unit&017)|DH_IE;
				/*
				 * Clists are either:
				 *	1)  in kernel virtual space,
				 *	    which in turn lies in the
				 *	    first 64K of physical memory or
				 *	2)  at UNIBUS virtual address 0.
				 *
				 * In either case, the extension bits are 0.
				 */
				car = (ubadr_t)addr->dhcar;
				if (!ubmap) {
#if defined(CS02)
					dmaddr = (struct dmdevice *)dminfo[dh].ui_addr;

					car |= ((ubadr_t)(dmaddr->dmlst_h&077) << 16);
#else
					car |= (ubadr_t)((addr->dhsilo & 0300) << 10);
#endif
				}
			cntr = car - cpaddr(tp->t_outq.c_cf);
			ndflush(&tp->t_outq, cntr);
			}
			if (tp->t_line)
				(*linesw[tp->t_line].l_start)(tp);
			else
				dhstart(tp);
		}
	}
}

/*
 * Start (restart) transmission on the given DH11 line.
 */
dhstart(tp)
	register struct tty *tp;
{
	register struct dhdevice *addr;
	register int dh, unit, nch;
	int s, csrl;
	ubadr_t uba;
	struct dmdevice *dmaddr;

	unit = UNIT(tp->t_dev);
	dh = unit >> 4;
	unit &= 0xf;
	addr = (struct dhdevice *)tp->t_addr;

	/*
	 * Must hold interrupts in following code to prevent
	 * state of the tp from changing.
	 */
	s = spltty();
	/*
	 * If it's currently active, or delaying, no need to do anything.
	 */
	if (tp->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	ttyowake(tp);		/* Wake up any sleepers */
	/*
	 * Now restart transmission unless the output queue is
	 * empty.
	 */
	if (tp->t_outq.c_cc == 0)
		goto out;
/*
 * This is where any per character delay handling would be done if ever
 * implemented again.  See the comments in dhv.c and dhu.c
*/
	nch = ndqb(&tp->t_outq, 0);
	/*
	 * If characters to transmit, restart transmission.
	 */
	if (nch) {
		uba = cpaddr(tp->t_outq.c_cf);
		csrl = (unit&017) | DH_IE;
		if (ubmap)
			addr->un.dhcsrl = (char)csrl;
		else {
#if defined(CS02)
			dmaddr = (struct dmdevice *)dminfo[dh].ui_addr;
			addr->un.dhcsrl = csrl;
			dmaddr->dmlst_h = hiint(uba) & 077;
#else
			addr->un.dhcsrl = csrl | DH_IE | ((hiint(uba)<<4)&060);
#endif
		}
		addr->dhcar = loint(uba);

		{ short word = 1 << unit;
		dhsar[dh] |= word;
		addr->dhbcr = -nch;
		addr->dhbar |= word;
		}
		tp->t_state |= TS_BUSY;
	}
out:
	splx(s);
}


/*
 * Stop output on a line, e.g. for ^S/^Q or output flush.
 */
/*ARGSUSED*/
dhstop(tp, flag)
	register struct tty *tp;
{
	register struct dhdevice *addr;
	register int unit, s;

	addr = (struct dhdevice *)tp->t_addr;
	/*
	 * Block input/output interrupts while messing with state.
	 */
	s = spltty();
	if (tp->t_state & TS_BUSY) {
		/*
		 * Device is transmitting; stop output
		 * by selecting the line and setting the byte
		 * count to -1.  We will clean up later
		 * by examining the address where the dh stopped.
		 */
		unit = UNIT(tp->t_dev);
		addr->un.dhcsrl = (unit&017) | DH_IE;
		if ((tp->t_state & TS_TTSTOP) == 0) {
			tp->t_state |= TS_FLUSH;
		}
		addr->dhbcr = -1;
	}
	splx(s);
}

int dhtransitions, dhslowtimers, dhfasttimers;		/*DEBUG*/
/*
 * At software clock interrupt time, check status.
 * Empty all the dh silos that are in use, and decide whether
 * to turn any silos off or on.
 */
dhtimer()
{
	register int dh, s;
	static int timercalls;

	if (dhsilos) {
		dhfasttimers++;		/*DEBUG*/
		timercalls++;
		s = spltty();
		for (dh = 0; dh < NDH; dh++)
			if (dhsilos & (1 << dh))
				dhrint(dh);
		splx(s);
	}
	if ((dhsilos == 0) || ((timercalls += FASTTIMER) >= hz)) {
		dhslowtimers++;		/*DEBUG*/
		timercalls = 0;
		for (dh = 0; dh < NDH; dh++) {
		    ave(dhrate[dh], dhchars[dh], 8);
		    if ((dhchars[dh] > dhhighrate) &&
		      ((dhsilos & (1 << dh)) == 0)) {
			((struct dhdevice *)(dhinfo[dh].ui_addr))->dhsilo =
			    (dhchars[dh] > 500? 32 : 16);
			dhsilos |= (1 << dh);
			dhtransitions++;		/*DEBUG*/
		    } else if ((dhsilos & (1 << dh)) &&
		      (dhrate[dh] < dhlowrate)) {
			((struct dhdevice *)(dhinfo[dh].ui_addr))->dhsilo = 0;
			dhsilos &= ~(1 << dh);
		    }
		    dhchars[dh] = 0;
		}
	}
	timeout(dhtimer, (caddr_t) 0, dhsilos ? FASTTIMER : hz);
}

/*
 * Turn on the line associated with dh dev.
 */
dmopen(dev)
	dev_t dev;
	{
	register struct tty *tp;
	register struct dmdevice *addr;
	register int unit;
	int	dm;

	unit = UNIT(dev);
	dm = unit >> 4;
	tp = &dh11[unit];
	if	(dm >= NDH || dminfo[dm].ui_alive == 0)
		{
		tp->t_state |= TS_CARR_ON;
		return;
		}
	(void)dmctl(unit, DML_ON, DMSET);
	}

/*
 * Dump control bits into the DM registers.
 */
dmctl(unit, bits, how)
	int unit;
	int bits, how;
	{
	register struct dmdevice *addr;
	register int s, mbits;
	int	dm;

	dm = unit >> 4;
	addr = (struct dmdevice *)dminfo[dm].ui_addr;
	if	(!addr)
		return(0);
	s = spltty();
	addr->dmcsr &= ~DM_SE;
	while	(addr->dmcsr & DM_BUSY)
		;
	addr->dmcsr = unit & 0xf;
	mbits = addr->dmlstat;

	switch	(how)
		{
		case	DMGET:
			break;		/* go re-enable scan */
		case	DMSET:
			mbits = bits;
			break;
		case	DMBIS:
			mbits |= bits;
			break;
		case	DMBIC:
			mbits &= ~bits;
			break;
		}
	addr->dmlstat = mbits;
	addr->dmcsr = DM_IE|DM_SE;
	splx(s);
	return(mbits);
	}

/*
 * DM interrupt; deal with carrier transitions.
 */
dmintr(dm)
	int	dm;
	{
	register struct uba_device *ui;
	register struct tty *tp;
	register struct dmdevice *addr;
	int unit;

	ui = &dminfo[dm];
	addr = (struct dmdevice *)ui->ui_addr;
	if	(addr->dmcsr&DM_DONE == 0)
		return;
	unit = addr->dmcsr & 0xf;
	tp = &dh11[(dm << 4) + unit];
	if	(addr->dmcsr & DM_CF)
		{
		if	(addr->dmlstat & DML_CAR)
			(void)(*linesw[tp->t_line].l_modem)(tp, 1);
		else if (!(tp->t_dev & SOFTCAR) &&
			  (*linesw[tp->t_line].l_modem)(tp, 0) == 0)
			addr->dmlstat = 0;
		}
	if	(addr->dmcsr & DM_CTS)
		{
		if	(tp->t_flags & RTSCTS)
			{
			if	(addr->dmlstat & DML_CTS)
				{
				tp->t_state &= ~TS_TTSTOP;
				ttstart(tp);
				}
			else
				{
				tp->t_state |= TS_TTSTOP;
				dhstop(tp, 0);
				}
			}
		}
	addr->dmcsr = DM_IE|DM_SE;
	}
#endif

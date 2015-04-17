/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dhu.c	2.3 (2.11BSD GTE) 1997/5/9
 */

/*
 * Rewritten for hardware flow control - sms 1997/5/9
 *
 * based on	dh.c 6.3	84/03/15
 * and on	dmf.c	6.2	84/02/16
 *
 * Dave Johnson, Brown University Computer Science
 *	ddj%brown@csnet-relay
 */

#include "dhu.h"
#if NDHU > 0
/*
 * DHU-11 driver
 */
#include "param.h"
#include "dhureg.h"
#include "conf.h"
#include "user.h"
#include "file.h"
#include "ioctl.h"
#include "tty.h"
#include "clist.h"
#include "map.h"
#include "proc.h"
#include "uba.h"
#include "ubavar.h"
#include "systm.h"
#include "syslog.h"
#include <sys/kernel.h>

struct	uba_device dhuinfo[NDHU];

#define	NDHULINE	(NDHU*16)

#define	UNIT(x)	(minor(x) & 077)
#define	SOFTCAR	0x80
#define	HWFLOW	0x40

#define ISPEED	B9600
#define IFLAGS	(EVENP|ODDP|ECHO)

/*
 * default receive silo timeout value -- valid values are 2..255
 * number of ms. to delay between first char received and receive interrupt
 *
 * A value of 20 gives same response as ABLE dh/dm with silo alarm = 0
 */
int	dhu_def_timo = 20;

/*
 * Baud rates: no 50, 200, or 38400 baud; all other rates are from "Group B".
 *	EXTA => 19200 baud
 *	EXTB => 2000 baud
 */
char	dhu_speeds[] =
	{ 0, 0, 1, 2, 3, 4, 0, 5, 6, 7, 8, 10, 11, 13, 14, 9 };

struct	tty dhu_tty[NDHULINE];
int	ndhu = NDHULINE;
int	dhuact;				/* mask of active dhu's */
int	dhu_overrun[NDHULINE];
int	dhustart();
long	dhumctl(),dmtodhu();
extern	int wakeup();

#if defined(UCB_CLIST)
extern	ubadr_t clstaddr;
#define	cpaddr(x)	(clstaddr + (ubadr_t)((x) - (char *)cfree))
#else
#define	cpaddr(x)	((u_short)(x))
#endif

/*
 * Routine called to attach a dhu.
 * Called duattached for autoconfig.
 */
duattach(addr,unit)
	register caddr_t addr;
	register u_int unit;
	{
	register struct uba_device *ui;

	if	(addr && unit < NDHU && !dhuinfo[unit].ui_addr)
		{
		ui = &dhuinfo[unit];
		ui->ui_unit = unit;
		ui->ui_addr = addr;
		ui->ui_alive = 1;
		return(1);
		}
	return(0);
	}

/*
 * Open a DHU11 line, mapping the clist onto the uba if this
 * is the first dhu on this uba.  Turn on this dhu if this is
 * the first use of it.
*/
/*ARGSUSED*/
dhuopen(dev, flag)
	dev_t dev;
	int	flag;
	{
	register struct tty *tp;
	int unit, dhu;
	register struct dhudevice *addr;
	register struct uba_device *ui;
	int s, error;

	unit = UNIT(dev);
	dhu = unit >> 4;
	if	(unit >= NDHULINE || (ui = &dhuinfo[dhu])->ui_alive == 0)
		return (ENXIO);
	tp = &dhu_tty[unit];
	addr = (struct dhudevice *)ui->ui_addr;
	tp->t_addr = (caddr_t)addr;
	tp->t_oproc = dhustart;

	if	((dhuact&(1<<dhu)) == 0)
		{
		addr->dhucsr = DHU_SELECT(0) | DHU_IE;
		addr->dhutimo = dhu_def_timo;
		dhuact |= (1<<dhu);
		/* anything else to configure whole board */
		}
	/*
	 * If this is first open, initialize tty state to default.
	 */
	s = spltty();
	if	((tp->t_state&TS_ISOPEN) == 0)
		{
		tp->t_state |= TS_WOPEN;
		if	(tp->t_ispeed == 0)
			{
			tp->t_state |= TS_HUPCLS;
			tp->t_ispeed = ISPEED;
			tp->t_ospeed = ISPEED;
			tp->t_flags = IFLAGS;
			}
		ttychars(tp);
		tp->t_dev = dev;
		if	(dev & HWFLOW)
			tp->t_flags |= RTSCTS;
		else
			tp->t_flags &= ~RTSCTS;
		dhuparam(unit);
		}
	else if (tp->t_state & TS_XCLUDE && u.u_uid)
		{
		error = EBUSY;
		goto out;
		}
/*
 * Turn the device on.  Wait for carrier (the wait is short if this is a
 * softcarrier/hardwired line ;-)).  Then do the line discipline specific open.
*/
	dhumctl(dev, (long)DHU_ON, DMSET);
	addr->dhucsr = DHU_SELECT(unit) | DHU_IE;
	if	((addr->dhustat & DHU_ST_DCD) || (dev & SOFTCAR))
		tp->t_state |= TS_CARR_ON;
	while	((tp->t_state & TS_CARR_ON) == 0 &&
		 (flag & O_NONBLOCK) == 0)
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
 * Close a DHU11 line.  Clear the 'break' state in case it's asserted and 
 * then drop DTR+RTS.
 */
/*ARGSUSED*/
dhuclose(dev, flag)
	dev_t dev;
	int flag;
	{
	register struct tty *tp;
	register int unit;

	unit = UNIT(dev);
	tp = &dhu_tty[unit];
/*
 * Do we need to do this?  Perhaps this should be ifdef'd.  I can't see how
 * this can happen...
*/
	if	(!(tp->t_state & TS_ISOPEN))
		return(EBADF);
	(*linesw[tp->t_line].l_close)(tp, flag);
	(void) dhumctl(unit, (long)DHU_BRK, DMBIC);
	(void) dhumctl(unit, DHU_OFF, DMSET);
	ttyclose(tp);
	if	(dhu_overrun[unit])
		{
		log(LOG_NOTICE, "dhu%d %d overruns\n", unit, dhu_overrun[unit]);
		dhu_overrun[unit] = 0;
		}
	return(0);
	}

dhuselect(dev, rw)
	dev_t	dev;
	int	rw;
	{

	return(ttyselect(&dhu_tty[UNIT(dev)], rw));
	}

dhuread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	{
	register struct tty *tp = &dhu_tty[UNIT(dev)];

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
	}

dhuwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	{
	register struct tty *tp = &dhu_tty[UNIT(dev)];

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
	}

/*
 * DHU11 receiver interrupt.
*/
dhurint(dhu)
	int dhu;
	{
	register struct tty *tp;
	register int	c;
	register struct dhudevice *addr;
	struct tty *tp0;
	struct uba_device *ui;
	int	line;

	ui = &dhuinfo[dhu];
	addr = (struct dhudevice *)ui->ui_addr;
	if	(!addr)
		return;
	tp0 = &dhu_tty[dhu<<4];
	/*
	 * Loop fetching characters from the silo for this
	 * dhu until there are no more in the silo.
	*/
	while	((c = addr->dhurbuf) < 0)
		{	/* (c & DHU_RB_VALID) == on */
		line = DHU_RX_LINE(c);
		tp = tp0 + line;
		if	((c & DHU_RB_STAT) == DHU_RB_STAT)
			{
			/*
			 * modem changed or diag info
			 */
			if	(c & DHU_RB_DIAG)
				{
				if	((c & 0xff) > 0201)
					log(LOG_NOTICE,"dhu%d diag %o\n",
						dhu, c);
				continue;
				}
			if	(!(tp->t_dev & SOFTCAR) ||
				  (tp->t_flags & MDMBUF))
				(*linesw[tp->t_line].l_modem)(tp,
						(c & DHU_ST_DCD) != 0);
			if	(tp->t_flags & RTSCTS)
				{
				if	(c & DHU_ST_CTS)
					{
					tp->t_state &= ~TS_TTSTOP;
					ttstart(tp);
					}
				else
					{
					tp->t_state |= TS_TTSTOP;
					dhustop(tp, 0);
					}
				}
			continue;
			}
		if	((tp->t_state&TS_ISOPEN) == 0)
			{
			wakeup((caddr_t)&tp->t_rawq);
			continue;
			}
		if	(c & DHU_RB_PE)
			if	((tp->t_flags&(EVENP|ODDP)) == EVENP ||
				 (tp->t_flags&(EVENP|ODDP)) == ODDP)
				continue;
		if	(c & DHU_RB_DO)
			{
			dhu_overrun[(dhu << 4) + line]++;
			/* bit bucket the silo to free the cpu */
			while	(addr->dhurbuf & DHU_RB_VALID)
				;
			break;
			}
		if	(c & DHU_RB_FE)
			{
			/*
			 * At framing error (break) generate
			 * a null (in raw mode, for getty), or a
			 * interrupt (in cooked/cbreak mode).
			 */
			if (tp->t_flags&RAW)
				c = 0;
			else
#ifdef	OLDWAY
				c = tp->t_intrc;
#else
				c = tp->t_brkc;
#endif
			}
#if NBK > 0
		if	(tp->t_line == NETLDISC)
			{
			c &= 0x7f;
			BKINPUT(c, tp);
			}
		else
#endif
			(*linesw[tp->t_line].l_rint)(c, tp);
		}
	}

/*
 * Ioctl for DHU11.
 */
/*ARGSUSED*/
dhuioctl(dev, cmd, data, flag)
	u_int cmd;
	caddr_t data;
	{
	register struct tty *tp;
	register int unit = UNIT(dev);
	int error;

	tp = &dhu_tty[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if	(error >= 0)
		return(error);
	error = ttioctl(tp, cmd, data, flag);
	if	(error >= 0)
		{
		if	(cmd == TIOCSETP || cmd == TIOCSETN || 
			 cmd == TIOCLSET || cmd == TIOCLBIC || cmd == TIOCLBIS)
			dhuparam(unit);
		return(error);
		}

	switch	(cmd)
		{
		case	TIOCSBRK:
			(void) dhumctl(unit, (long)DHU_BRK, DMBIS);
			break;
		case	TIOCCBRK:
			(void) dhumctl(unit, (long)DHU_BRK, DMBIC);
			break;
		case	TIOCSDTR:
			(void) dhumctl(unit, (long)DHU_DTR|DHU_RTS, DMBIS);
			break;
		case	TIOCCDTR:
			(void) dhumctl(unit, (long)DHU_DTR|DHU_RTS, DMBIC);
			break;
		case	TIOCMSET:
			(void) dhumctl(dev, dmtodhu(*(int *)data), DMSET);
			break;
		case	TIOCMBIS:
			(void) dhumctl(dev, dmtodhu(*(int *)data), DMBIS);
			break;
		case	TIOCMBIC:
			(void) dhumctl(dev, dmtodhu(*(int *)data), DMBIC);
			break;
		case	TIOCMGET:
			*(int *)data = dhutodm(dhumctl(dev, 0L, DMGET));
			break;
		default:
			return(ENOTTY);
		}
	return(0);
	}

static long
dmtodhu(bits)
	register int bits;
	{
	long b = 0;

	if	(bits & TIOCM_RTS) b |= DHU_RTS;
	if	(bits & TIOCM_DTR) b |= DHU_DTR;
	if	(bits & TIOCM_LE) b |= DHU_LE;
	return(b);
	}

static
dhutodm(bits)
	long bits;
	{
	register int b = 0;

	if	(bits & DHU_DSR) b |= TIOCM_DSR;
	if	(bits & DHU_RNG) b |= TIOCM_RNG;
	if	(bits & DHU_CAR) b |= TIOCM_CAR;
	if	(bits & DHU_CTS) b |= TIOCM_CTS;
	if	(bits & DHU_RTS) b |= TIOCM_RTS;
	if	(bits & DHU_DTR) b |= TIOCM_DTR;
	if	(bits & DHU_LE) b |= TIOCM_LE;
	return(b);
	}

/*
 * Set parameters from open or stty into the DHU hardware
 * registers.
 */
static
dhuparam(unit)
	register int unit;
	{
	register struct tty *tp;
	register struct dhudevice *addr;
	register int lpar;
	int s;

	tp = &dhu_tty[unit];
	addr = (struct dhudevice *)tp->t_addr;
	/*
	 * Block interrupts so parameters will be set
	 * before the line interrupts.
	 */
	s = spltty();
	if	(tp->t_ispeed == 0)
		{
		tp->t_state |= TS_HUPCLS;
		(void)dhumctl(unit, (long)DHU_OFF, DMSET);
		goto out;
		}
	lpar = (dhu_speeds[tp->t_ospeed]<<12) | (dhu_speeds[tp->t_ispeed]<<8);
	if	((tp->t_ispeed) == B134)
		lpar |= DHU_LP_BITS6|DHU_LP_PENABLE;
	else if (tp->t_flags & (RAW|LITOUT|PASS8))
		lpar |= DHU_LP_BITS8;
	else
		lpar |= DHU_LP_BITS7|DHU_LP_PENABLE;
	if	(tp->t_flags&EVENP)
		lpar |= DHU_LP_EPAR;
	if	((tp->t_ospeed) == B110)
		lpar |= DHU_LP_TWOSB;
	addr->dhucsr = DHU_SELECT(unit) | DHU_IE;
	addr->dhulpr = lpar;
out:
	splx(s);
	return;
	}

/*
 * DHU11 transmitter interrupt.
 * Restart each line which used to be active but has
 * terminated transmission since the last interrupt.
 */
dhuxint(dhu)
	int dhu;
	{
	register struct tty *tp;
	register struct dhudevice *addr;
	struct tty *tp0;
	struct uba_device *ui;
	register int line, t;
	u_short cntr;
	ubadr_t	base;

	ui = &dhuinfo[dhu];
	tp0 = &dhu_tty[dhu<<4];
	addr = (struct dhudevice *)ui->ui_addr;
	while	((t = addr->dhucsrh) & DHU_CSH_TI)
		{
		line = DHU_TX_LINE(t);
		tp = tp0 + line;
		tp->t_state &= ~TS_BUSY;
		if	(t & DHU_CSH_NXM)
			{
			log(LOG_NOTICE, "dhu%d,%d NXM\n", dhu, line);
			/* SHOULD RESTART OR SOMETHING... */
			}
		if	(tp->t_state&TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		else	
			{
			addr->dhucsrl = DHU_SELECT(line) | DHU_IE;
			base = (ubadr_t) addr->dhubar1;
			/*
			 * Clists are either:
			 *	1)  in kernel virtual space,
			 *	    which in turn lies in the
			 *	    first 64K of physical memory or
			 *	2)  at UNIBUS virtual address 0.
			 *
			 * In either case, the extension bits are 0.
			*/
			if	(!ubmap)
				base |= (ubadr_t)((addr->dhubar2 & 037) << 16);
			cntr = base - cpaddr(tp->t_outq.c_cf);
			ndflush(&tp->t_outq, cntr);
			}
		if	(tp->t_line)
			(*linesw[tp->t_line].l_start)(tp);
		else
			dhustart(tp);
		}
	}

/*
 * Start (restart) transmission on the given DHU11 line.
 */
dhustart(tp)
	register struct tty *tp;
	{
	register struct dhudevice *addr;
	register int unit, nch;
	ubadr_t car;
	int s;

	unit = UNIT(tp->t_dev);
	addr = (struct dhudevice *)tp->t_addr;

	s = spltty();
	/*
	 * If it's currently active, or delaying, no need to do anything.
	 */
	if	(tp->t_state&(TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	/*
	 * If there are sleepers, and output has drained below low
	 * water mark, wake up the sleepers..
	 */
	ttyowake(tp);

	/*
	 * Now restart transmission unless the output queue is
	 * empty.
	 */
	if	(tp->t_outq.c_cc == 0)
		goto out;
	addr->dhucsrl = DHU_SELECT(unit) | DHU_IE;
/*
 * If CTS is off and we're doing hardware flow control then mark the output
 * as stopped and do not transmit anything.
*/
	if	((addr->dhustat & DHU_ST_CTS) == 0 && (tp->t_flags & RTSCTS))
		{
		tp->t_state |= TS_TTSTOP;
		goto out;
		}
/*
 * This is where any per character delay handling for special characters 
 * would go if ever implemented again.  The call to ndqb would be replaced
 * with a scan for special characters and then the appropriate sleep/wakeup
 * done.
*/
	nch = ndqb(&tp->t_outq, 0);
	/*
	 * If characters to transmit, restart transmission.
	 */
	if	(nch)
		{
		car = cpaddr(tp->t_outq.c_cf);
		addr->dhucsrl = DHU_SELECT(unit) | DHU_IE;
		addr->dhulcr &= ~DHU_LC_TXABORT;
		addr->dhubcr = nch;
		addr->dhubar1 = loint(car);
		if	(ubmap)
			addr->dhubar2 = (hiint(car) & DHU_BA2_XBA) | DHU_BA2_DMAGO;
		else
			addr->dhubar2 = (hiint(car) & 037) | DHU_BA2_DMAGO;
		tp->t_state |= TS_BUSY;
		}
out:
	splx(s);
	}

/*
 * Stop output on a line, e.g. for ^S/^Q or output flush.
 */
/*ARGSUSED*/
dhustop(tp, flag)
	register struct tty *tp;
	{
	register struct dhudevice *addr;
	register int unit, s;

	addr = (struct dhudevice *)tp->t_addr;
	/*
	 * Block input/output interrupts while messing with state.
	 */
	s = spltty();
	if	(tp->t_state & TS_BUSY)
		{
		/*
		 * Device is transmitting; stop output
		 * by selecting the line and setting the
		 * abort xmit bit.  We will get an xmit interrupt,
		 * where we will figure out where to continue the
		 * next time the transmitter is enabled.  If
		 * TS_FLUSH is set, the outq will be flushed.
		 * In either case, dhustart will clear the TXABORT bit.
		 */
		unit = UNIT(tp->t_dev);
		addr->dhucsrl = DHU_SELECT(unit) | DHU_IE;
		addr->dhulcr |= DHU_LC_TXABORT;
		if	((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
		}
	(void) splx(s);
	}

/*
 * DHU11 modem control
 */
static long
dhumctl(dev, bits, how)
	dev_t dev;
	long bits;
	int how;
	{
	register struct dhudevice *dhuaddr;
	register int unit, line;
	long mbits;
	int s;

	unit = UNIT(dev);
	dhuaddr = (struct dhudevice *)(dhu_tty[unit].t_addr);
	line = unit & 0xf;
	s = spltty();
	dhuaddr->dhucsr = DHU_SELECT(line) | DHU_IE;
	/*
	 * combine byte from stat register (read only, bits 16..23)
	 * with lcr register (read write, bits 0..15).
	 */
	mbits = (u_short)dhuaddr->dhulcr | ((long)dhuaddr->dhustat << 16);
	switch	(how)
		{
		case	DMSET:
			mbits = (mbits & 0xff0000L) | bits;
			break;
		case	DMBIS:
			mbits |= bits;
			break;
		case	DMBIC:
			mbits &= ~bits;
			break;
		case	DMGET:
			goto out;
		}
	dhuaddr->dhulcr = (mbits & 0xffffL) | DHU_LC_RXEN;
	dhuaddr->dhulcr2 = DHU_LC2_TXEN;
out:
	(void) splx(s);
	return(mbits);
}
#endif

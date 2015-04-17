/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dz.c	1.4 (2.11BSD GTE) 1997/2/14
 */

/*
 * DZ11 device driver
 *
 * This driver mimics dh.c; see it for explanation of common code.
 */
#include "dz.h"

#if NDZ > 0
#include "param.h"
#include "user.h"
#include "file.h"
#include "conf.h"
#include "ioctl.h"
#include "tty.h"
#include "dzreg.h"
#include "pdma.h"
#include "proc.h"
#include "ubavar.h"
#include "vm.h"
#include "kernel.h"
#include "syslog.h"
#include "systm.h"

struct	uba_device dzinfo[NDZ];

#define	NDZLINE		(NDZ*8)
#define	FASTTIMER	2	/* rate to drain silos, when in use */

int	dzstart(), dzxint(), dzdma();
int	ttrstrt();
struct	tty dz_tty[NDZLINE];
int	dz_cnt = { NDZLINE };
int	dzact;
int	dzsilos;			/* mask of dz's with silo in use */
int	dzchars[NDZ];			/* recent input count */
int	dzrate[NDZ];			/* smoothed input count */
int	dztimerintvl;			/* time interval for dztimer */
int	dzhighrate = 100;		/* silo on if dzchars > dzhighrate */
int	dzlowrate = 75;			/* silo off if dzrate < dzlowrate */

#define dzwait(x)	while (((x)->dzlcs & DZ_ACK) == 0)

/*
 * Software copy of dzbrk since it isn't readable
 */
char	dz_brk[NDZ];
char	dzsoftCAR[NDZ];
char	dz_lnen[NDZ];	/* saved line enable bits for DZ32 */

/*
 * The dz11 doesn't interrupt on carrier transitions, so
 * we have to use a timer to watch it.
 */
char	dz_timer;		/* timer started? */

/*
 * Pdma structures for fast output code
 */
struct	pdma dzpdma[NDZLINE];

char	dz_speeds[] =
	{ 0,020,021,022,023,024,0,025,026,027,030,032,034,036,037,0 };
 
#ifndef	PORTSELECTOR
#define	ISPEED	B9600
#define	IFLAGS	(EVENP|ODDP|ECHO)
#else
#define	ISPEED	B4800
#define	IFLAGS	(EVENP|ODDP)
#endif

#define	UNIT(x)	(minor(x)&0177)

dzattach(addr, unit)
	caddr_t addr;
	u_int unit;
{
	extern dzscan();

	if (!addr || unit >= NDZ || dzinfo[unit].ui_addr)
		return (0);
	{
		register struct uba_device *ui;

		ui = &dzinfo[unit];
		ui->ui_unit = unit;
		ui->ui_addr = addr;
		ui->ui_alive = 1;
	}
	{
		register struct pdma *pdp = &dzpdma[unit*8];
		register struct tty *tp = &dz_tty[unit*8];
		register int cntr;

		for (cntr = 0; cntr < 8; cntr++) {
			pdp->pd_addr = (struct dzdevice *)addr;
			pdp->p_arg = tp;
			pdp++, tp++;
		}
	}
	if (dz_timer == 0) {
		dz_timer++;
		timeout(dzscan, (caddr_t)0, hz);
		dztimerintvl = FASTTIMER;
	}
	return (1);
}

/*ARGSUSED*/
dzopen(dev, flag)
	register dev_t dev;
{
	register struct tty *tp;
	register int unit;

	unit = UNIT(dev);
	if (unit >= NDZLINE || dzpdma[unit].pd_addr == 0)
		return (ENXIO);
	tp = &dz_tty[unit];
	tp->t_addr = (caddr_t)&dzpdma[unit];
	tp->t_oproc = dzstart;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
#ifndef PORTSELECTOR
		if (tp->t_ispeed == 0) {
#else
			tp->t_state |= TS_HUPCLS;
#endif PORTSELECTOR

			tp->t_ispeed = ISPEED;
			tp->t_ospeed = ISPEED;
			tp->t_flags = IFLAGS;
#ifndef PORTSELECTOR
		}
#endif PORTSELECTOR
		dzparam(unit);
	} else if (tp->t_state&TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);
	(void) dzmctl(dev, DZ_ON, DMSET);
#ifdef pdp11
	if (dev & 0200) {
		dzsoftCAR[unit >> 3] |= (1<<(unit&07));
		tp->t_state |= TS_CARR_ON;
	}
	else
		dzsoftCAR[unit >> 3] &= ~(1<<(unit&07));
#endif
	(void) _spl5();
	while ((tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	(void) _spl0();
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*ARGSUSED*/
dzclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register int unit;
	struct dzdevice *dzaddr;
	register int dz;

	unit = UNIT(dev);
	dz = unit >> 3;
	tp = &dz_tty[unit];
	(*linesw[tp->t_line].l_close)(tp, flag);
	dzaddr = dzpdma[unit].pd_addr;
	dzaddr->dzbrk = (dz_brk[dz] &= ~(1 << (unit&07)));
	if ((tp->t_state&(TS_HUPCLS|TS_WOPEN)) || (tp->t_state&TS_ISOPEN) == 0)
		(void) dzmctl(dev, DZ_OFF, DMSET);
	ttyclose(tp);
}

dzread(dev, uio, flag)
	register dev_t	dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp;

	tp = &dz_tty[UNIT(dev)];
	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

dzwrite(dev, uio, flag)
	register dev_t	dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp;

	tp = &dz_tty[UNIT(dev)];
	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

/*ARGSUSED*/
dzrint(dz)
	int dz;
{
	register struct tty *tp;
	register int c;
	register struct dzdevice *dzaddr;
	struct tty *tp0;
	register int unit;
	int overrun = 0;

	if ((dzact & (1<<dz)) == 0)
		return;
	unit = dz * 8;
	dzaddr = dzpdma[unit].pd_addr;
	tp0 = &dz_tty[unit];
	while ((c = dzaddr->dzrbuf) < 0) {	/* char present */
		dzchars[dz]++;
		tp = tp0 + ((c>>8)&07);
		if (tp >= &dz_tty[NDZLINE])
			continue;
		if ((tp->t_state & TS_ISOPEN) == 0) {
			wakeup((caddr_t)&tp->t_rawq);
#ifdef PORTSELECTOR
			if ((tp->t_state&TS_WOPEN) == 0)
#endif
				continue;
		}
		if (c&DZ_FE)
			if (tp->t_flags & RAW)
				c = 0;
			else
#ifdef	OLDWAY
				c = tp->t_intrc;
#else
				c = tp->t_brkc;
#endif
		if (c&DZ_DO && overrun == 0) {
			log(LOG_WARNING, "dz%d,%d: silo overflow\n", dz, (c>>8)&7);
			overrun = 1;
		}
		if (c&DZ_PE)
			if (((tp->t_flags & (EVENP|ODDP)) == EVENP)
			  || ((tp->t_flags & (EVENP|ODDP)) == ODDP))
				continue;
#if NBK > 0
		if (tp->t_line == NETLDISC) {
			c &= 0177;
			BKINPUT(c, tp);
		} else
#endif
			(*linesw[tp->t_line].l_rint)(c, tp);
	}
}

/*ARGSUSED*/
dzioctl(dev, cmd, data, flag)
	dev_t dev;
	u_int cmd;
	caddr_t data;
{
	register struct tty *tp;
	register int unit = UNIT(dev);
	int dz = unit >> 3;
	register struct dzdevice *dzaddr;
	register int error;

	tp = &dz_tty[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag);
	if (error >= 0) {
		if (cmd == TIOCSETP || cmd == TIOCSETN || cmd == TIOCLBIS ||
		    cmd == TIOCLBIC || cmd == TIOCLSET)
			dzparam(unit);
		return (error);
	}
	switch (cmd) {

	case TIOCSBRK:
		dzaddr = ((struct pdma *)(tp->t_addr))->pd_addr;
		dzaddr->dzbrk = (dz_brk[dz] |= 1 << (unit&07));
		break;

	case TIOCCBRK:
		dzaddr = ((struct pdma *)(tp->t_addr))->pd_addr;
		dzaddr->dzbrk = (dz_brk[dz] &= ~(1 << (unit&07)));
		break;

	case TIOCSDTR:
		(void) dzmctl(dev, DZ_DTR|DZ_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) dzmctl(dev, DZ_DTR|DZ_RTS, DMBIC);
		break;

	case TIOCMSET:
		(void) dzmctl(dev, dmtodz(*(int *)data), DMSET);
		break;

	case TIOCMBIS:
		(void) dzmctl(dev, dmtodz(*(int *)data), DMBIS);
		break;

	case TIOCMBIC:
		(void) dzmctl(dev, dmtodz(*(int *)data), DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = dztodm(dzmctl(dev, 0, DMGET));
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

static
dmtodz(bits)
	register int bits;
{
	register int b;

	b = (bits >>1) & 0370;
	if (bits & DML_ST) b |= DZ_ST;
	if (bits & DML_RTS) b |= DZ_RTS;
	if (bits & DML_DTR) b |= DZ_DTR;
	if (bits & DML_LE) b |= DZ_LE;
	return(b);
}

static
dztodm(bits)
	register int bits;
{
	register int b;

	b = (bits << 1) & 0360;
	if (bits & DZ_DSR) b |= DML_DSR;
	if (bits & DZ_DTR) b |= DML_DTR;
	if (bits & DZ_ST) b |= DML_ST;
	if (bits & DZ_RTS) b |= DML_RTS;
	return(b);
}

dzparam(unit)
	int unit;
{
	register struct tty *tp;
	register struct dzdevice *dzaddr;
	register int lpr;

	tp = &dz_tty[unit];
	dzaddr = dzpdma[unit].pd_addr;
	if (dzsilos & (1 << (unit >> 3)))
		dzaddr->dzcsr = DZ_IEN | DZ_SAE;
	else
		dzaddr->dzcsr = DZ_IEN;
	dzact |= (1<<(unit>>3));
	if (tp->t_ispeed == 0) {
		(void) dzmctl(unit, DZ_OFF, DMSET);	/* hang up line */
		return;
	}
	lpr = (dz_speeds[tp->t_ispeed]<<8) | (unit & 07);
	if (tp->t_flags & (RAW|LITOUT|PASS8))
		lpr |= BITS8;
	else
		lpr |= (BITS7|PENABLE);
	if ((tp->t_flags & EVENP) == 0)
		lpr |= OPAR;
	if (tp->t_ispeed == B110)
		lpr |= TWOSB;
	dzaddr->dzlpr = lpr;
}

dzxint(tp)
	register struct tty *tp;
{
	register struct pdma *dp;

	dp = (struct pdma *)tp->t_addr;
	tp->t_state &= ~TS_BUSY;
	if (tp->t_state & TS_FLUSH)
		tp->t_state &= ~TS_FLUSH;
	else {
		ndflush(&tp->t_outq, dp->p_mem-tp->t_outq.c_cf);
		dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	}
	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
	else
		dzstart(tp);
	if ((tp->t_outq.c_cc == 0) || (tp->t_state&TS_BUSY)==0)
		dp->pd_addr->dztcr &= ~(1 << (UNIT(tp->t_dev) & 07));
}

dzstart(tp)
	register struct tty *tp;
{
	register struct pdma *dp;
	struct dzdevice *dzaddr;
	register int cc;
	int s;

	dp = (struct pdma *)tp->t_addr;
	dzaddr = dp->pd_addr;
	s = spl5();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t) &tp->t_outq);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
	if (tp->t_flags & (RAW|LITOUT))
		cc = ndqb(&tp->t_outq, 0);
	else {
		cc = ndqb(&tp->t_outq, 0200);
		if (cc == 0) {
			cc = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, (cc&0x7f) + 6);
			tp->t_state |= TS_TIMEOUT;
			goto out;
		}
	}
	tp->t_state |= TS_BUSY;
	dp->p_end = dp->p_mem = tp->t_outq.c_cf;
	dp->p_end += cc;
	dzaddr->dztcr |= (1 << (UNIT(tp->t_dev) & 7));
out:
	splx(s);
}

/*
 * Stop output on a line.
 */
/*ARGSUSED*/
dzstop(tp, flag)
	register struct tty *tp;
{
	register struct pdma *dp;
	register int s;

	dp = (struct pdma *)tp->t_addr;
	s = spl5();
	if (tp->t_state & TS_BUSY) {
		dp->p_end = dp->p_mem;
		if ((tp->t_state & TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

static
dzmctl(dev, bits, how)
	dev_t dev;
	int bits, how;
{
	register struct dzdevice *dzaddr;
	register int unit, mbits;
	int b, s;

	unit = UNIT(dev);
	b = 1<<(unit&7);
	dzaddr = dzpdma[unit].pd_addr;
	s = spl5();
	mbits = (dzaddr->dzdtr & b) ? DZ_DTR : 0;
	mbits |= (dzaddr->dzmsr & b) ? DZ_CD : 0;
	mbits |= (dzaddr->dztbuf & b) ? DZ_RI : 0;
	switch (how) {
	case DMSET:
		mbits = bits;
		break;

	case DMBIS:
		mbits |= bits;
		break;

	case DMBIC:
		mbits &= ~bits;
		break;

	case DMGET:
		(void) splx(s);
		return(mbits);
	}
	if (mbits & DZ_DTR)
		dzaddr->dzdtr |= b;
	else
		dzaddr->dzdtr &= ~b;
	(void) splx(s);
	return(mbits);
}
 
int dztransitions, dzfasttimers;		/*DEBUG*/
dzscan()
{
	register i;
	register struct dzdevice *dzaddr;
	register bit;
	register struct tty *tp;
	register car;
	int olddzsilos = dzsilos;
	int dztimer();
 
	for (i = 0; i < NDZLINE; i++) {
		dzaddr = dzpdma[i].pd_addr;
		if (dzaddr == 0)
			continue;
		tp = &dz_tty[i];
		bit = 1<<(i&07);
		car = 0;
		if (dzsoftCAR[i>>3]&bit)
			car = 1;
		else if (dzaddr->dzcsr & DZ_32) {
			dzaddr->dzlcs = i&07;
			dzwait(dzaddr);
			car = dzaddr->dzlcs & DZ_CD;
		} else
			car = dzaddr->dzmsr&bit;
		if (car) {
			/* carrier present */
			if ((tp->t_state & TS_CARR_ON) == 0)
				(void)(*linesw[tp->t_line].l_modem)(tp, 1);
		} else if ((tp->t_state&TS_CARR_ON) &&
		    (*linesw[tp->t_line].l_modem)(tp, 0) == 0)
			dzaddr->dzdtr &= ~bit;
	}
	for (i = 0; i < NDZ; i++) {
		ave(dzrate[i], dzchars[i], 8);
		if (dzchars[i] > dzhighrate && ((dzsilos & (1 << i)) == 0)) {
			dzpdma[i << 3].pd_addr->dzcsr = DZ_IEN | DZ_SAE;
			dzsilos |= (1 << i);
			dztransitions++;		/*DEBUG*/
		} else if ((dzsilos & (1 << i)) && (dzrate[i] < dzlowrate)) {
			dzpdma[i << 3].pd_addr->dzcsr = DZ_IEN;
			dzsilos &= ~(1 << i);
		}
		dzchars[i] = 0;
	}
	if (dzsilos && !olddzsilos)
		timeout(dztimer, (caddr_t)0, dztimerintvl);
	timeout(dzscan, (caddr_t)0, hz);
}

dztimer()
{
	register int dz;
	register int s;

	if (dzsilos == 0)
		return;
	s = spl5();
	dzfasttimers++;		/*DEBUG*/
	for (dz = 0; dz < NDZ; dz++)
		if (dzsilos & (1 << dz))
		    dzrint(dz);
	splx(s);
	timeout(dztimer, (caddr_t) 0, dztimerintvl);
}
#endif

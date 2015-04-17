/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys_net.c	1.5 (2.11BSD GTE) 1997/2/16
 *
 * Print the csr of attached ethernet cards.  sms - 1997/2/16
 *
 * Initialize the supervisor mode 'hz' variable via a call from the kernel
 * rather compiling in a constant. sms - 1997/2/14
 *
 * Change uiomove calling convention.  The r/w type is now encapsulated
 * in the uio structure now. sms - 11/26/94
 *
 * 2.11BSD - map the I/O region with sufficient UMRs. this precludes
 * 	       drivers such as the DEUNA from allocating a UMR per packet.
 *	       sms - 9/8/90
 */

#include "param.h"
#include "../machine/cons.h"
#include "../machine/psl.h"

#include "user.h"
#include "uio.h"
#include "map.h"
#include "uba.h"
#include "mbuf.h"
#include "acct.h"
#include "ioctl.h"
#include "tty.h"

#include "../pdpuba/ubavar.h"

#include "acc.h"
#if NACC > 0
extern struct uba_driver accdriver;
#endif

#include "css.h"
#if NCSS > 0
extern struct uba_driver cssdriver;
#endif

#include "de.h"
#if NDE > 0
extern struct uba_driver dedriver;
#endif

#include "ec.h"
#if NEC > 0
extern struct uba_driver ecdriver;
#endif

#include "il.h"
#if NIL > 0
extern struct uba_driver ildriver;
#endif

#include "qe.h"
#if NQE > 0
extern struct uba_driver qedriver;
#endif

#include "qt.h"
#if NQT > 0
extern struct uba_driver qtdriver;
#endif

#include "sri.h"
#if NSRI > 0
extern struct uba_driver sridriver;
#endif

#include "vv.h"
#if NVV > 0
extern struct uba_driver vvdriver;
#endif

static struct uba_device ubdinit[] = {
#if NDE > 0
	{ &dedriver,	0,0, (caddr_t)0174510 },
#endif
#if NIL > 0
	{ &ildriver,	0,0, (caddr_t)0164000 },
#endif
#if NQE > 0
	{ &qedriver,	0,0, (caddr_t)0174440, 0, 0 },
#endif
#if NQE > 1
	{ &qedriver,	1,0, (caddr_t)0174460, 0, 0 },
#endif
#if NQT > 0
	{ &qtdriver,	0,0, (caddr_t)0174440, 0, 0 },
#endif
#if NQT > 1
	{ &qtdriver,	1,0, (caddr_t)0174460, 0, 0 },
#endif
#if NSRI > 0
	{ &sridriver,	0,0, (caddr_t)0167770 },
#endif
#if NVV > 0
	{ &vvdriver,	0,0, (caddr_t)0161000 },
#endif
#if NEC > 0
	{ &ecdriver,	0,0, (caddr_t)0164330 },
#endif
#if NACC > 0
	{ &accdriver,	0,0, (caddr_t)0000000 },
#endif
#if NCSS > 0
	{ &cssdriver,	0,0, (caddr_t)0000000 },
#endif
	NULL,
};

int hz;				/* kernel calls netsethz() to initialize */
long startnet;			/* start of network data space */

void
netsethz(ticks)
	int	ticks;
	{

	hz = ticks;
	}

netstart()
{
	extern memaddr miobase, miostart, netdata;
	extern ubadr_t mioumr;
	extern u_short miosize;
	register struct uba_driver *udp;
	register struct uba_device *ui = ubdinit;
	register int s;
	char	*attaching = "attaching ";
	int first;
	struct ubmap *ubp;
	ubadr_t paddr;

	/*
	 * The networking uses a mapped region as the DMA area for
	 * network interface drivers.  Allocate this latter region.
	 */
	if ((miobase = MALLOC(coremap, btoc(miosize))) == 0)
		panic("miobase");

	/*
	 * Allocate sufficient UMRs to map the DMA region.  Save the
	 * starting click and UNIBUS addresses for use in ubmalloc later.
	 * This is early in the systems life, so there had better be
	 * sufficient UMRs available!
	 */
	if (mfkd(&ubmap)) {
		miostart = miobase;
		s = (int)btoub(miosize);
		first = MALLOC(ub_map, s);
#ifdef	DIAGNOSTIC
		if	(!first)
			panic("ub_map");
#endif
		mioumr = (ubadr_t)first << 13;
		ubp = &UBMAP[first];
		paddr = ctob((ubadr_t)miostart);
		while	(s--) {
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			ubp++;
			paddr += (ubadr_t)UBPAGE;
		}
	}

	startnet = ctob((long)mfkd(&netdata));

	mbinit();
	for (ui = ubdinit; udp = ui->ui_driver; ++ui) {
		if (badaddr(ui->ui_addr, 2))
			continue;
		ui->ui_alive = 1;
		udp->ud_dinfo[ui->ui_unit] = ui;
		printf("%s%s%d csr %o\n", attaching,udp->ud_dname,ui->ui_unit, 
			ui->ui_addr);
		(*udp->ud_attach)(ui);
	}
#include "sl.h"
#if NSL > 0
	printf("%ssl\n", attaching);
	slattach();
#endif

#include "loop.h"
#if NLOOP > 0
	printf("%slo0\n", attaching);
	loattach();
#endif

	s = splimp();
	ifinit();
	domaininit();			/* must follow interfaces */
	splx(s);
}

/*
 * Panic is called on fatal errors.  It prints "net panic: mesg" and
 * then calls the kernel entry netcrash() to bring down the net.
 */
panic(s)
	register char *s;
{
	printf("net panic: %s\n", s);
	NETCRASH();
	/* NOTREACHED */
}

netcopyout(m, to, len)
	struct mbuf *m;
	char *to;
	int *len;
{
	if (*len > m->m_len)
		*len = m->m_len;
	return(copyout(mtod(m, caddr_t), to, *len));
}

/*
 * These routines are copies of various kernel routines that are required in
 * the supervisor space by the networking.  DO NOT MODIFY THESE ROUTINES!
 * Modify the kernel version and bring a copy here.  The "#ifdef notdef"
 * modifications simplify by eliminating portions of routines not required
 * by the networking.
 */

/* copied from kern_descrip.c */
ufavail()
{
	register int i, avail = 0;

	for (i = 0; i < NOFILE; i++)
		if (u.u_ofile[i] == NULL)
			avail++;
	return (avail);
}

/* copied from kern_descrip.c */
ufalloc(i)
	register int i;
{

	for (; i < NOFILE; i++)
		if (u.u_ofile[i] == NULL) {
			u.u_r.r_val1 = i;
			u.u_pofile[i] = 0;
			if (i > u.u_lastfile)
				u.u_lastfile = i;
			return (i);
		}
	u.u_error = EMFILE;
	return (-1);
}

/* copied from ufs_fio.c */
suser()
{

	if (u.u_uid == 0) {
		u.u_acflag |= ASU;
		return (1);
	}
	u.u_error = EPERM;
	return (0);
}

/* copied from kern_sysctl.c */
sysctl_int(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	int *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp && newlen != sizeof(int))
		return (EINVAL);
	*oldlenp = sizeof(int);
	if (oldp)
		error = copyout(valp, oldp, sizeof(int));
	if (error == 0 && newp)
		error = copyin(newp, valp, sizeof(int));
	return (error);
}

/*
 * An old version of uiomove, as uiofmove() and vcopy{in,out}() don't
 * exist in supervisor space.  Note, we assume that all transfers will
 * be to/from user D space.  Probably safe, until someone decides to
 * put NFS into the kernel.
 *
 * The 4.3BSD uio/iovec paradigm adopted, ureadc() and uwritec() inlined 
 * at that time to speed things up. 3/90 sms
 */
uiomove(cp, n, uio)
	caddr_t cp;
	u_int n;
	register struct uio *uio;
{
	register struct iovec *iov;
	int error, count, ch;
	register u_int cnt;

#ifdef DIAGNOSTIC
	if (uio->uio_segflg != UIO_USERSPACE)
		panic("net uiomove");
#endif
	while (n && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;
		count = cnt;
		if ((cnt | (int)cp | (int)iov->iov_base) & 1) {
			if (uio->uio_rw == UIO_READ) {
				while (cnt--)
					if (subyte(iov->iov_base++, *cp++) < 0)
						return (EFAULT);
			}
			else {
				while (cnt--) {
					if ((ch = fubyte(iov->iov_base++)) < 0)
						return (EFAULT);
					*cp++ = ch;
			 	}
			}
		cnt = count;	/* use register */
		}
		else {
			if (uio->uio_rw == UIO_READ)
				error = copyout(cp, iov->iov_base, cnt);
			else
				error = copyin(iov->iov_base, cp, cnt);
			if (error)
				return (error);
			iov->iov_base += cnt;
			cp += cnt;
		}
	iov->iov_len -= cnt;
	uio->uio_resid -= cnt;
	uio->uio_offset += cnt;
	n -= cnt;
	}
	return (0);
}

#define TOCONS	0x1
#define TOTTY	0x2
#define TOLOG	0x4

/* copied from subr_prf.c */
/*VARARGS1*/
printf(fmt, x1)
	char *fmt;
	unsigned x1;
{
	prf(fmt, &x1, TOCONS | TOLOG);
}

/* copied from subr_prf.c */
prf(fmt, adx, flags)
	register char *fmt;
	register u_int *adx;
	int flags;
{
	register int c;
	u_int b;
	char *s;
	int	i, any;

loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			return;
		_pchar(c, flags);
	}
	c = *fmt++;
	switch (c) {

	case 'l':
		c = *fmt++;
		switch(c) {
			case 'x':
				b = 16;
				goto lnumber;
			case 'd':
				b = 10;
				goto lnumber;
			case 'o':
				b = 8;
				goto lnumber;
			default:
				_pchar('%', flags);
				_pchar('l', flags);
				_pchar(c, flags);
		}
		break;
	case 'X':
		b = 16;
		goto lnumber;
	case 'D':
		b = 10;
		goto lnumber;
	case 'O':
		b = 8;
lnumber:	printn(*(long *)adx, b, flags);
		adx += (sizeof(long) / sizeof(int)) - 1;
		break;
	case 'x':
		b = 16;	
		goto number;
	case 'd':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
	case 'o':
		b = 8;
number:		printn((long)*adx, b, flags);
		break;
	case 'c':
		_pchar(*adx, flags);
		break;
	case 'b':
		b = *adx++;
		s = (char *)*adx;
		printn((long)b, *s++, flags);
		any = 0;
		if (b) {
			while (i = *s++) {
				if (b & (1 << (i - 1))) {
					_pchar(any? ',' : '<', flags);
					any = 1;
					for (; (c = *s) > 32; s++)
						_pchar(c, flags);
				} else
					for (; *s > 32; s++)
						;
			}
			if (any)
				_pchar('>', flags);
		}
		break;
	case 's':
		s = (char *)*adx;
		while (c = *s++)
			_pchar(c, flags);
		break;
	case '%':
		_pchar(c, flags);
		break;
	default:
		_pchar('%', flags);
		_pchar(c, flags);
		break;
	}
	adx++;
	goto loop;
}

/* copied from subr_prf.c */
printn(n, b, flags)
	long n;
	u_int b;
{
	char prbuf[12];
	register char *cp = prbuf;
	register int offset = 0;

	if (n<0)
		switch(b) {
		case 8:		/* unchecked, but should be like hex case */
		case 16:
			offset = b-1;
			n++;
			break;
		case 10:
			_pchar('-', flags);
			n = -n;
			break;
		}
	do {
		*cp++ = "0123456789ABCDEF"[offset + n%b];
	} while (n = n/b);	/* Avoid  n /= b, since that requires alrem */
	do
		_pchar(*--cp, flags);
	while (cp > prbuf);
}

extern int putchar();

_pchar(c, flg)
	int c, flg;
	{
	return(SKcall(putchar, sizeof(int)+sizeof(int)+sizeof(struct tty *),
		 c, flg, (struct tty *)0));
	}

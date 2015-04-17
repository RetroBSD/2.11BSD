/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vp.c	1.2 (2.11BSD GTE) 1/2/93
 */

/*
 * Versatec printer/plotter driver for model c-pdp11(dma) controller
 *
 * Thomas Ferrin, UCSF Computer Graphics Laboratory, April 1979
 *
 * Uses the same user interface as Bell Labs' driver but runs in raw
 * mode, since even the small 11" versatec outputs ~53kbytes/inch!
 */

#include "vp.h"
#if	NVP > 0
#include "param.h"
#include "user.h"
#include "buf.h"
#include "proc.h"
#include "vcmd.h"

/* #define NOMESG		/* no console messages if defined */
/* #define VP_TWOSCOMPL		/* use two's-complement bytecount */

#define	PRINTADR ((struct devregs *)0177514)	/* printer csr */
#define	PLOTADR  ((struct devregs *)0177510)	/* plotter csr */
/*
 * Note that there is really only 1 csr!  Changes to 'PRINTADR'
 * csr are reflected in 'PLOTADR' csr and vice versa. Referencing
 * the particular csr sets the device into that mode of operation.
 */
#define	DMA	((struct dmaregs *)0177500)	/* dma register base */

#define ERROR	0100000
#define DTCENABLE 040000	/* interrupt on data xfer complete */
#define	IENABLE	0100		/* interrupt on ready or error */
#define	READY	0200		/* previous operation complete */
#define REOT	010		/* remote end-of-transmission */
#define RESET	02
#define CMDS	076
#define SPP	01		/* simultaneous print/plot mode */

struct	buf	vpbuf;
#ifndef NOMESG
char *vperr = "\nVersatec needs attention\n";
#endif

struct devregs {
	short	csr;
};

struct dmaregs {
	short	plotbytes;
	short	addrext;
	short	printbytes;
	unsigned short	addr;
};

struct  {
	int	state;
	unsigned bytecount;
	int	alive;
} vp11;

/* states */
#define	PPLOT	0400	/* low 6 bits reserved for hardware functions */
#define	PLOT	0200
#define	PRINT	0100
#define MODE	0700
#define	ERRMSG	01000
#define NOSTART	02000
#define WBUSY	04000

/*
 *  This driver should be converted to use a single VPADDR pointer.
 *  Until then, the attach only marks the unit as alive.
 */
vpattach(addr, unit)
struct vpdevice *addr;
{
	if (((u_int) unit == 0) || (addr == (struct vpdevice *)DMA)) {
		vp11.alive++;
		return 1;
	}
	return 0;
}

/* ARGSUSED */
vpopen(dev, flag)
dev_t dev;
{
	register short *vpp;

	if (!vp11.alive || vp11.state&MODE)
		return(ENXIO);
	vpp = &PRINTADR->csr;
	vp11.state = PRINT;
	vpbuf.b_flags = B_DONE;
	*vpp = IENABLE | RESET;
	/*
	 * Contrary to the implication in the operator's manual, it is an
	 * error to have the bits for both an operation complete interrupt
	 * and a data transmission complete interrupt set at the same time.
	 * Doing so will cause erratic operation of 'READY' interrupts.
	 */
	if (vperror())
		*vpp = vp11.state = 0;
	return(0);
}

/* ARGSUSED */
vpclose(dev, flag)
dev_t dev;
{
	register short *vpp;

	vperror();
	vpp = (vp11.state&PLOT)? &PLOTADR->csr : &PRINTADR->csr;
	*vpp = IENABLE | REOT;	/* page eject past toner bath */
	vperror();
	*vpp = vp11.state = 0;
}

vpwrite(dev, uio)
dev_t dev;
struct uio *uio;
{
	int vpstart();

	if ((vp11.bytecount=uio->uio_resid) == 0)	/* true byte count */
		return;
	if (uio->uio_resid&01)	/* avoid EFAULT error in physio() */
		uio->uio_resid++;
	if (vperror())
		return;
	return (physio(vpstart, &vpbuf, dev, B_WRITE, uio));
	/* note that newer 1200A's print @ 1000 lines/min = 2.2kbytes/sec */
}

#ifdef V6
vpctl(dev, v)	
dev_t dev;
int *v;
{
	register int ctrl;
	register short *vpp;

	if (v) {
		*v = vp11.state;
		return;
	}
	if (vperror())
		return;
	ctrl = u.u_arg[0];
	vp11.state = (vp11.state&~MODE) | (ctrl&MODE);
	vpp = (ctrl&PLOT)? &PLOTADR->csr : &PRINTADR->csr;
	if (ctrl&CMDS) {
		*vpp = IENABLE | (ctrl&CMDS) | (vp11.state&SPP);
		vperror();
	}
	if (ctrl&PPLOT) {
		vp11.state |= SPP;
		*vpp = DTCENABLE | SPP;
	} else if (ctrl&PRINT) {
		vp11.state &= ~SPP;
		*vpp = IENABLE;
	} else
		*vpp = IENABLE | (vp11.state&SPP);
}
#else

/* ARGSUSED */
vpioctl(dev, cmd, data, flag)
	dev_t dev;
	u_int cmd;
	register caddr_t data;
	int flag;
{
	register int ctrl;
	register short *vpp;
	register struct buf *bp;
	struct	uio	uio;
	struct	iovec	iov;

	switch (cmd) {

	case GETSTATE:
		*(int *)data = vp11.state;
		break;

	case SETSTATE:
		ctrl = *(int *)data;
		if (vperror())
			return;
		vp11.state = (vp11.state&~MODE) | (ctrl&MODE);
		vpp = (ctrl&PLOT)? &PLOTADR->csr : &PRINTADR->csr;
		if (ctrl&CMDS) {
			*vpp = IENABLE | (ctrl&CMDS) | (vp11.state&SPP);
			vperror();
		}
		if (ctrl&PPLOT) {
			vp11.state |= SPP;
			*vpp = DTCENABLE | SPP;
		} else if (ctrl&PRINT) {
			vp11.state &= ~SPP;
			*vpp = IENABLE;
		} else
			*vpp = IENABLE | (vp11.state&SPP);
		return (0);

#ifdef THIS_IS_NOT_USEABLE
	case BUFWRITE:
		uio.uio_iovcnt = 1;
		uio.uio_iov = &iov;
		iov.iov_base = fuword(addr);
		iov.iov_len = uio.uio_resid = fuword(addr+NBPW);
		if ((int)iov.iov_base == -1 || iov.iov_len == -1) {
			u.u_error = EFAULT;
			return;
		}
		bp = &vpbuf;
		if (vp11.state&WBUSY) {
			/* wait for previous write */
			(void) _spl4();
			while ((bp->b_flags&B_DONE) == 0)
				sleep(bp, PRIBIO);
			u.u_procp->p_flag &= ~SLOCK;
			bp->b_flags &= ~B_BUSY;
			vp11.state &= ~WBUSY;
			(void) _spl0();
			return(geterror(bp));
		}
		if (uio.uio_resid == 0)
			return;
		/* simulate a write without starting i/o */
		(void) _spl4();
		vp11.state |= NOSTART;
		vpwrite(dev);
		vp11.state &= ~NOSTART;
		if (u.u_error) {
			(void) _spl0();
			return;
		}
		/* fake write was ok, now do it for real */
		bp->b_flags = B_BUSY | B_PHYS | B_WRITE;
		u.u_procp->p_flag |= SLOCK;
		vp11.state |= WBUSY;
		vpstart(bp);
		(void) _spl0();
		return;
#endif THIS_IS_NOT_USEABLE

	default:
		return (ENOTTY);
	}
	return (0);
}
#endif

vperror()
{
	register int state, err;

	state = vp11.state & PLOT;
	(void) _spl4();
	while ((err=(state? PLOTADR->csr : PRINTADR->csr)&(READY|ERROR))==0)
		sleep((caddr_t)&vp11, PRIBIO);
	(void) _spl0();
	if (err&ERROR) {
		u.u_error = EIO;
#ifndef NOMESG
		if (!(vp11.state&ERRMSG)) {
			printf(vperr);
			vp11.state |= ERRMSG;
		}
#endif
	} else
		vp11.state &= ~ERRMSG;
	return(err&ERROR);
}

vpstart(bp)
register struct buf *bp;
{
	register struct dmaregs *vpp = DMA;

	if (vp11.state&NOSTART) {
		bp->b_flags |= B_DONE;	/* fake it */
		return;
	}
	mapalloc(bp);
	vpp->addr = (short)(bp->b_un.b_addr);
	vpp->addrext = (bp->b_xmem & 03) << 4;
#ifndef	VP_TWOSCOMPL
	if (vp11.state&PLOT)
		vpp->plotbytes = vp11.bytecount;
	else
	 	vpp->printbytes = vp11.bytecount;
#else
	if (vp11.state&PLOT)
		vpp->plotbytes = -vp11.bytecount;
	else
	 	vpp->printbytes = -vp11.bytecount;
#endif
}

vpintr()
{
	register struct buf *bp = &vpbuf;

	/*
	 * Hardware botch.  Interrupts occur a few usec before the 'READY'
	 * bit actually sets.  On fast processors it is unreliable to test
	 * this bit in the interrupt service routine!
	 */
	if (bp->b_flags&B_DONE) {
		wakeup((caddr_t)&vp11);
		return;
	}
	if (((vp11.state&PLOT)? PLOTADR->csr : PRINTADR->csr) < 0) {
#ifndef NOMESG
		if (!(vp11.state&ERRMSG)) {
			printf(vperr);
			vp11.state |= ERRMSG;
		}
#endif
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		wakeup((caddr_t)&vp11);
	}
	iodone(bp);
}
#endif NVP

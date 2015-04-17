/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mem.c	1.3 (2.11BSD GTE) 1996/5/15
 */

#include "param.h"
#include "../machine/seg.h"
#include "user.h"
#include "conf.h"
#include "uio.h"

/*
 * This routine is callable only from the high
 * kernel as it assumes normal mapping and doesn't
 * bother to save 'seg5'.
 */

mmrw(dev, uio, flag)
	dev_t dev;
	register struct uio *uio;
	int flag;
{
	register struct iovec *iov;
	int error = 0;
register u_int c;
	u_int on;
	char	zero[1024];

	if	(minor(dev) == 3)
		bzero(zero, sizeof (zero));
	while (uio->uio_resid && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

/* minor device 0 is physical memory (/dev/mem) */
		case 0:
			mapseg5((memaddr)(uio->uio_offset>>6),
				   ((btoc(8192)-1)<<8)|RW);
			on = uio->uio_offset & 077L;
			c = MIN(iov->iov_len, 8192 - on);
			error = uiomove(SEG5+on, c, uio);
			normalseg5();
			break;
/* minor device 1 is kernel memory (/dev/kmem) */
		case 1:
			error = uiomove((caddr_t)uio->uio_offset, iov->iov_len, uio);
			break;
/* minor device 2 is EOF/RATHOLE (/dev/null) */
		case 2:
			if (uio->uio_rw == UIO_READ)
				return(0);
			c = iov->iov_len;
			iov->iov_base += c;
			iov->iov_len -= c;
			uio->uio_offset += c;
			uio->uio_resid -= c;
			break;
/* minor device 3 is ZERO (/dev/zero) */
		case 3:
			if	(uio->uio_rw == UIO_WRITE)
				return(EIO);
			c = MIN(iov->iov_len, sizeof (zero));
			error = uiomove(zero, c, uio);
			break;
		default:
			return(EINVAL);
		} /* switch */
	} /* while */
	return(error);
}

/*
 * Internal versions of mmread(), mmwrite()
 * used by disk driver ecc routines.
 */
getmemc(addr)
	long addr;
{
	register int a, c, d;

	/*
	 * bn = addr >> 6
	 * on = addr & 077
	 */
	a = UISA[0];
	d = UISD[0];
	UISA[0] = addr >> 6;
	UISD[0] = RO;			/* one click, read only */
	c = fuibyte((caddr_t)(addr & 077));
	UISA[0] = a;
	UISD[0] = d;
	return(c);
}

putmemc(addr,contents)
	long addr;
	int contents;
{
	register int a, d;

	/*
	 * bn = addr >> 6
	 * on = addr & 077
	 */
	a = UISA[0];
	d = UISD[0];
	UISA[0] = addr >> 6;
	UISD[0] = RW;			/* one click, read/write */
	suibyte((caddr_t)(addr & 077), contents);
	UISA[0] = a;
	UISD[0] = d;
}

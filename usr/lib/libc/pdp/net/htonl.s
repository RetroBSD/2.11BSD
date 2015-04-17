/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)htonl.s	1.1 (Berkeley) 1/25/87\0>
	.even
#endif LIBC_SCCS

/*
 * netlong = htonl(hostlong);
 *	u_long	netlong,
 *		hostlong;
 *
 * hostlong = ntohl(netlong);
 *	u_long	hostlong,
 *		netlong;
 *
 * Translate from host unsigned long representation to network unsigned
 * long representation and back.  On the PDP-11 all this requires is
 * swapping the bytes within the high and low words of a long, so the
 * two routines are really one ...
 */
#include "DEFS.h"

.globl	_htonl, _ntohl
_htonl:
_ntohl:
	PROFCODE(_htonl)
	mov	2(sp),r0
	mov	4(sp),r1
	swab	r0
	swab	r1
	rts	pc

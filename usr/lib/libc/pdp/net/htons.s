/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)htons.s	1.1 (Berkeley) 1/25/87\0>
	.even
#endif LIBC_SCCS

/*
 * netshort = htons(hostshort);
 *	u_short	netshort,
 *		hostshort;
 *
 * hostshort = ntohs(netshort);
 *	u_short	hostshort,
 *		netshort;
 *
 * Translate from host unsigned short representation to network unsigned
 * short representation and back.  On the PDP-11 all this requires is
 * swapping the bytes of a short, so the two routines are really one ...
 */
#include "DEFS.h"

.globl	_htons, _ntohs
_htons:
_ntohs:
	PROFCODE(_htons)
	mov	2(sp),r0
	swab	r0
	rts	pc

/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)ldfps.s	2.3 (Berkeley) 6/12/88\0>
	.even
#endif LIBC_SCCS

/*
 * void
 * ldfps(nfps);
 *	u_int nfps;
 *
 * Load floating point status register with nfps.
 */
#include "DEFS.h"

ldfps	= 170100^tst

ENTRY(ldfps)
	ldfps	2(sp)		/ load fps with new configuration
	rts	pc		/ (no value returned)

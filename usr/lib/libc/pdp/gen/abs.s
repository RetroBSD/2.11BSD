/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)abs.s	1.1 (Berkeley) 1/21/87\0>
	.even
#endif LIBC_SCCS

/* abs - int absolute value */

#include "DEFS.h"

ENTRY(abs)
	mov	2(sp),r0	/ grab number
	bge	1f
	neg	r0		/   and negate is less than 0
1:
	rts	pc		/ return with booty

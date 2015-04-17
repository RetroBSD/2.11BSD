/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)modf.s	2.3 (Berkeley) 1/5/87\0>
	.even
#endif LIBC_SCCS

/*
 * double modf (value, iptr)
 * double value, *iptr;
 *
 * Modf returns the fractional part of "value",
 * and stores the integer part indirectly through "iptr".
 */

#include "DEFS.h"

#define	one	040200

ENTRY(modf)
	movf	2(sp),fr0
	modf	$one,fr0
	movf	fr1,*10.(sp)
	rts	pc

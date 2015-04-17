/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)fabs.s	1.1 (Berkeley) 1/25/87\0>
	.even
#endif LIBC_SCCS

/* abs_x = fabs(x)
 *	double	abs_x, x;
 *
 * floating absolute value
 */

#include "DEFS.h"

ENTRY(fabs)
	bic	$0100000,2(sp)	/ rip sign bit to pieces
	movf	2(sp),fr0	/ grab shredded x
	rts	pc		/ and return

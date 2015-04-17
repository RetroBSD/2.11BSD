/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)frexp.s	2.3 (Berkeley) 1/6/87\0>
	.even
#endif LIBC_SCCS

/*
 * mantissa = frexp(value, ip)
 *	double	mantissa,
 *		value;
 *	int	*ip;
 *
 * returns a fractional part 1/2 <= |mantisa| < 1
 * and stores an exponent so value = mantisa * 2^(*ip)
 */
#include "DEFS.h"

ENTRY(frexp)
	movf	2(sp),fr0	/ fr0 = value
	movei	fr0,r0		/ r0 = log2(value)
	movie	$0,fr0		/ force result exponent to biased 0
	mov	r0,*10.(sp)	/ *ip = old log2(value)
	rts	pc

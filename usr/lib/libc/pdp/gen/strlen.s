/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)strlen.s	1.1 (Berkeley) 1/20/87\0>
	.even
#endif LIBC_SCCS

/*
 * Return the length of cp (not counting '\0').
 *
 * strlen(cp)
 *	char *cp;
 */
#include "DEFS.h"

ENTRY(strlen)
	mov	2(sp),r0	/ r0 = cp
1:
	tstb	(r0)+		/ find end os string
	bne	1b
	sub	2(sp),r0	/ length = location('\0')+1 - cp - 1
	dec	r0
	rts	pc

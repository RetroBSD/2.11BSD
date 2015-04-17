/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)strcat.s	1.1 (Berkeley) 1/20/87\0>
	.even
#endif LIBC_SCCS

/*
 * Concatenate string s2 to the end of s1
 * and return the base of s1.
 *
 * char *
 * strcat(s1, s2)
 *	char *s1, *s2;
 */
#include "DEFS.h"

ENTRY(strcat)
	mov	2(sp),r0	/ r0 = s1
1:
	tstb	(r0)+		/ find end of string
	bne	1b
	dec	r0		/ back up to '\0'
	mov	4(sp),r1	/ r1 = s2
2:
	movb	(r1)+,(r0)+	/ copy s2 to end of s1
	bne	2b
	mov	2(sp),r0	/ and return s1
	rts	pc

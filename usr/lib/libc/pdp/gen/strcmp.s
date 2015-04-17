/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)strcmp.s	1.1 (Berkeley) 1/21/87\0>
	.even
#endif LIBC_SCCS

/*
 * Compare string s1 lexicographically to string s2.
 * Return:
 *	0	s1 == s2
 *	> 0	s1 > s2
 *	< 0	s2 < s2
 *
 * strcmp(s1, s2)
 *	char *s1, *s2;
 */
#include "DEFS.h"

ENTRY(strcmp)
	mov	2(sp),r0	/ r0 = s1
	mov	4(sp),r1	/ r1 = s2
1:
	cmpb	(r0)+,(r1)	/ compare the two strings
	bne	2f		/ stop on first mismatch
	tstb	(r1)+		/ but don't pass end of either string
	bne	1b
	clr	r0		/ fell off end of strings with '\0' == '\0`
	rts	pc		/   - success! return zero
2:
	movb	-(r0),r0	/ mismatch, return *s1 - *s2 ...
	movb	(r1),r1		/ (no subb instruction ...)
	sub	r1,r0
	rts	pc

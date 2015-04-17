/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)strncmp.s	1.1 (Berkeley) 1/21/87\0>
	.even
#endif LIBC_SCCS

/*
 * Compare at most n characters of string
 * s1 lexicographically to string s2.
 * Return:
 *	0	s1 == s2
 *	> 0	s1 > s2
 *	< 0	s2 < s2
 *
 * strncmp(s1, s2, n)
 *	char *s1, *s2;
 *	int n;
 */
#include "DEFS.h"

ENTRY(strncmp)
	mov	6(sp),r0	/ r0 = n
	beq	4f		/ (all done if n == 0 - return 0)
	mov	2(sp),r1	/ r1 = s1
	mov	r2,-(sp)	/ need an extra register for s2 ...
	mov	6(sp),r2	/ r2 = s2
1:
	cmpb	(r1)+,(r2)	/ compare the two strings
	bne	5f		/ stop on first mismatch
	tstb	(r2)+		/ but don't pass end of either string
	beq	2f
	sob	r0,1b		/ and n running out will stop us too ...
2:
	clr	r0		/ fell off end of strings while still matching
3:
	mov	(sp)+,r2	/ restore r2
4:
	rts	pc		/ and return
5:
	movb	-(r1),r0	/ mismatch, return *s1 - *s2 ...
	movb	(r2),r1		/ (no subb instruction ...)
	sub	r1,r0
	br	3b

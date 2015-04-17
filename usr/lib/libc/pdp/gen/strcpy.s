/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)strcpy.s	1.1 (Berkeley) 1/20/87\0>
	.even
#endif LIBC_SCCS

/*
 * Copy string s2 over top of s1.
 * Return base of s1.
 *
 * char *
 * strcpy(s1, s2)
 *	char *s1, *s2;
 */
#include "DEFS.h"

ENTRY(strcpy)
	mov	2(sp),r0	/ r0 = s1
	mov	4(sp),r1	/ r1 = s2
1:
	movb	(r1)+,(r0)+	/ copy s2 over s1
	bne	1b		/ but don't pass end of s2
	mov	2(sp),r0	/ and return s1
	rts	pc

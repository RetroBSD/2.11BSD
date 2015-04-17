/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)strncat.s	1.1 (Berkeley) 1/20/87\0>
	.even
#endif LIBC_SCCS

/*
 * Concatenate string s2 on the end of s1
 * and return the base of s1.  The parameter
 * n is the maximum length of string s2 to
 * concatenate.
 *
 * char *
 * strncat(s1, s2, n)
 *	char *s1, *s2;
 *	int n;
 */
#include "DEFS.h"

ENTRY(strncat)
	mov	6(sp),r0	/ r0 = n
	beq	4f		/ (all done if n == 0)
	mov	2(sp),r1	/ r1 = s1
1:
	tstb	(r1)+		/ find end of s1
	bne	1b
	dec	r1		/ back up to '\0'
	mov	r2,-(sp)	/ need an extra register for s2 ...
	mov	6(sp),r2	/ r2 = s2
2:
	movb	(r2)+,(r1)+	/ copy s2 to the end of s1 stopping at the
	beq	3f		/   end of s2 or when n runs out ...
	sob	r0,2b
	clrb	(r1)
3:
	mov	(sp)+,r2	/ restore r2
4:
	mov	2(sp),r0	/ and return s1
	rts	pc

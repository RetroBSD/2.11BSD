/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)strncpy.s	1.1 (Berkeley) 1/20/87\0>
	.even
#endif LIBC_SCCS

/*
 * Copy string s2 over top of string s1.
 * Truncate or null-pad to n bytes.
 *
 * char *
 * strncpy(s1, s2, n)
 *	char *s1, *s2;
 */
#include "DEFS.h"

ENTRY(strncpy)
	mov	6(sp),r0	/ r0 = n
	beq	3f		/ (all done if n == 0)
	mov	2(sp),r1	/ r1 = s1
	mov	r2,-(sp)	/ need an extra register for s2 ...
	mov	6(sp),r2	/ r2 = s2
1:
	movb	(r2)+,(r1)+	/ copy s2 to the end of s1 stopping at the
	beq	4f		/   end of s2 or when n runs out ...
	sob	r0,1b
2:
	mov	(sp)+,r2	/ restore r2
3:
	mov	2(sp),r0	/ and return s1
	rts	pc
4:
	dec	r0		/ the '\0' may also have run n out ...
	beq	2b
5:
	clrb	(r1)+		/ null pad s1 till n runs out
	sob	r0,5b
	br	2b		/ clean up and return

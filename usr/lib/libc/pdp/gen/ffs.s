/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)ffs.s	1.2 (Berkeley) 1/8/87\0>
	.even
#endif LIBC_SCCS

#include "DEFS.h"

/*
 * ffs(mask)
 *	long	mask;
 *
 * Return index of lowest order bit set in mask (counting from the right
 * starting at 1), or zero if mask is zero.  Ffs(m) is essentiallty
 * log2(m)+1 with a 0 error return.
 */
ENTRY(ffs)
	clr	r0		/ cnt:r0 = 0
	mov	4(sp),r1	/ if ((imask = loint(mask)) != 0)
	bne	1f
	mov	2(sp),r1	/ if ((imask = hiint(mask)) == 0)
	beq	2f
	mov	$16.,r0		/ else  cnt = 16
1:
	inc	r0		/ do { cnt++
	asr	r1		/ 	imask >>= 1
	bcc	1b		/ } while (!wasodd(imask))
2:
	rts	pc		/ return(cnt:r0)

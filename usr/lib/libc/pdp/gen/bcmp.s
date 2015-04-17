/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)bcmp.s	1.2 (Berkeley) 1/8/87\0>
	.even
#endif LIBC_SCCS

#include "DEFS.h"

/*
 * bcmp(b1, b2, length)
 *
 * Compare length bytes starting at b1 and b2 - return 0 if equal, non zero
 * otherwise.  Bcmp uses a cost analysis heuristic to determine if it's worth
 * while to try for a word comparison.  If length > N, then try for a word
 * comparison.  Bcmp will fail if N < 3.  8 is a good value for N:
 *
 *	Algorithm			Cost (in instructions)
 *	byte loop			3 * length
 *	word loop overhead [worst case]	12
 *	word loop			1.5 * length
 *
 *	N * 3 > 12 + N * 1.5
 *	N > 8
 */
#define	N	8.

ENTRY(bcmp)
	mov	6(sp),r0	/ if ((r0 = length) == 0)
	beq	3f		/	return(length:r0)
	mov	r2,-(sp)	/ reserve a register for our use
	mov	6(sp),r2	/ r2 = b2
	mov	4(sp),r1	/ r1 = b1
	cmp	r0,$N		/ if (length > N)
	bhi	4f		/	try words

1:	cmpb	(r1)+,(r2)+	/ do  if (*b1++ != *b2++)
	bne	2f		/	return(length:r0) {length != 0}
	sob	r0,1b		/ while (--length)
2:
	mov	(sp)+,r2	/ return(length:r0)
3:
	rts	pc

/*
 * The length of the comparison justifies trying a word by word comparison.
 * If b1 and b2 are of the same parity, we can do a word comparison by
 * handling any leading and trailing odd bytes separately.
 */
4:
	bit	$1,r1		/ if (b1&1 != b2&1)
	beq	5f		/	do bytes
	bit	$1,r2
	beq	1b		/ (b1 odd, b2 even - do bytes)
	cmpb	(r1)+,(r2)+	/ if (leading odd bytes equal)
	bne	2b		/	return(non_zero::r0)
	dec	r0		/ else	onto word loop
	br	6f
5:
	bit	$1,r2
	bne	1b		/ (b1 even, b2 odd - do bytes)
6:
	mov	r0,-(sp)	/ save trailing byte indicator
	asr	r0		/ length >>= 1 (unsigned)
	bic	$0100000,r0	/ (length is now in remaining words to compare)
7:
	cmp	(r1)+,(r2)+	/ do  if (*(short *)b1++ != *(short *)b2++)
	bne	9f		/       return(length:r0) {length != 0}
	sob	r0,7b		/ while (--length)
8:
	mov	(sp)+,r0	/ if (no odd trailing byte)
	bic	$!1,r0
	beq	2b		/	return(0)
	cmpb	(r1)+,(r2)+	/ if (trailing odd bytes different)
	bne	2b		/	return(non-zero)
	clr	r0		/ else	return(0)
	br	2b
9:
	mov	(sp)+,r0	/ bad compare in word loop,
	br	2b		/ return(non-zero)

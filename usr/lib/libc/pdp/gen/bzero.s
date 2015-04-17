/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)bzero.s	1.2 (Berkeley) 8/21/87\0>
	.even
#endif LIBC_SCCS

#include "DEFS.h"

/*
 * bzero(b, length)
 *	caddr_t	b;
 *	u_int	length;
 *
 * Zero length bytes starting at b.  Bzero uses a cost analysis heuristic to
 * determine if it's worth while to try zeroing memory by words.  If length >
 * N, then try for a word zero.  This routine will fail if N < 8.
 * 8 is a good value for N:
 *
 *	Algorithm			Cost (in instructions)
 *	byte loop			2 * length
 *	word loop overhead [worst case]	11
 *	word loop			0.625 * length
 *
 *	N * 2 > 11 + N * 0.625
 *	N > 8
 */
#undef	N
#define	N	8.

ENTRY(bzero)
	mov	4(sp),r0	/ if ((r0 = length) == 0)
	beq	2f		/	return
	mov	2(sp),r1	/ r1 = b
	cmp	r0,$N		/ if (length > N)
	bhi	3f		/	do words
1:
	clrb	(r1)+		/ do  *b++ = 0
	sob	r0,1b		/ while (--length)
2:
	rts	pc		/ and return

/*
 * The length of the zero justifies using a word loop, handling any leading
 * and trailing odd bytes separately.
 */
3:
	bit	$1,r1		/ if (odd leading byte)
	beq	4f		/ {	zero it
	clrb	(r1)+		/	length--
	dec	r0		/ }
4:
	mov	r0,-(sp)	/ save trailing byte indicator
	asr	r0		/ length >>= 1 (unsigned)
	bic	$0100000,r0	/ (length is now in remaining words to zero)
	asr	r0		/ if (length >>= 1, wasodd(length))
	bcc	5f		/	handle leading non multiple of four
	clr	(r1)+
5:
	asr	r0		/ if (length >>= 1, wasodd(length))
	bcc	6f		/	handle leading non multiple of eight
	clr	(r1)+
	clr	(r1)+
6:
	clr	(r1)+		/ do
	clr	(r1)+		/	clear eight bytes
	clr	(r1)+
	clr	(r1)+
	sob	r0,6b		/ while (--length)
	bit	$1,(sp)+	/ if (odd trailing byte)
	beq	7f
	clrb	(r1)+		/	zero it
7:
	rts	pc		/ and return

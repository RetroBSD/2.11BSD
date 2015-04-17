/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)bcopy.s	1.2 (Berkeley) 8/21/87\0>
	.even
#endif LIBC_SCCS

#include "DEFS.h"

/*
 * bcopy(src, dst, length)
 *	caddr_t	src, dst;
 *	u_int	length;
 *
 * Copy length bytes from src to dst - handles overlapping moves.  Bcopy uses
 * a cost analysis heuristic to determine if it's worth while to see if a word
 * copy should be tried.  The test is if length > N, then try for a word
 * copy.  This routine will fail if N < 8.  10 is a good value for N:
 *
 *	Algorithm			Cost (in instructions)
 *	byte loop cost			2 * length
 *	word loop setup [worst case]	14 (instructions)
 *	word loop cost			0.625 * length, for length > 10
 *
 *	N * 2 > 14 N * 0.625
 *	N > 10
 */
#undef	N
#define	N	10.

ENTRY(bcopy)
	mov	6(sp),r0	/ if ((r0 = length) == 0)
	beq	3f		/	return
	mov	r2,-(sp)	/ reserve a register for our use
	mov	6(sp),r2	/ r2 = dst
	mov	4(sp),r1	/ r1 = src
	cmp	r2,r1		/ if (dst == src)
	beq	2f		/ 	return
	bhi	9f		/ if (dst > src)
				/	do backwards copy

/*
 * copy memory from src:r1 to dst:r2 for length:r0 bytes, FORWARDS ...
 */
	cmp	r0,$N		/ if (length >= N)
	bhi	4f		/	try words
1:
	movb	(r1)+,(r2)+	/ do  *dst++ = *src++
	sob	r0,1b		/ while (--length)
2:
	mov	(sp)+,r2	/ and return
3:
	rts	pc

/*
 * The length of the copy justifies trying a word by word copy.  If src and
 * dst are of the same parity, we can do a word copy by handling any leading
 * and trailing odd bytes separately.
 */
4:
	bit	$1,r1		/ if (src&1 != dst&1)
	beq	5f		/	do bytes
	bit	$1,r2
	beq	1b		/ (src odd, dst even - do bytes)
	movb	(r1)+,(r2)+	/ copy leading odd byte
	dec	r0
	br	6f
5:
	bit	$1,r2
	bne	1b		/ (src even, dst odd - do bytes)
6:
	mov	r0,-(sp)	/ save trailing byte indicator
	asr	r0		/ length >>= 1 (unsigned)
	bic	$0100000,r0	/ (length is now in remaining words to copy)
	asr	r0		/ if (length >>= 1, wasodd(length))
	bcc	7f		/	handle leading non multiple of four
	mov	(r1)+,(r2)+
7:
	asr	r0		/ if (length >>= 1, wasodd(length))
	bcc	8f		/	handle leading non multiple of eight
	mov	(r1)+,(r2)+
	mov	(r1)+,(r2)+
8:
	mov	(r1)+,(r2)+	/ do
	mov	(r1)+,(r2)+	/	move eight bytes
	mov	(r1)+,(r2)+
	mov	(r1)+,(r2)+
	sob	r0,8b		/ while (--length)
	bit	$1,(sp)+	/ if (odd trailing byte)
	beq	2b
	movb	(r1)+,(r2)+	/	copy it
	br	2b		/ and return

/*
 * copy memory from src:r1 to dst:r2 for length:r0 bytes, BACKWARDS ...
 */
9:
	add	r0,r1		/ src += length
	add	r0,r2		/ dst += length

	cmp	r0,$N		/ if (length >= N)
	bhi	4f		/	try words
1:
	movb	-(r1),-(r2)	/ do  *--dst = *--src
	sob	r0,1b		/ while (--length)
2:
	mov	(sp)+,r2	/ and return
3:
	rts	pc

/*
 * The length of the copy justifies trying a word by word copy.  If src and
 * dst are of the same parity, we can do a word copy by handling any leading
 * and trailing odd bytes separately.
 */
4:
	bit	$1,r1		/ if (src&1 != dst&1)
	beq	5f		/	do bytes
	bit	$1,r2
	beq	1b		/ (src odd, dst even - do bytes)
	movb	-(r1),-(r2)	/ copy leading odd byte
	dec	r0
	br	6f
5:
	bit	$1,r2
	bne	1b		/ (src even, dst odd - do bytes)
6:
	mov	r0,-(sp)	/ save trailing byte indicator
	asr	r0		/ length >>= 1 (unsigned)
	bic	$0100000,r0	/ (length is now in remaining words to copy)
	asr	r0		/ if (length >>= 1, wasodd(length))
	bcc	7f		/	handle leading non multiple of four
	mov	-(r1),-(r2)
7:
	asr	r0		/ if (length >>= 1, wasodd(length))
	bcc	8f		/	handle leading non multiple of eight
	mov	-(r1),-(r2)
	mov	-(r1),-(r2)
8:
	mov	-(r1),-(r2)	/ do
	mov	-(r1),-(r2)	/	move eight bytes
	mov	-(r1),-(r2)
	mov	-(r1),-(r2)
	sob	r0,8b		/ while (--length)
	bit	$1,(sp)+	/ if (odd trailing byte)
	beq	2b
	movb	-(r1),-(r2)	/	copy it
	br	2b		/ and return

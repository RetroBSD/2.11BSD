/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)bunequal.s 1.2 (Berkeley) 1/8/87\0>
	.even
#endif LIBC_SCCS

#define	ENTRY(x)	.globl _/**/x; \
		_/**/x:

/*
 * This is taken from bcmp.s from 2.10.
 * The output of bunequal is the offset of the byte which didn't match;
 * if all the bytes match, then we return n.
 *
 * bunequal(b1, b2, length)
 */

ENTRY(bunequal)
	mov	6(sp),r0		/ if ((r0 = length) == 0)
	beq	3f			/	return(length:r0)
	mov	r2,-(sp)		/ reserve a register for our use
	mov	6(sp),r2		/ r2 = b2
	mov	4(sp),r1		/ r1 = b1
1:
	cmpb	(r1)+,(r2)+		/ do  if (*b1++ != *b2++)
	bne	2f			/	return(length:r0) {length != 0}
	sob	r0,1b			/ while (--length)
2:
	mov	(sp)+,r2		/ restore r2
	neg	r0			/ return(length:r0)
	add	6(sp),r0
3:
	rts	pc

/* brand new code, using the above as base... */
/*
 * bskip(s1, n, b) : finds the first occurrence of any byte != 'b' in the 'n'
 * bytes beginning at 's1'.
 */

ENTRY(bskip)
	mov	2(sp),r1		/ r1 = s1
	mov	4(sp),r0		/ if ((r0 = n) == 0)
	beq	3f			/	return(n:r0)
	tstb	6(sp)			/ in reality b is always zero, so optimize
	bne	4f			/ slower test
1:
	tstb	(r1)+			/ do  if (*s1++)
	bne	2f			/	return(n:r0) {n != 0}
	sob	r0,1b			/ while (--n)
2:
	neg	r0			/ return(n:r0)
	add	4(sp),r0
3:
	rts	pc

4:
	mov	r2,-(sp)		/ reserve a register for our use
	movb	10(sp),r2		/ r2 = b
1:
	cmpb	(r1)+,r2		/ do  if (*s1++ != b)
	bne	2f			/	return(n:r0) {n != 0}
	sob	r0,1b			/ while (--n)
2:
	mov	(sp)+,r2		/ restore r2
	neg	r0			/ return(n:r0)
	add	4(sp),r0
	rts	pc

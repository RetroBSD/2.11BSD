/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid: <@(#)sbrk.s	2.6 (2.11BSD GTE) 1995/05/08\0>
	.even
#endif SYSLIBC_SCCS

/*
 * XXX - this routine can't use SYSCALL!!!
 */
#include "SYS.h"

.data
.globl	_end
.globl	curbrk, minbrk

curbrk:	_end
minbrk:	_end
.text

ENTRY(sbrk)
	mov	2(sp),r0	/ grab increment
	beq	1f		/ (bop out early if zero)
	add	curbrk,r0	/ calculate and pass break address
	mov	r0,-(sp)
	tst	-(sp)		/ simulate return address stack spacing
	SYS(sbrk)
	bes	2f
	cmp	(sp)+,(sp)+	/ (clean up stack)
1:
	mov	curbrk,r0	/ return old break address and add
	add	2(sp),curbrk	/   increment to curbrk
	rts	pc
2:
	cmp	(sp)+,(sp)+	/ (clean up stack)
	jmp	x_error

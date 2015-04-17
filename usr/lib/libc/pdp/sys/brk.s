/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid: <@(#)brk.s	2.6 (2.11BSD GTE) 1995/05/08\0>
	.even
#endif SYSLIBC_SCCS

/*
 * XXX - this routine can't use SYSCALL!!!
 */
#include "SYS.h"

.globl	curbrk, minbrk

ENTRY(brk)
	cmp	2(sp),minbrk	/ break request too low?
	bhis	1f
	mov	minbrk,2(sp)	/ yes, knock the request up to minbrk
1:
	SYS(sbrk)		/ ask for break
	bes	2f
	mov	2(sp),curbrk	/   and remember it if it succeeded
	rts	pc
2:
	jmp	x_error

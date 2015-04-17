/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid: <@(#)execle.s	2.6 (2.11BSD GTE) 1995/05/08\0>
	.even
#endif SYSLIBC_SCCS

/*
 * XXX - this routine can't use SYSCALL!!!
 */
#include "SYS.h"

ENTRY(execle)
	tst	-(sp)		/ leave space for environment pointer
	mov	sp,r0		/ calculate address of first argv element
	add	$6.,r0
	mov	r0,-(sp)	/ (pass it)
1:
	tst	(r0)+		/ found the end of the argv list yet?
	bne	1b		/ nope - keep looping
	mov	(r0),2(sp)	/ yeah, pass the user environment pointer
	mov	6(sp),-(sp)	/ pass the name
	tst	-(sp)		/ simulate return address stack spacing
	SYS(execve)		/   and go for it ...
	add	$8.,sp		/ if we get back it's an error
	jmp	x_error

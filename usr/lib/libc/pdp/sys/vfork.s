/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid: <@(#)vfork.s	2.6 (2.11BSD) 1995/05/13\0>
	.even
#endif SYSLIBC_SCCS

/*
 * XXX - this routine can't use SYSCALL!!!
 *
 * pid = vfork();
 *
 * pid == 0 in child process; pid == -1 means error return in child.
 * Since the parent and child share the stack, the return address for the 
 * parent would be overwritten by the child.  Therefore, save the return 
 * address in r1 and "return" by a jump indirect.
 */
#include "SYS.h"

.globl	__ovno

.bss
savov:	.=.+2
.text

ENTRY(vfork)
	mov	(sp)+,r1	/ save return address
	mov	__ovno,savov	/ save __ovno for parent
	SYS(vfork)		/ (takes no parameters)
	br	1f		/ child returns here
	bcs	2f		/ parent returns here
	mov	savov,__ovno	/ restore ovno in case child switched
	jmp	(r1)		/ "return" to saved location
1:
	clr	r0
	jmp	(r1)
2:
	mov	r0,_errno
	mov	$-1,r0
	jmp	(r1)		/ "return" to saved location

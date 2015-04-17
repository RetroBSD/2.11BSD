/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid: <@(#)pipe.s	2.6 (2.11BSD GTE) 1995/05/08\0>
	.even
#endif SYSLIBC_SCCS

/*
 * XXX - this routine can't use SYSCALL!!!
 */
#include "SYS.h"

ENTRY(pipe)
	SYS(pipe)
	bes	1f
	mov	r2,-(sp)
	mov	4(sp),r2
	mov	r0,(r2)+
	mov	r1,(r2)
	mov	(sp)+,r2
	clr	r0
	rts	pc
1:
	jmp	x_error

/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid: <@(#)_exit.s	3.0 (2.11BSD GTE) 3/13/93\0>
	.even
#endif SYSLIBC_SCCS

#include "SYS.h"

ENTRY(_exit)
	SYS(exit)
/* NOTREACHED */

	.globl	x_norm
	.globl	x_error
x_norm:
	bcc	1f
x_error:
	mov	r0,_errno
	mov	$-1,r0
1:	rts	pc

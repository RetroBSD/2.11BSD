/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid: <@(#)fork.s	2.6 (2.11BSD GTE) 1995/05/10\0>
	.even
#endif SYSLIBC_SCCS

/*
 * XXX - this routine can't use SYSCALL!!!
 */
#include "SYS.h"

ENTRY(fork)
	SYS(fork)
	br	1f			/ child returns here
	bcs	2f			/ parent returns here
	rts	pc
1:
	clr	r0			/ child gets a zero
	rts	pc
2:
	jmp	x_error

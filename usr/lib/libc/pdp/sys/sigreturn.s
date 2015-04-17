/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid: <@(#)sigreturn.s	2.6 (2.11BSD GTE) 1995/05/08\0>
	.even
#endif SYSLIBC_SCCS

/*
 * error = sigreturn(scp);
 *	int			error;
 * 	struct sigcontext	*scp;
 *
 * Overlaid version of sigreturn.  MUST BE LOADED IN BASE SEGMENT!!!
 *
 * Note that no qualification for returning to the base segment is made in
 * sigreturn (to avoid a posible superfluous overlay switch request) as cret
 * does.  The problem is the routine being returned to in the base could
 * easily be one which doesn't use cret itself to return (like a system call
 * from the library) which on its own return might be returning to a different
 * overlay than the one currently loaded.
 */
#include "SYS.h"

SC_OVNO	= 18.				/ offset of sc_ovno in sigcontext
emt	= 0104000			/ overlay switch - ovno in r0

.globl	__ovno

/*
 * XXX - cat'n use ENTRY(sigreturn)!!! - ENTRY includes profiling code which
 * mungs registers.
 */
.globl	_sigreturn
_sigreturn:
	mov	2(sp),r0		/ r0 = scp->sc_ovno
	mov	SC_OVNO(r0),r0
	beq	1f			/ zero implies no overlay switches yet
	cmp	r0,__ovno		/ same overlay currently loaded?
	beq	1f			/   yes, nothing to do
	emt				/   no, request overlay load
	mov	r0,__ovno		/     and indicate new overlay

	/ The last two instructions represent a potential race condition ...
1:
	SYS(sigreturn)			/ attempt the sigreturn
	jmp	x_error

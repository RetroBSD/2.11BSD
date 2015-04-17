/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid: <@(#)ptrace.s	2.6 (2.11BSD GTE) 1995/05/08\0>
	.even
#endif SYSLIBC_SCCS

#include "SYS.h"

ENTRY(ptrace)
	clr	_errno		/ -1 is a legitimate return value so we must
	SYS(ptrace)		/   clear errno so the caller may
	bes	error		/   disambiguate
	rts	pc
error:
	jmp	x_error

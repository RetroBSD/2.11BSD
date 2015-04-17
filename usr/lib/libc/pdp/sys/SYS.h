/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)SYS.h	1.5 (2.11BSD GTE) 1995/05/08
 */

#include <syscall.h>

.comm	_errno,2

#define	ENTRY(x)	.globl _/**/x; \
		_/**/x: \
			PROFCODE(_/**/x);

#define	ASENTRY(x)	.globl x; \
		x: \
			PROFCODE(x);

#ifdef PROF
#define	PROFCODE(x)	.data; \
		1:	x+1; \
			.text; \
			.globl	mcount; \
			mov	$1b, r0; \
			jsr	pc,mcount;
#else !PROF
#define	PROFCODE(x)	;
#endif PROF

#define	SYS(s)		sys	SYS_/**/s.

#define	SYSCALL(s, r)	ENTRY(s); \
			SYS(s); \
			EXIT_/**/r

		.globl	x_norm, x_error

#define	EXIT_norm		jmp	x_norm

#define	EXIT_long		bcc	1f; \
				mov	r0,_errno; \
				mov	$-1,r1; \
				sxt	r0; \
				1: rts	pc;

#define	EXIT_error		jmp	x_error

#define	EXIT_noerror		rts	pc;

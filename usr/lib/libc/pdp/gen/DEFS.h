/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)DEFS.h	1.1 (Berkeley) 1/25/87
 */

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

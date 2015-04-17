/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)DEFS.h	1.2 (2.11BSD GTE) 12/24/92
 */

#ifndef _DEFS_
#define	_DEFS_
/*
 * Machine language assist.  Uses the C preprocessor to produce suitable code
 * for various 11's.
 */ 
#include "localopts.h"
#include "assym.h"

#define	INTSTK	500.			/* bytes for interrupt stack */

#ifdef PROF
#	define	HIGH	06		/* See also the :splfix files */
#	define	HIPRI	0300		/* Many spl's are done in-line */
#else
#	define	HIGH	07
#	define	HIPRI	0340
#endif

#	define	NET	02
#	define	NETPRI	0100

/*
 * adapt to any 11 at boot
 */
#ifdef	GENERIC
#	undef	NONSEPARATE	/* Enable support for separate I&D if found */
#endif

#ifdef NONSEPARATE		/* 11/40, 34, 23, 24 */
#	define mfpd		mfpi
#	define mtpd		mtpi
#endif

#if defined(GENERIC) || defined(SUPERVISOR) || defined(NONSEPARATE)
	/*
	 * GENERIC: movb instruction are available on all PDP-11s.
	 *
	 * SUPERVISOR: can't use spl instructions even if the machine
	 * supports them since spl is a privileged instruction.
	 */
#	define	SPLHIGH		movb	$HIPRI,PS
#	define	SPL7		movb	$0340,PS
#	define	SPLLOW		clrb	PS
#	define	SPLNET		movb	$NETPRI,PS
#else
#	define SPLHIGH		spl	HIGH
#	define SPL7		spl	7
#	define SPLLOW		spl	0
#	define SPLNET		spl	NET
#endif


#define	CONST(s, x, v)	DEC_/**/s(x); x=v;
#define	INT(s, x, v)	.data; .even; DEC_/**/s(x); x:; v; .text;
#define	CHAR(s, x, v)	.data; DEC_/**/s(x); x:; .byte v; .text;
#define	STRING(s, x, v)	.data; DEC_/**/s(x); x:; v; .text;
#define	SPACE(s, x, n)	.bss;  .even; DEC_/**/s(x); x:; .=.+[n]; .text;

#define	DEC_GLOBAL(x)	.globl x;
#define	DEC_LOCAL(x)

/*
 * Macros for compatibility with standard library routines that we have
 * copies of ...
 */
#define	ENTRY(x)	.globl _/**/x; _/**/x:;
#define	ASENTRY(x)	.globl x; x:;

#define	P_ENTRY(x)	.globl _/**/x; _/**/x:; PROFCODE;
#define	P_ASENTRY(x)	.globl x; x:; PROFCODE;

/*
 * PROFCODE:
 * - kernel profiling not currently implemented.
 */
#define	PROFCODE	;
#endif _DEFS_

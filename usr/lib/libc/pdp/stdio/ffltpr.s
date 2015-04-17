/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)ffltpr.s	5.4 (Berkeley) 1/25/87\0>
	.even
#endif LIBC_SCCS

/*
 * Fake floating output support for doprnt.  See fltpr.s for comments.
 */

.globl	pgen, pfloat, pscien

pfloat:
pscien:
pgen:
	add	$8,r4		/ Simply output a "?"
	movb	$'?,(r3)+
	rts	pc

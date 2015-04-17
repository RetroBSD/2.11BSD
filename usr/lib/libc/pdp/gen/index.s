/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)index.s	1.2 (2.11BSD) 1996/1/12\0>
	.even
#endif LIBC_SCCS

/*
 * Find the first occurence of c in the string cp.
 * Return pointer to match or null pointer.
 *
 * char *
 * index(cp, c)
 *	char *cp, c;
 */
#include "DEFS.h"

	.globl	_strchr
_strchr = _index ^ .

ENTRY(index)
	mov	2(sp),r0	/ r0 = cp
	mov	4(sp),r1	/ r1 = c
	beq	3f		/ check for special case of c == '\0'
1:
	cmpb	(r0),r1		/ look for c ...
	beq	2f
	tstb	(r0)+		/ but don't pass end of string ...
	bne	1b
	clr	r0		/ fell off the end of the string - return 0
2:
	rts	pc		/ return pointer
3:
	tstb	(r0)+		/ just find end of string
	bne	3b
	dec	r0		/ back up to '\0'
	rts	pc		/   and return pointer


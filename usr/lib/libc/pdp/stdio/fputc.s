/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)fputc.s	1.1 (Berkeley) 2/4/87\0>
	.even
#endif LIBC_SCCS

#include "DEFS.h"
#include "STDIO.h"

.globl	__flsbuf

/*
 * fputc(c, iop)
 *	char	c;
 *	FILE	*iop;
 *
 * Output c to stream iop and return c.  For unbuffered streams iop->cnt is
 * always zero.  For normally buffered streams iop->cnt is the amount of space
 * left in the buffer.  For line buffered streams, -(iop->cnt) is the number
 * of characters in the buffer.
 */
ENTRY(fputc)
	mov	4(sp),r1		/ grab iop
	dec	_CNT(r1)		/ less room in buffer
	blt	2f
	mov	2(sp),r0		/ grab the character (for return)
1:
	movb	r0,*_PTR(r1)		/ push character into the buffer,
	inc	_PTR(r1)		/   increment the pointer,
	bic	$!0377,r0		/   cast to u_char - just in case ...
	rts	pc			/   and return
2:
	bit	$_IOLBF,_FLAG(r1)	/ iop-cnt < 0: are we line buffered?
	beq	3f			/ (nope, flush output)
	mov	_CNT(r1),r0		/ -(iop->cnt) < iop->bufsiz?
	add	_BUFSIZ(r1),r0		/ (same as 0 < iop->bufsiz + iop->cnt)
	ble	3f			/ (nope, flush output)
	mov	2(sp),r0		/ grab the character
	cmpb	r0,$NL			/ a newline?
	bne	1b			/ no, just put it into the buffer
3:
	jmp	__flsbuf

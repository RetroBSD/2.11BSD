/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)puts.s	5.6 (Berkeley) 12/24/88\0>
	.even
#endif LIBC_SCCS

#include "DEFS.h"
#include "STDIO.h"

/*
 * BUFSIZ is only used if we're asked to output to an unbuffered output
 * stream.  Given that a majority of our arguments are going to be less
 * than 80 characters (one screen line), we might as well use a fairly small
 * value for BUFSIZ ...
 */
#define		BUFSIZ	128.

/*
 * puts(s);
 * char *s;
 *
 * argument: a source string.
 * side effects: writes to the standard output using the data in
 *	the null-terminated source string; a newline is appended.
 * result: technically void; for compatibility we return 0 for errors,
 *	a newline (\n) otherwise
 */
ENTRY(puts)
	mov	$STDOUT,r0		/ out to stdout
	mov	$1,r1			/ (append a newline)
	br	Lputs

/*
 * fputs(s, iop);
 * char *s;
 * FILE *iop;
 *
 * arguments: a source string and a file pointer.
 * side effects: writes to the file indicated by iop using the data in
 *	the null-terminated source string.
 * result: technically void; for compatibility we return 0 for errors,
 *	a newline (\n) otherwise
 */
ENTRY(fputs)
	mov	4(sp),r0		/ out to iop
	clr	r1			/ (don't append a newline)
/	br	Lputs
/*FALLTHROUGH*/

/*
 * ASENTRY(Lputs)(s::2(sp), iop::r0, nlflag::r1)
 *	char	*s;
 *	FILE	*iop;
 *
 * Implements puts and fputs.
 */
.globl	__flsbuf, _fflush

#define		S	r4
#define		IOP	r3
#define		COUNT	r2
#define		P	r1
#define		C	r0
/*
 * P & C get trounced when we call someone else ...
 */

Lputs:
	mov	r2,-(sp)		/ need a few registers
	mov	r3,-(sp)
	mov	r4,-(sp)
	mov	r0,IOP			/ put IOP in the right register
	mov	r1,-(sp)		/ save newline flag
	mov	10.(sp),S		/ grab string pointer
	sub	$BUFSIZ+2,sp		/ allocate a buffer and flag on stack

#	define	NLFLAG	BUFSIZ+2(sp)
#	define	UNBUF	BUFSIZ(sp)
#	define	BUF	sp

#	define	FRSIZE	BUFSIZ+4

	/*
	 * For unbuffered I/O, line buffer the output line.
	 * Ugly but fast -- and doesn't CURRENTLY break anything (sigh).
	 */
	mov	_FLAG(IOP),UNBUF	/ get a copy of the current flags for
	bic	$!_IONBF,UNBUF		/  iob - iob buffered?
	beq	1f

	bic	$_IONBF,_FLAG(IOP)	/ no, clear no-buffering flag
	mov	BUF,_BASE(IOP)		/ and set up to buffer into our on
	mov	BUF,_PTR(IOP)		/ stack buffer
	mov	$BUFSIZ,_BUFSIZ(IOP)
	br	2f			/ have _flsbuf finish the buffer setup
1:
	tst	_CNT(IOP)		/ has buffer been allocated?
	bgt	3f
2:
	mov	IOP,-(sp)		/ get _flsbuf('\0', stdout) to make
	clr	-(sp)			/   one
	jsr	pc,__flsbuf
	cmp	(sp)+,(sp)+
	tst	r0
	blt	Lerror
	inc	_CNT(IOP)		/ unput the '\0' we sent
	dec	_PTR(IOP)
3:
	tstb	(S)			/ null string?
	beq	Lnl

	mov	_BASE(IOP),COUNT	/ figure out how much room is left
	add	_BUFSIZ(IOP),COUNT	/   in buffer (base+bufsiz-ptr)
	mov	_PTR(IOP),P
	sub	P,COUNT
Lloop:
	/*
	 * Copy till terminating null found or out of room.
	 */
	mov	COUNT,C
1:
	movb	(S)+,(P)+		/ found null?
	beq	Llast
	sob	C,1b			/ run out of room?

	mov	P,_PTR(IOP)		/ yes, fix up IOP
	clr	_CNT(IOP)
	mov	IOP,-(sp)		/ the buffer is full - flush it
	jsr	pc,_fflush
	tst	(sp)+
	tst	r0
	blt	Lerror
	tstb	(S)			/ more data??
	beq	Lnl			/ nope, clean up ...

	mov	_PTR(IOP),P		/ yes, easy to compute how much room
	mov	_BUFSIZ(IOP),COUNT	/   is left this time ...
	br	Lloop
Llast:
	sub	C,COUNT			/ how much did we actually move?
	add	COUNT,_PTR(IOP)		/ update IOP
	sub	COUNT,_CNT(IOP)
Lnl:
	tst	NLFLAG			/ need to append a newline?
	beq	1f

	movb	$NL,*_PTR(IOP)		/ yes, there's always room for one
	inc	_PTR(IOP)		/   more character at this point
	dec	_CNT(IOP)
1:
	bit	$_IOLBF,_FLAG(IOP)	/ if line buffered ...
	bne	2f
	tst	UNBUF			/   or unbuffered ...
	bne	2f
	tst	_CNT(IOP)		/   or a full buffer ...
	bgt	3f
2:
	mov	IOP,-(sp)		/ ... flush the buffer
	jsr	pc,_fflush
	tst	(sp)+
	tst	r0
	blt	Lerror
3:
	movb	$NL,r0			/ compatibility hack
Lfixup:
	/*
	 * Fix up buffering again.
	 */
	tst	UNBUF
	beq	Lret
	bis	$_IONBF,_FLAG(IOP)	/ reset flag
	clr	_BASE(IOP)		/ clear data structure
	clr	_BUFSIZ(IOP)
	clr	_CNT(IOP)
Lret:
	add	$FRSIZE,sp		/ deallocate local stack variables
	mov	(sp)+,r4		/ restore registers
	mov	(sp)+,r3
	mov	(sp)+,r2
	rts	pc			/ and return

	/*
	 * Bomb out.  Return 0 (why not? that's what the old one did).
	 */
Lerror:
	clr	r0
	br	Lfixup

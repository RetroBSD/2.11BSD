/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)gets.s	5.6 (Berkeley) 9/2/88\0>
	.even
#endif LIBC_SCCS

#include "DEFS.h"
#include "STDIO.h"

.globl	__filbuf

#define		S	r3
#define		IOP	r2
#define		P	r1
#define		C	r0
/*
 * P & C get trounced when we call someone else ...
 */

/*
 * char *gets(s);
 * char *s;
 *
 * argument: a target string
 * side effects: reads bytes up to and including a newline from the
 *	standard input into the target string and replaces the newline
 *	with a null to null-terminate the string.
 * result: the target string if successful, 0 otherwise.
 */
ENTRY(gets)
	mov	r2,-(sp)		/ need a few registers
	mov	r3,-(sp)

#	define	OLD_S	6.(sp)

	mov	OLD_S,S			/ grab string pointer
	mov	$STDIN,IOP		/ in from stdin

	/*
	 * If no characters, call _filbuf() to get some.
	 */
	tst	_CNT(IOP)
	bgt	Lscan

Lloop:
	mov	IOP,-(sp)		/ _filbuf(stdin)
	jsr	pc,__filbuf
	tst	(sp)+
	tst	r0			/ _filbuf return EOF?
	blt	Leof
	cmpb	r0,$NL			/ a newline?
	bne	1f
	clrb	(S)			/ yes, terminate the string and return
	br	Lret			/   with a pointer to it
1:
	movb	r0,(S)+			/ save the returned character
	tst	_BASE(IOP)		/ is input buffered?
	beq	Lloop			/ no, have to do it the hard way ...
	tst	_CNT(IOP)		/ did __filbuf leave us anything
	beq	Lloop			/   to work with??

Lscan:
	/*
	 * Copy till terminating newline found or end of buffer.
	 */
	mov	_PTR(IOP),P		/ grab pointer into I/O buffer
	mov	_CNT(IOP),C		/   and how many characters in it
1:
	movb	(P),(S)+		/ copy from buffer to string
	cmpb	(P)+,$NL		/ was it a newline?
	beq	2f
	sob	C,1b			/ repeat till we hit the end of the
	br	Lloop			/ buffer, and back for another
2:
	clrb	-(S)			/ overwrite the newline with a null
	sub	_PTR(IOP),P		/ figure out how much we took from
	add	P,_PTR(IOP)		/ the buffer and update IOP
	sub	P,_CNT(IOP)

Lret:
	mov	OLD_S,r0		/ return pointer to string
Lexit:
	mov	(sp)+,r3		/ restore registers
	mov	(sp)+,r2
	rts	pc			/ and return

	/*
	 * End of file?  Check to see if we copied any data.
	 */
Leof:
	cmp	S,OLD_S			/ did we copy anything?
	beq	Lerror			/ nope, return null
	clrb	(S)			/ yes, terminate string
	br	Lret			/   and return a pointer to it

	/*
	 * Error/eof return -- null pointer.
	 */
Lerror:
	clr	r0
	br	Lexit

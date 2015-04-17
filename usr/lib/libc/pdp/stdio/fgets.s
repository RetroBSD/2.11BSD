/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)fgets.s	5.6 (Berkeley) 9/2/88\0>
	.even
#endif LIBC_SCCS

#include "DEFS.h"
#include "STDIO.h"

.globl	__filbuf

#define		S	r4
#define		IOP	r3
#define		COUNT	r2
#define		P	r1
#define		C	r0
/*
 * P & C get trounced when we call someone else ...
 */

/*
 * char *fgets(s, n, iop);
 * char *s;
 * int n;
 * FILE *iop;
 *
 * arguments: a target string, a length, and a file pointer.
 * side effects: reads up to and including a newline, or up to n-1 bytes,
 *	whichever is less, from the file indicated by iop into the target
 *	string and null terminates.
 * result: the target string if successful, 0 otherwise.
 */
ENTRY(fgets)
	mov	r2,-(sp)		/ need a few registers
	mov	r3,-(sp)
	mov	r4,-(sp)

#	define	OLD_S	8.(sp)

	mov	OLD_S,S			/ grab string pointer
	mov	10.(sp),COUNT		/   string length
	mov	12.(sp),IOP		/   and I/O pointer

	/*
	 * Sanity check -- is the string big enough?  Has to hold at least
	 * a null ...
	 */
	dec	COUNT			/ We scan at most n-1 characters
	ble	Lerror

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
	movb	r0,(S)+			/ save the returned character
	dec	COUNT			/ out of space?
	ble	1f
	cmpb	r0,$NL			/ a newline?
	bne	2f
1:
	clrb	(S)			/ yes, terminate the string and return
	br	Lret			/   with a pointer to it
2:
	tst	_BASE(IOP)		/ is input buffered?
	beq	Lloop			/ no, have to do it the hard way ...
	tst	_CNT(IOP)		/ did __filbuf leave us anything
	beq	Lloop			/   to work with??

Lscan:
	/*
	 * Copy till terminating newline found or end of buffer or end of
	 * string.
	 */
	mov	_PTR(IOP),P		/ grab pointer into I/O buffer
	mov	_CNT(IOP),C		/   and how many characters in it
	sub	C,COUNT			/ more in buffer than we can take?
	bge	1f
	add	COUNT,C			/ only copy till end of string then
	clr	COUNT
1:
	movb	(P),(S)+		/ copy from buffer to string
	cmpb	(P)+,$NL		/ was it a newline?
	beq	2f
	sob	C,1b			/ repeat till we run out ...
	tst	COUNT			/ more room in string?
	bne	Lloop			/ yes, go back for another buffer
2:
	clrb	(S)			/ terminate string
	sub	_PTR(IOP),P		/ figure out how much we took from
	add	P,_PTR(IOP)		/ the buffer and update IOP
	sub	P,_CNT(IOP)

Lret:
	mov	OLD_S,r0		/ return pointer to string
Lexit:
	mov	(sp)+,r4		/ restore registers
	mov	(sp)+,r3
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

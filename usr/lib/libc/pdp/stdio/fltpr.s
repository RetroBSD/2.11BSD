/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)fltpr.s	5.4 (Berkeley) 6/3/87\0>
	.even
#endif LIBC_SCCS

/*
 * Floating point output support for doprnt.
 */
#include "DEFS.h"

.data
sign:	.=.+2
digits:	.=.+2
decpt:	.=.+2
.text

.globl	_ecvt, _fcvt, _gcvt

/*
 * If the compiler notices any floating point type stuff going on in a
 * C object, it automatically outputs a reference to the symbol "fltused"
 * to force loading of this portion of the doprnt support.  If fltused is
 * not defined the references to pgen, pfloat and pscien in doprnt will
 * be satisfied by ffltpr.s (fake floating print).  This artifice saves 1056
 * bytes of text and 98 bytes of data space at last count.
 */
.globl	fltused
fltused:

#ifdef NONFP
/*
 * And yet another kludge: since only one module may satisfy the fltused
 * reference and since we really want to do the same trick with the floating
 * point interpreter, we have this module reference the symbol fpsim defined
 * by the interpreter to drag it in too ... (sigh) See the file
 * ../fpsim/README for the real dirt ...
 */
.globl	fpsim
#endif

ASENTRY(pgen)
	mov	r3,-(sp)
	mov	r0,-(sp)
	tst	r2
	bne	1f
	mov	$6,(sp)
1:
	movf	(r4)+,fr0
	movf	fr0,-(sp)
	jsr	pc,_gcvt
	add	$8+2+2,sp
1:
	tstb	(r3)+
	bne	1b
	dec	r3
	rts	pc

ASENTRY(pfloat)
	mov	$sign,-(sp)
	mov	$decpt,-(sp)
	tst	r2
	bne	1f
	mov	$6,r0
1:
	mov	r0,-(sp)
	mov	r0,digits
	movf	(r4)+,fr0
	movf	fr0,-(sp)
	jsr	pc,_fcvt
	add	$8+2+2+2,sp
	tst	sign
	beq	1f
	movb	$'-,(r3)+
1:
	mov	decpt,r2
	bgt	1f
	movb	$'0,(r3)+
1:
	mov	r2,r1
	ble	1f
2:
	movb	(r0)+,(r3)+
	sob	r1,2b
1:
	mov	digits,r1
	beq	1f
	movb	$'.,(r3)+
1:
	neg	r2
	ble	1f
2:
	dec	r1
	blt	1f
	movb	$'0,(r3)+
	sob	r2,2b
1:
	tst	r1
	ble	2f
1:
	movb	(r0)+,(r3)+
	sob	r1,1b
2:
	rts	pc

ASENTRY(pscien)
	mov	$sign,-(sp)
	mov	$decpt,-(sp)
	mov	r0,-(sp)
	mov	r0,digits
	tst	r2
	bne	1f
	mov	$6,(sp)
1:
	movf	(r4)+,fr0
	movf	fr0,-(sp)
	jsr	pc,_ecvt
	add	$8+2+2+2,sp
	tst	sign
	beq	1f
	movb	$'-,(r3)+
1:
	cmpb	(r0),$'0
	bne	1f
	inc	decpt
1:
	movb	(r0)+,(r3)+
	movb	$'.,(r3)+
	mov	digits,r1
	dec	r1
	ble	1f
2:
	movb	(r0)+,(r3)+
	sob	r1,2b
1:
	movb	$'e,(r3)+
	mov	decpt,r2
	dec	r2
	mov	r2,r1
	bge	1f
	movb	$'-,(r3)+
	neg	r1
	br	2f
1:
	movb	$'+,(r3)+
2:
	clr	r0
	div	$10.,r0
	add	$'0,r0
	movb	r0,(r3)+
	add	$'0,r1
	movb	r1,(r3)+
	rts	pc

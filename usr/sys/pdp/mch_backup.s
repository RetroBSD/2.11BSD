/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_backup.s	1.2 (2.11BSD GTE) 12/26/92
 */

#include "DEFS.h"

SPACE(GLOBAL, ssr, 3*2)			/ contents of MM registers SSR0-2
					/   after trap

/*
 * unwind(ar0::r0, mod::r1)
 *	caddr_t ar0;
 *	u_char mod;
 *
 * Unwind a register modification from a partially completed aborted
 * instruction.  Ar0 is the address of the user's register r0 as saved in
 * the stack (_regloc contains the relative indexes of the the user's
 * registers from ar0).  Mod contains a register modification recorded as:
 *
 *	 7		3 2	0
 *	+------------------------+
 *	|  displacement	 |  reg	 |
 *	+------------------------+
 *
 * where "reg" is the index of the modified register and displacement is
 * the 2's complement displacement of the modification.
 */
unwind:
	mov	r1,-(sp)		/ grab displacement
	asr	(sp)			/   tmp = mod >> 3 { signed shift }
	asr	(sp)
	asr	(sp)
	bic	$!7,r1			/ mod = _regloc[mod&7] * sizeof(int)
	movb	_regloc(r1),r1
	asl	r1
	add	r0,r1			/ ar0[mod] -= tmp
	sub	(sp)+,(r1)
	rts	pc

/*
 * backup(ar0)
 *	caddr_t ar0;
 *
 * Back up user CPU state after aborted instruction.  Called from trap when
 * attempting an automatic stack grow to restore the user's state prior to
 * a stack fault.  Returns zero if successful.
 */
#ifndef KERN_NONSEP
/*
 * Backup routine for use with machines with SSR1 and SSR2 (saved in ssr+2
 * and ssr+4 by trap).  SSR1 records any auto-increments and/or decrements
 * that were already performed for the aborted instruction.  SSR2 holds
 * the adddress of the instruction which faulted.
 */
ENTRY(backup)
	mov	2(sp),r0		/ r0 = ar0
	movb	ssr+2,r1		/ undo first register modification from
	jsr	pc,unwind		/   saved SSR1
	movb	ssr+3,r1		/   and second ...
	jsr	pc,unwind
	movb	_regloc+7,r1		/ ar0[_regloc[PC] * sizeof(int)]
	asl	r1			/   = saved SSR2
	add	r0,r1
	mov	ssr+4,(r1)
	clr	r0			/ and return success
	rts	pc

#else KERN_NONSEP
/*
 * 11/40 version of backup, for use with no SSR1 and SSR2.  Actually SSR1
 * usually exists for all processors except the '34 and '40 but always
 * reads as zero on those without separate I&D ...
 */

.bss
bflg:	.=.+1
jflg:	.=.+1
fflg:	.=.+1
.text

ENTRY(backup)
	mov	2(sp),ssr+2		/ pass ar0 to the real backup routine
	mov	r2,-(sp)		/ backup needs an extra register
	jsr	pc,backup
	mov	r2,ssr+2		/ save computed register modification
	mov	(sp)+,r2		/   mask and restore r2
	movb	jflg,r0			/ if backup failed, return failure
	bne	1f
	mov	2(sp),r0		/ r0 = ar0
	movb	ssr+2,r1		/ undo register modifications
	jsr	pc,unwind
	movb	ssr+3,r1
	jsr	pc,unwind
	movb	_regloc+7,r1		/ ar0[_regloc[PC] * sizeof(int)] =
	asl	r1			/   computed fault pc
	add	r0,r1
	mov	ssr+4,(r1)
	clr	r0			/ and indicate success
2:
	rts	pc

/*
 * Hard part: simulate the ssr2 register missing on 11/40 and similar
 * processors.
 */
backup:
	clr	r2			/ backup register ssr1
	mov	$1,bflg			/ clrs jflg
	clrb	fflg
	mov	ssr+4,r0
	jsr	pc,fetch
	mov	r0,r1
	ash	$-11.,r0
	bic	$!36,r0
	jmp	*0f(r0)
0:		t00; t01; t02; t03; t04; t05; t06; t07
		t10; t11; t12; t13; t14; t15; t16; t17

t00:
	clrb	bflg

t10:
	mov	r1,r0
	swab	r0
	bic	$!16,r0
	jmp	*0f(r0)
0:		u0; u1; u2; u3; u4; u5; u6; u7

u6:					/ single op, m[tf]pi, sxt, illegal
	bit	$400,r1
	beq	u5			/ all but m[tf], sxt
	bit	$200,r1
	beq	1f			/ mfpi
	bit	$100,r1
	bne	u5			/ sxt

	/ Simulate mtpi with double (sp)+,dd.
	bic	$4000,r1		/ turn instr into (sp)+
	br	t01

	/ Simulate mfpi with double ss,-(sp).
1:
	ash	$6,r1
	bis	$46,r1			/ -(sp)
	br	t01

u4:					/ jsr
	mov	r1,r0
	jsr	pc,setreg		/ assume no fault
	bis	$173000,r2		/ -2 from sp
	rts	pc

t07:					/ EIS
	clrb	bflg

u0:					/ jmp, swab
u5:					/ single op
f5:					/ movei, movfi
ff1:					/ ldfps
ff2:					/ stfps
ff3:					/ stst
	mov	r1,r0
	br	setreg

t01:					/ mov
t02:					/ cmp
t03:					/ bit
t04:					/ bic
t05:					/ bis
t06:					/ add
t16:					/ sub
	clrb	bflg

t11:					/ movb
t12:					/ cmpb
t13:					/ bitb
t14:					/ bicb
t15:					/ bisb
	mov	r1,r0
	ash	$-6,r0
	jsr	pc,setreg
	swab	r2
	mov	r1,r0
	jsr	pc,setreg

	/ If delta(dest) is zero, no need to fetch source.
	bit	$370,r2
	beq	1f

	/ If mode(source) is R, no fault is possible.
	bit	$7000,r1
	beq	1f

	/ If reg(source) is reg(dest), too bad.
	mov	r2,-(sp)
	bic	$174370,(sp)
	cmpb	1(sp),(sp)+
	beq	u7

	/ Start source cycle.  Pick up value of reg.
	mov	r1,r0
	ash	$-6,r0
	bic	$!7,r0
	movb	_regloc(r0),r0
	asl	r0
	add	ssr+2,r0
	mov	(r0),r0

	/ If reg has been incremented, must decrement it before fetch.
	bit	$174000,r2
	ble	2f
	dec	r0
	bit	$10000,r2
	beq	2f
	dec	r0
2:

	/ If mode is 6,7 fetch and add X(R) to R.
	bit	$4000,r1
	beq	2f
	bit	$2000,r1
	beq	2f
	mov	r0,-(sp)
	mov	ssr+4,r0
	add	$2,r0
	jsr	pc,fetch
	add	(sp)+,r0
2:

	/ Fetch operand.  If mode is 3, 5, or 7, fetch *.
	jsr	pc,fetch
	bit	$1000,r1
	beq	1f
	bit	$6000,r1
	bne	fetch
1:
	rts	pc

t17:					/ floating point instructions

	clrb	bflg
	mov	r1,r0
	swab	r0
	bic	$!16,r0
	jmp	*0f(r0)
0:		f0; f1; f2; f3; f4; f5; f6; f7

f0:
	mov	r1,r0
	ash	$-5,r0
	bic	$!16,r0
	jmp	*0f(r0)
0:		ff0; ff1; ff2; ff3; ff4; ff5; ff6; ff7

f1:					/ mulf, modf
f2:					/ addf, movf
f3:					/ subf, cmpf
f4:					/ movf, divf
ff4:					/ clrf
ff5:					/ tstf
ff6:					/ absf
ff7:					/ negf
	inc	fflg
	mov	r1,r0
	br	setreg

f6:
	bit	$400,r1
	beq	f1			/ movfo
	br	f5			/ movie

f7:
	bit	$400,r1
	beq	f5			/ movif
	br	f1			/ movof

ff0:					/ cfcc, setf, setd, seti, setl

u1:					/ br
u2:					/ br
u3:					/ br
u7:					/ illegal
	incb	jflg
	rts	pc

setreg:
	mov	r0,-(sp)
	bic	$!7,r0
	bis	r0,r2
	mov	(sp)+,r0
	ash	$-3,r0
	bic	$!7,r0
	movb	0f(r0),r0
	tstb	bflg
	beq	1f
	bit	$2,r2
	beq	2f
	bit	$4,r2
	beq	2f
1:
	cmp	r0,$20
	beq	2f
	cmp	r0,$-20
	beq	2f
	asl	r0
2:

	tstb	fflg
	beq	3f
	asl	r0
	stfps	r1
	bit	$200,r1
	beq	3f
	asl	r0
3:

	bisb	r0,r2
	rts	pc

0:	.byte	0,0,10,20,-10,-20,0,0

fetch:
	bic	$1,r0
	mov	nofault,-(sp)
	mov	$1f,nofault
	mfpi	(r0)
	mov	(sp)+,r0
	mov	(sp)+,nofault
	rts	pc

1:
 	mov	(sp)+,nofault
	clrb	r2			/ clear out dest on fault
	mov	$-1,r0
	rts	pc
#endif KERN_NONSEP

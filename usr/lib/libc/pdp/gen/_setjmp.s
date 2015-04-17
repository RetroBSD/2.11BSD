/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)_setjmp.s	1.2 (Berkeley) 1/5/87\0>
	.even
#endif LIBC_SCCS

/*
 * val = _setjmp(env)
 *	int	val;
 *	jmp_buf	*env;
 *
 * _longjmp(env, val)
 *	jmp_buf	*env;
 *	int	val;
 *
 * Overlaid version of _setjmp and _longjmp.  MUST BE LOADED IN BASE SEGMENT!!!
 *
 * _Longjmp(env,val) will generate a "return(val)" from the last call to
 * _setjmp(env) by restoring the general registers r2, r3, and r4 from the
 * stack, rolling back the frame pointer (r5) to the env constructed by
 * _setjmp.
 *
 * For the overlaid _setjmp/_longjmp pair, the jmp_buf looks like:
 *
 *	-------------------------
 *	| overlay number	|
 *	|-----------------------|
 *	| frame pointer		|
 *	|-----------------------|
 *	| stack pointer		|
 *	|-----------------------|
 *	| return address	|
 *	-------------------------
 */
#include "DEFS.h"

emt	= 0104000			/ overlay switch - ovno in r0
.globl	__ovno

ENTRY(_setjmp)
	mov	2(sp),r0		/ r0 = env
	mov	__ovno,(r0)+		/ save caller's current overlay
	mov	r5,(r0)+		/   and frame pointer
	mov	sp,(r0)			/ calculate caller's pre jsr pc,
	add	$2,(r0)+		/   _setjmp sp as (sp + ret addr)
	mov	(sp),(r0)		/ save return pc
	clr	r0			/    and return a zero
	rts	pc

/*
 * Note that no qualification for returning to the base segment is made in
 * _longjmp (to avoid a posible superfluous overlay switch request) as cret
 * does.  The problem is the routine being returned to in the base could
 * easily be one which doesn't use cret itself to return (like a system call
 * from the library) which on its own return might be returning to a different
 * overlay than the one currently loaded.
 *
 */
.globl	rollback
ENTRY(_longjmp)
	mov	2(sp),r1		/ r1 = env
	mov	(r1)+,r0		/ r0 = env[overlay number]
	beq	1f			/ zero implies no overlay switches yet
	cmp	r0,__ovno		/ same overlay currently loaded?
	beq	1f			/   yes, nothing to do
	emt				/   no, request overlay load
	mov	r0,__ovno		/     and indicate new overlay

	/ The last two instructions represent a potential race condition ...
1:
	mov	(r1)+,r0		/ r0 = env[frame pointer]
	jsr	pc,rollback		/ attempt stack frame rollback
	mov	4(sp),r0		/ r0 = val
	bne	2f			/ if (val == 0)
	inc	r0			/   r0 = 1
2:
	mov	(r1)+,sp		/ restore stack pointer
	jmp	*(r1)			/   and simulate return

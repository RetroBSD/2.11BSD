/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	defined(LIBC_SCCS) && !defined(lint)
	<@(#)setjmp.s	1.5 (2.11BSD GTE) 1997/9/7\0>
	.even
#endif

/*
 * val = setjmp(env)
 *	int	val;
 *	jmp_buf	*env;
 *
 * longjmp(env, val)
 *	jmp_buf	*env;
 *	int	val;
 *
 * Overlaid version of setjmp and longjmp.  MUST BE LOADED IN BASE SEGMENT!!!
 *
 * Longjmp(env,val) will generate a "return(val)" from the last call to
 * setjmp(env) by restoring the general registers r2, r3, and r4 from the
 * stack and performing a sigreturn on the sigcontext constructed in env by
 * setjmp, see <signal.h>.
 */
#include "DEFS.h"

SIG_SETMASK = 3				/ XXX - from signal.h

.globl	_sigaltstack, _sigprocmask	/ needed to create sigcontext
.globl	__ovno

ENTRY(setjmp)
	mov	r2,-(sp)	/ save r2
	mov	4(sp),r2	/ r2 = env
	sub	$6.,sp		/ allocate sizeof(struct sigaltstack)
	mov	sp,r0		/   and get current sigaltstack via
	mov	r0,-(sp)	/   sigaltstack(0, sp) (cant use "mov sp,-(sp)")
	clr	-(sp)
	jsr	pc,_sigaltstack
	add	$8.,sp		/ toss 0, &oss, ss_sp, ss_size,
	mov	(sp)+,(r2)+	/   save ss_flags of caller

	sub	$4,sp		/ sizeof (sigset_t) - oset
	mov	sp,r0		/ can't use mov sp,-(sp)
	mov	r0,-(sp)	/ 'oset'
	clr	-(sp)		/ 'set'
	mov	$SIG_SETMASK,-(sp)	/ 'how'
	jsr	pc,_sigprocmask	/ sigprocmask(SIG_SETMASK, NULL, &oset)
	add	$6,sp		/ toss how, set, &oset
	mov	(sp)+,(r2)+	/ oset(hi) to env
	mov	(sp)+,(r2)+	/ oset(lo) to env

	mov	sp,(r2)		/ calculate caller's pre jsr pc,setjmp
	add	$4,(r2)+	/   sp as (sp + saved r2 + ret addr)
	mov	r5,(r2)+	/ save caller's frame pointer
	cmp	(r2)+,(r2)+	/   fake r0 & r1
	mov	2(sp),(r2)+	/   return pc,
	mov	$0170000,(r2)+	/   fake (but appropriate) ps, and
	mov	__ovno,(r2)	/   current overlay
	mov	(sp)+,r2	/ restore r2
	clr	r0		/ and return a zero
	rts	pc

SC_FP	= 8.			/ offset of sc_fp in sigcontext
SC_R0	= 12.			/ offset of sc_r0 in sigcontext

.globl	rollback, _sigreturn, _longjmperror
ENTRY(longjmp)
	mov	2(sp),r1	/ r1 = env
	mov	SC_FP(r1),r0	/ r0 = env->sc_fp
	jsr	pc,rollback	/ attempt stack frame rollback
	mov	4(sp),r0	/ r0 = val
	bne	1f		/ if (val == 0)
	inc	r0		/   r0 = 1
1:
	mov	r0,SC_R0(r1)	/ env->sc_r0 = r0 (`return' val)
	mov	2(sp),-(sp)	/ push env
	jsr	pc,_sigreturn	/ perform sigreturn(env)
	jsr	pc,_longjmperror / if sigreturn returns, it's an error
	iot			/ and die if longjmperror returns

/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	defined(LIBC_SCCS) && !defined(lint)
	<@(#)rollback.s	1.4 (2.11BSD GTE) 1/1/94\0>
	.even
#endif

/*
 * rollback(fp::r0)
 *	caddr_t	fp;
 *
 * Overlaid version of rollback.
 *
 * This routine is used in conjuction with the various versions of longjmp to
 * roll back r5 to a saved stack frame and restore r2, r3, and r4 from the
 * immediately nested stack frame.
 *
 * Note that the register restore can fail if the routine being jumped to calls
 * an assembly routine that uses one or more of the registers without creating
 * a stack frame to save the old values of the registers.  E.g. the assembly
 * routine simply "pushes" any registers it uses then "pops" them afterwards.
 * This causes rollback to fail because there's no stack frame to pull the
 * original values of the registers from.
 *
 *	Caveat:	Don't use register variables in routines using setjmp's!
 */

.globl	_longjmperror			/ just in case we roll off the end ...
.globl	rollback
rollback:
	cmp	r5,r0			/ if (we're longjmp'ing to ourselves)
	beq	3f			/   return;
1:					/ do {
	cmp	(r5),r0			/   if (the next frame is The Frame)
	beq	2f			/     goto 2f;
	mov	(r5),r5			/ while ((r5 = r5->next_frame) != 0)
	bne	1b
	jsr	pc,_longjmperror	/ call longjmperror
	iot				/   and die if we return
2:
	mov	r5,r2			/ r2 = immediately nested frame
	tst	-(r2)			/ skip past saved overlay number
	mov	-(r2),r4		/ restore r4,
	mov	-(r2),r3		/   r3, and
	mov	-(r2),r2		/   r2
	mov	(r5),r5			/ r5 = desired frame
3:
	rts	pc			/   and return

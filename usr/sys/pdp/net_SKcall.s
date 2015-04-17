/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)net_SKcall.s	1.1 (2.10BSD Berkeley) 4/3/88
 */

#include "DEFS.h"
#include "../machine/mch_iopage.h"
/*
 * Inter-address-space procedure call subroutines.
 *	SKcall -- supervisor to kernel
 */

/*
 * SKcall(func, nbytes, a1, ..., aN)
 *
 * C-call from supervisor to kernel.
 * Calls func(a1, ..., aN) in kernel mode.
 */
ENTRY(SKcall)
	mov	r5,-(sp)
	mov	PS,-(sp)	/ Save current PS
	mov	010(sp),r0	/ r0: # bytes of arguments to copy

	/*
	 * Allocate enough space on the kernel stack to hold
	 * the call frame.  We must allocate space to hold all the
	 * arguments, plus:
	 *	a PS, PC pair to take us to the function being called
	 *	    in kernel space.
	 *	a return address (Kretu)
	 *	a null R5 entry as a marker
	 *	a PS, PC pair to take us back to supervisor mode.
	 *
	 * We assume that if the kernel stack pointer is pointing to
	 * an address before the start of seg6, the kernel stack has
	 * been remapped and we must use mtpd instructions instead
	 * of simple mov's.
	 *
	 *			WARNING:
	 *			=======
	 *
	 * The reason we use mtpd is to guarantee that we write into
	 * the kernel stack.  However, when the kernel stack is remapped
	 * to point outside of seg 6, there may not be enough room for
	 * storing the arguments in the new stack.  There is currently no
	 * check made here to prevent overflowing this remapped stack.
	 */

	mov	$040340,PS	/ Current SUPV, prev KERN, spl7 (paranoia)
	mfpd	sp		/ Kernel stack pointer
	mov	(sp),r1		/ Pointer to base of new call frame
	sub	r0,(sp)
	sub	$12.,(sp)	/ New SP.
	mtpd	sp		/ Set up alternate stack ptr

	cmp	r1,$_u		/ Is kernel SP pointing in seg6?
	bhi	skmov		/ Yes: OK to use normal mov instructions
	jmp	skmtpd		/ No: must use mtpd instead

skmov:
	mov	(sp),PS		/ Restore original PS
	mov	(sp)+,-(r1)	/ Transfer saved PS to alternate stack
	mov	$ret,-(r1)	/ Will return to ret below
	mov	r5,-(r1)	/ Old frame ptr
	mov	r1,r5		/ New frame ptr for other mode

	/*
	 * Copy arguments.
	 * Note that the stack offsets here are the same as they
	 * were right after we saved the PS.
	 */

	mov	r2,-(sp)	/ Save
	mov	r0,r2		/ r2: number of bytes of arguments to copy
	beq	2f		/ None to copy? if so, skip loop
	asr	r2		/ r2: number of words of arguments to copy
	add	sp,r0
	add	$10.,r0		/ r0: ptr to argN+1
1:	
	mov	-(r0),-(r1)
	sob	r2,1b
2:
	mov	(sp)+,r2	/ Restore

	/*
	 * Build the rest of the kernel stack frame.
	 * This consists of a normal return address (Kretu) to which
	 * the kernel function will return, and a <PS, PC> pair used
	 * by Kretu to get back here to supervisor mode.
	 */

	mov	$Kretu,-(r1)	/ Address of RTT-er in kernel mode
	mov	$30000,-(r1)	/ New PS mode: (current KERN, prev USER)
	bisb	PS,(r1)		/ New PS priority
	mov	04(sp),-(r1)	/ Stack address of function to be called

skcall:
	/*
	 * At this point, the stacks look like the following:
	 *
	 *	SUPERVISOR		KERNEL
	 *	----------		------
	 *SSP->	<R5>			<KERN func address>	<-KSP
	 *	<caller's rtnaddr>	<KERN PS>
	 *	<KERN func address>	<Kretu>
	 *	<# bytes of args>	<arg0>
	 *	<arg0>			 ...
	 *	 ...			<argN>
	 *	<argN>			 <0>
	 *				<ret in SUPV space>
	 *				<PS for SUPV mode>
	 *
	 * "<KERN func address>" is the address in kernel address
	 * space of the function being called.
	 */

	iot

	/*
	 * Return to caller of SKcall.
	 * We arrive here with the PS being the same as it was on
	 * calling SKcall, via a "rtt" from Kretu in kernel mode.
	 */
ret:
	mov	(sp)+,r5
	rts	pc

	/*
	 * The kernel stack has been repointed.
	 * Use mtpd to set up the kernel stack for the call.
	 *
	 * We expect that r1 contains the address in kernel space
	 * of the base of the call frame we are about to build.
	 * The word on the top of the stack is the original PS.
	 */

skmtpd:
	mov	(sp),PS		/ Restore original PS (mainly for priority)
	bic	$30000,PS	/ Previous mode is kernel
	mtpd	-(r1)		/ Transfer saved PS to alternate stack
	mov	$ret,-(sp)	/ Will return to ret below
	mtpd	-(r1)
	mov	r5,-(sp)	/ Old frame ptr
	mtpd	-(r1)
	mov	r1,r5		/ New frame ptr for other mode

	/*
	 * Copy arguments.
	 * Note that the stack offsets here are the same as they
	 * were right after we saved the PS.
	 */

	mov	r2,-(sp)	/ Save
	mov	r0,r2		/ r2: number of bytes of arguments to copy
	beq	2f		/ None to copy? if so, skip loop
	asr	r2		/ r2: number of words of arguments to copy
	add	sp,r0
	add	$10.,r0		/ r0: ptr to argN+1
1:	
	mov	-(r0),-(sp)
	mtpd	-(r1)
	sob	r2,1b
2:
	mov	(sp)+,r2	/ Restore

	/*
	 * Build the rest of the kernel stack frame.
	 * This consists of a normal return address (Kretu) to which
	 * the kernel function will return, and a <PS, PC> pair used
	 * by Kretu to get back here to supervisor mode.
	 */

	mov	$Kretu,-(sp)	/ Address of RTT-er in kernel mode
	mtpd	-(r1)
	mov	$010000,-(sp)	/ New PS mode: (current KERN, prev SUPV)
	bisb	PS,(sp)		/ New PS priority
	mtpd	-(r1)
	mov	04(sp),-(sp)	/ Stack address of function to be called
	mtpd	-(r1)
	jmp	skcall		/ Back to complete the call


/*
 * Sretu is the exit point in supervisor code that cleans up
 * the supervisor stack for return to kernel mode from a
 * kernel-to-net procedure call.
 *
 * We can't use rtt here, so instead we use the a "iot"
 * in just the same way as we would use to "call" something
 * in the kernel.
 */
ASENTRY(Sretu)
	mov	r5,sp		/ Base of frame area
	tst	(sp)+		/ Throw away old frame pointer
	/*
	 * At this point, the stacks look like the following:
	 *
	 *	KERNEL			SUPERVISOR
	 *	------			----------
	 *				<_Sretu>
	 *				<arg0>
	 *KSP->	<ret in KERN space>	 ...
	 *	<R5>			<argN>
	 *	<caller's rtnaddr>	 <0>
	 *	<SUPV func address>	<old stack top>	<-SSP
	 *	<# bytes of args>
	 *	<arg0>
	 *	 ...
	 *	<argN>
	 */
	iot

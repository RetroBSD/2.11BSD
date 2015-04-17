/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_KScall.s	1.1 (2.10BSD Berkeley) 4/3/88
 */

#include "DEFS.h"
#include "../machine/mch_iopage.h"

/*
 * Inter-address-space procedure call subroutines.
 *	KScall -- kernel to supervisor
 */

/*
 * KScall(func, nbytes, a1, ..., aN)
 *
 * C-call from kernel to supervisor.
 * Calls func(a1, ..., aN) in supervisor mode.
 */
ENTRY(KScall)
	mov	r5,-(sp)
	mov	PS,-(sp)	/ Save current PS
	mov	010(sp),r0	/ r0: # bytes of arguments to copy

	/*
	 * Allocate enough space on the supervisor stack to hold
	 * the call frame.  We must allocate space to hold all the
	 * arguments, plus:
	 *	a null R5 entry as a marker
	 *	a return address (Sretu)
	 *
	 * We assume here that the supervisor stack pointer is always
	 * in seg6 and further that both supervisor and kernel addresses
	 * in this region are mapped to the same physical memory.
	 *
	 * This means that it is NOT ok for the network code to move the
	 * supervisor SP outside of seg6, or for it to remap SDSA6 to
	 * point to something other than the stack, unless it can guarantee
	 * that KScall will not be called for the duration of the stack
	 * fiddling.
	 */
	mov	$010340,PS	/ Current KERN, prev SUPV, spl7 (paranoia)
	mfpd	sp		/ Supervisor stack pointer
	mov	(sp),r1		/ Pointer to base of call frame

#ifdef	CHECKSTACK
	/*
	 * The following is just to make sure that the supervisor stack
	 * pointer looks reasonable and a check that *SDSA6 == *KDSA6
	 */
	cmp	r1,$_u
	blos	8f
	cmp	r1,$USIZE\<6.+_u
	bhis	8f
	cmp	SDSA6,KDSA6
	beq	9f
8:
	spl	7		/ lock out interrupts
	halt			/ drop dead so people can see where we are
	br	.		/ go no further
9:
#endif /* CHECKSTACK */

	sub	r0,(sp)
	sub	$4.,(sp)	/ New SP.
	mtpd	sp		/ Set up alternate stack ptr
	mov	(sp),PS		/ Restore current PS
	mov	r5,-(r1)	/ Old frame ptr
	mov	r1,r5		/ New frame ptr for other mode

	/*
	 * Copy arguments.
	 * Note that the stack offsets here are one word deeper than
	 * after we saved the PS.
	 */

	mov	r2,-(sp)	/ Save
	mov	r0,r2		/ r2: number of bytes of arguments to copy
	beq	2f		/ None to copy? if so, skip loop
	asr	r2		/ r2: number of words of arguments to copy
	add	sp,r0
	add	$12.,r0		/ r0: ptr to argN+1
1:	
	mov	-(r0),-(r1)
	sob	r2,1b
2:
	mov	(sp)+,r2	/ Restore

	/*
	 * Build the remainder of the frame on the
	 * supervisor stack.
	 */
	mov	$Sretu,-(r1)	/ Return address in supervisor space

	/*
	 * Push the iothndlr <PS, PC> pair, and the
	 * <PS, PC> pair for calling the supervisor function
	 * on the kernel stack.
	 */
	mov	PS,r0		/ To get current priority
	bic	$170000,r0	/ Clear out current/previous modes
	bis	$050000,r0	/ Current SUPV, previous SUPV
	mov	$ret,-(sp)	/ Stack kernel PC to go with saved kernel PS
	mov	r0,-(sp)	/ Stack the new PS
	mov	10.(sp),-(sp)	/ Stack the address of function being called

	/*
	 * At this point, the stacks look like the following:
	 *
	 *	KERNEL			SUPERVISOR
	 *	------			----------
	 *KSP->	<SUPV func address>	<Sretu>	<-SSP
	 *	<SUPV PS>		<arg0>
	 *	<KERN "ret">		 ...
	 *	<KERN saved PS>	<argN>
	 *	<R5>			 <0>
	 *	<caller's rtnaddr>
	 *	<SUPV func address>
	 *	<# bytes of args>
	 *	<arg0>
	 *	 ...
	 *	<argN>
	 *
	 * "<SUPV func address>" is the address in supervisor address
	 * space of the function being called.
	 */
	rtt			/ Call the function

	/*
	 * Return to caller of KScall.
	 * We arrive here with the same PS as that with which we were
	 * called, via the rtt in iothndlr.
	 */
ret:
	mov	(sp)+,r5
	rts	pc


/*
 * Kretu is the exit point in kernel code that cleans up
 * the kernel stack for return to supervisor mode from a
 * net-to-kernel procedure call.
 */
ASENTRY(Kretu)
	mov	r5,sp		/ Throw away arguments from stack
	tst	(sp)+		/ Discard old frame pointer
	/*
	 * At this point, the stacks look like the following:
	 *
	 *	SUPERVISOR		KERNEL
	 *	----------		------
	 *SSP->	<R5>
	 *	<caller's rtnaddr>	<_Kretu>
	 *	<KERN func address>	<arg0>
	 *	<# bytes of args>	 ...
	 *	<arg0>			<argN>
	 *	 ...			 <0>
	 *	<argN>			<ret in SUPV space>	<-KSP
	 *				<PS for SUPV mode>
	 */
	rtt			/ Back to previous address space

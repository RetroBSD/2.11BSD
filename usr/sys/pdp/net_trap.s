/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)net_trap.s	1.1 (2.10BSD Berkeley) 4/3/88
 */

#include "DEFS.h"
#include "../machine/mch_iopage.h"

/*
 * All interrupts to the network kernel are vectored through this routine.
 * No attempt is made to force rescheduling of the cpu.
 *
 * Since the network kernel does not use overlays, we don't have to
 * worry about them here.  If a network interrupt service routine makes
 * a call to the kernel, then Kretu over on the kernel side will take
 * care of saving and restoring the overlay number.
 */

ASENTRY(call)
	mov	PS, -(sp)		/ save new PS
	mov	r1,-(sp)		/ save r1
	mfpd	sp			/ save old sp
	mov	4(sp), -(sp)		/ copy of new PS
	bic	$!37,(sp)		/ extract dev or unit
	jsr	pc,(r0)+		/   and execute trap handler
	cmp	(sp)+,(sp)+		/ discard dev and sp
	/*
	 * An 'rtt' here might not work since supervisor can't rtt to the
	 * kernel, instead, we use the "iot" mechanism to schedule a rtt
	 * from kernel mode to the PS, PC pair we push on the kernel stack.
	 *
	 * Transfer our saved <PS, PC> pair to the kernel stack.  The spl7
	 * below is pure paranoia, BUT while we're at it let's bump the
	 * interrupt count - it's a mere two instructions of overhead (the
	 * increment would have been done anyhow)!
	 */
	mov	PS,-(sp)
	mov	$40340,PS		/ current mode SUPV, prev KERN, BR7
#ifdef	UCB_METER
	mfpd	*$_cnt+V_INTR		/ fetch interrupt count
	inc	(sp)			/ bump it
	mtpd	*$_cnt+V_INTR		/ put it back
#endif
	mfpd	sp			/ old kernel stack pointer
	mov	(sp),r1
	sub	$4,(sp)			/ grow the kernel stack
	mtpd	sp

	/*
	 * Can't assume kernel stack in seg6, this is slower but safe.
	 */
	mov	12(sp),-(sp)		/ push PS for iothndlr
	mtpd	-(r1)
	mov	10(sp),-(sp)		/ push PC for iothndlr
	mtpd	-(r1)
	mov	(sp)+,PS

	mov	(sp)+,r1		/ restore r1,
	tst	(sp)+			/   discard new PS,
	mov	(sp)+,r0		/   restore r0,
	cmp	(sp)+,(sp)+		/   discard PS and PC
	iot				/ have the kernel do our rtt for us

/*
 * Scan net interrupt status register (netisr) for service requests.
 */
ENTRY(netintr)
	mov	r2,-(sp)		/ use r2 for scan of netisr
1:
	SPL7
	mov	_netisr,r2		/ get copy of netisr
	clr	_netisr			/   and clear it, atomically
	SPLNET
#ifdef INET
	bit	$1\<NETISR_IP,r2	/ IP scheduled?
	beq	2f
	jsr	pc,_ipintr
2:
#endif
#ifdef NS
	bit	$1\<NETISR_NS,r2	/ NS scheduled?
	beq	3f
	jsr	pc,_nsintr
3:
#endif
#ifdef IMP
	bit	$1\<NETISR_IMP,r2	/ IMP scheduled?
	beq	4f
	jsr	pc,_impintr
4:
#endif
	bit	$1\<NETISR_RAW,r2	/ RAW scheduled?
	beq	5f			/   (checked last since other
	jsr	pc,_rawintr		/   interrupt routines schedule it)
5:
	tst	_netisr			/ see if any new scheduled events
	bne	1b			/   if so, handle them
	mov	(sp)+,r2		/ restore r2,
	rts	pc			/   and return

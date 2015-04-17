/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_trap.s	2.2 (2.11BSD GTE) 1/9/94
 */
#include "DEFS.h"
#include "../machine/mch_iopage.h"
#include "../machine/koverlay.h"
#include "../machine/trap.h"


SPACE(LOCAL, saveps, 2)

/*
 *	jsr	r0,call
 *	jmp	trap_handler
 *
 *	mov	PS,saveps
 *	jsr	r0,call1
 *	jmp	trap_handler
 *
 * Interrupt interface code to C.  Creates an interrupt stack frame, calls
 * the indicated trap_handler.  Call forces the previous mode to be user so
 * that copy in/out and friends use user space if called from interrupt
 * level.  Call1 is simply an alternate entry to call that assumes that the
 * PS has already been changed and the original PS can be found in saveps.
 * Call1 also doesn't increment cnt.v_intr since it's only called from
 * synchronous software traps.
 *
 * The interrupt stack frame - note that values in <pdp/reg.h> must agree:
 *
 *				  reg.h
 *		| ps		|   2	/ ps and pc saved by interrupt
 *		| pc		|   1
 *	u.u_ar0	| r0		|   0
 *		| nps		|	/ from interrupt vector
 *		| ovno		|
 *		| r1		|  -3
 *		| sp		|  -4
 *		| code		|	/ nps & 037
 *	--------------------------------/ above belongs to call, below to csv
 *		| ret		|	/ return address to call
 *	r5	| old r5	|  -7
 *		| ovno		|
 *		| r4		|  -9
 *		| r3		| -10
 *		| r2		| -11
 *	sp	| *empty*	|
 */
call1:
	mov	saveps,-(sp)		/ stuff saved nps into interrupt frame
	SPLLOW				/ ensure low priority (why?)
	br	1f			/ branch around count of interrupts

ASENTRY(call)
	/*
	 * Interrupt entry code.  Save various registers, etc., and call
	 * interrupt service routine.
	 */
	mov	PS,-(sp)		/ stuff nps into interrupt frame
#ifdef UCB_METER
	inc	_cnt+V_INTR		/ cnt.v_intr++
#endif
1:
	mov	__ovno,-(sp)		/ save overlay number,
	mov	r1,-(sp)		/   r1,
	mfpd	sp			/   sp
	mov	6(sp),-(sp)		/ grab nps and calculate
	bic	$!37,(sp)		/   code = nps & 037
	bis	$30000,PS		/ force previous mode = user
	jsr	pc,(r0)			/ call trap_handler
#ifdef INET
	/*
	 * Check for scheduled network service requests.  The network sets
	 * _knetisr to schedule network activity at a later time when the
	 * system IPL is low and things are ``less hectic'' ...
	 */
	mov	PS,-(sp)		/ set SPL7 so _knetisr doesn't get
	SPL7				/   changed while we're looking at it
	tst	_knetisr		/ if (_knetisr != 0
	beq	2f
	bit	$340,20(sp)		/  && interrupted IPL == 0
	bne	2f			/  [shouldn't we check against SPLNET?]
	cmp	sp,$_u			/  && sp > _u (not on interrupt stack))
	blos	2f
#ifdef UCB_METER
	inc	_cnt+V_SOFT		/   increment soft interrupt counter,
#endif
	clr	_knetisr		/   reset the flag,
	SPLNET				/   set network IPL,
	clr	-(sp)			/   and KScall(netintr, 0)
	mov	$_netintr,-(sp)
	jsr	pc,_KScall
	cmp	(sp)+,(sp)+
2:
	mov	(sp)+,PS		/ restore PS
#endif
	/*
	 * Clean up from interrupt.  If previous mode was user and _runrun
	 * is set, generate an artificial SWITCHTRAP so we can schedule in a
	 * new process.
	 */
	bit	$20000,10(sp)		/ previous mode = user??
	beq	4f
	tstb	_runrun			/ yep, is the user's time up?
	beq	3f
	mov	$T_SWITCHTRAP,(sp)	/ yep, set code to T_SWITCHTRAP
	jsr	pc,_trap		/   and give up cpu
3:
	/*
	 * Trap from user space: toss code and reset user's stack pointer.
	 */
	tst	(sp)+			/ toss code, reset user's sp
	mtpd	sp			/   and enter common cleanup code
	br	5f
4:
	/*
	 * Trap from kernel or supervisor space: toss code and stack pointer
	 * (only user stack pointers need to be set or reset).
	 */
	cmp	(sp)+,(sp)+		/ trap from kernel or supervisor:
5:					/   toss code and sp
	/*
	 * Finish final clean up, restore registers, etc. make sure we
	 * leave the same overlay mapped that we came in on, and return
	 * from the interrupt.
	 */
	mov	(sp)+,r1		/ restore r1

	mov	(sp)+,r0		/ current overlay different from
	cmp	r0,__ovno		/   interrupted overlay?
	beq	6f
	SPL7				/ yes, go non-interruptable (rtt
					/   below restores PS)
	mov	r0,__ovno		/ reset ovno and mapping for
	asl	r0			/   interrupted overlay
	mov	ova(r0),OVLY_PAR
	mov	ovd(r0),OVLY_PDR
6:
	tst	(sp)+			/ toss nps
	mov	(sp)+,r0		/ restore r0
	rtt				/ and return from the trap ...


#ifdef INET
/*
 * iothndlr is used to allow the network in supervisor mode to make calls
 * to the kernel.
 *
 * the network pushes a <pc,ps> pair on the kernel stack before doing the
 * 'iot'.  when we process the iot here we throw the saved <pc,ps> pair 
 * resulting from the 'iot' away and use instead the pair pushed by the
 * network as the target for our 'rtt'.
 *
 * there was a warning that this hasn't been tested with overlays in the
 * kernel.  can't see why it wouldn't work.
 */
ASENTRY(iothndlr)
	mov	PS,saveps		/ save PS in case we have to trap
	bit	$20000,PS		/ previous mode = supervisor?
	bne	trap1			/   (no, let trap handle it)
	bit	$10000,PS
	beq	trap1			/   (no, let trap handle it)
	cmp	(sp)+,(sp)+		/ yes, toss iot frame and execute rtt
	rtt				/   on behalf of networking kernel
#endif


/*
 * System call interface to C code.  Creates a fake interrupt stack frame and
 * then vectors on to the real syscall handler.  It's not necessary to
 * create a real interrupt stack frame with everything in the world saved
 * since this is a synchronous trap.
 */
ASENTRY(syscall)
	mov	PS,saveps		/ save PS just in case we need to trap
	bit	$20000,PS		/ trap from user space?
	beq	trap2			/ no, die
	mov	r0,-(sp)
	cmp	-(sp),-(sp)		/ fake __ovno and nps - not needed
	mov	r1,-(sp)
	mfpd	sp			/ grab user's sp for argument addresses
	tst	-(sp)			/ fake code - not needed
	jsr	pc,_syscall		/ call syscall and start cleaning up
	tst	(sp)+
	mtpd	sp			/ reload user sp, r1 and r0
	mov	(sp)+,r1		/   (cret already reloaded the other
	cmp	(sp)+,(sp)+		/   registers)
	mov	(sp)+,r0
	rtt				/ and return from the trap


/*
 * Emt takes emulator traps, which are requests to change the overlay for the
 * current process.  If an invalid emt is received, _trap is called.  Note
 * that choverlay is not called with a trap frame - only r0 and r1 are saved;
 * it's not necessary to save __ovno as this is a synchronous trap.
 */
ASENTRY(emt)
	mov	PS,saveps		/ save PS just in case we need to trap
	bit	$20000,PS		/ if the emt isn't from user mode,
	beq	trap2			/   or, the process isn't overlaid,
	tst	_u+U_OVBASE		/   or the requested overlay number
	beq	trap2			/   isn't valid, enter _trap
	cmp	r0,$NOVL
	bhi	trap2
	mov	r0,-(sp)		/ everything's cool, save r0 and r1
	mov	r1,-(sp)		/   so they don't get trashed
	mov	r0,_u+U_CUROV		/ u.u_curov = r0
	mov	$RO,-(sp)		/ map the overlay in read only
#ifdef UCB_METER
	inc	_cnt+V_OVLY		/ cnt.v_ovly++
#endif
	add	$1,_u+U_RU+RU_OVLY+2	/ u.u_ru.ru_ovly++
	adc	_u+U_RU+RU_OVLY
	jsr	pc,_choverlay		/ and get choverlay to bring the overlay in
	tst	(sp)+			/ toss choverlay's paramter,
	mov	(sp)+,r1		/   restore r0 and r1,
	mov	(sp)+,r0
	rtt				/   and return from the trap


/*
 * Traps without specialized catchers get vectored here.  We re-enable memory
 * management because the fault that caused this trap may have been a memory
 * management fault which sometimes causes the contents of the memory managment
 * registers to be frozen so error recovery can be attempted.
 */
ASENTRY(trap)
	mov	PS,saveps		/ save PS for call1
trap1:
	tst	nofault			/ if someone's already got this trap
	bne	catchfault		/   scoped out, give it to them
	/*
	 * save current values of memory management registers in case we
	 * want to back up the instruction that failed
	 */
	mov	SSR0,ssr
#ifndef KERN_NONSEP
	mov	SSR1,ssr+2
#endif
	mov	SSR2,ssr+4
	mov	$1,SSR0			/ re-enable relocation
trap2:
	jsr	r0,call1; jmp _trap	/ and let call take us in to _trap ...
	/*NOTREACHED*/


/*
 * We branch here when we take a trap and find that the variable nofault is
 * set indicating that someone else is interested in taking the trap.
 */
catchfault:
	mov	$1,SSR0			/ re-enable memory management
	mov	nofault,(sp)		/   relocation, fiddle with the
	rtt				/   machine trap frame and boogie


/*
 * Power fail handling.  On power down, we just set up for the next trap.
 * On power up we attempt an autoboot ...
 */
#define	PVECT	24			/ power fail vector

ASENTRY(powrdown)
	mov	$powrup,PVECT		/ on power up, trap to powrup,
	SPL7				/ and loop at high priority
	br	.

#ifndef KERN_NONSEP
	.data
#endif
	/*
	 * Power back on... wait a bit, then attempt a reboot.  Can't do much
	 * since memory management is off (hence we are in "data" space).
	 */
powrup:
	/*
	 * Not sure why these are necessary except that on a 44 it appears
	 * that the first instruction or two of a power up trap do not 
	 * execute properly.
	 */
	nop;nop;nop
	/*
	 * The nested loop below gives 38 seconds of delay on a 11/44 (30 sec
	 * on a 11/93) for controllers to complete selftest after power comes
	 * back up.
	*/
	mov 	$400.,r0
2:
	clr	r1
3:	nop
	sob	r1,3b
	sob	r0,2b

	mov	$RB_POWRFAIL,r4		/ and try to reboot ...
	mov	_bootdev,r3
/*
 * 'jsr' can not be used because there is no stack at the moment (battery
 * backup preserves memory but not registers).  'hardboot' sets up a stack 
 * if it needs one, so it is not necessary to do that here.
*/
	jmp	*$hardboot
	/*NOTREACHED*/
	.text

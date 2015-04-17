/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_profile.s	1.2 (2.11BSD GTE) 12/24/92
 */

/*
 * XXX - Current system profiling code doesn't really work with the new
 * kernel.  As soon as I've figured out how I'm going to implement
 * general profiling, I'll integrate it into the kernel.
 *
 * Casey
 */

#ifdef PROF
/*
 * System profiler
 *
 * Expects to have a KW11-P in addition to the line-frequency clock, and it
 * should be set to BR7.  Uses supervisor I space register 2 and 3 (040000 -
 * 0100000) to maintain the profile.
 */

CONST(GLOBAL, _probsiz, 040000)		/ max prof pc/4 (040000 is everything)
SPACE(GLOBAL, _outside, 2)		/ times outside profiling range
SPACE(GLOBAL, _mode, 4*4)		/ what mode we're spending our time in

/*
 * Process profiling clock interrupts
 */
ENTRY(sprof)
	mov	r0,-(sp)		/ snag a register for our use
	mov	PS,r0			/ r0 = previous mode * 4
	ash	$-10.,r0
	bic	$!14,r0
	add	$1,_mode+2(r0)		/ mode[previous mode]++
	adc	_mode(r0)
	cmp	r0,$14			/ if previous made was user,
	beq	2f			/   we're done ...
	mov	2(sp),r0		/ r0 = (interrupted pc / 4) &~ 0140001
	asr	r0
	asr	r0
	bic	$0140001,r0
	cmp	r0,$_probsiz		/ if r0 >= probsiz,
	blo	1f
	inc	_outside		/   outside++
	br	2f			/   and we're done
1:
	mov	$010340,PS		/ set previous mode to supervisor
#ifdef INET
	mov	SISA2, -(sp)		/ save supervisor mapping
	mov	SISD2, -(sp)
	mov	SISA3, -(sp)
	mov	SISD3, -(sp)
	mov	_proloc, SISA2
	mov	77406, SISD2
	mov	_proloc+2, SISA3
	mov	77406, SISD3
#endif
	mfpi	40000(r0)		/   and increment 040000[r0]
	inc	(sp)			/   (the rtt will reset the PS
	mtpi	40000(r0)		/   properly)
#ifdef INET
	mov	(sp)+, SISD3
	mov	(sp)+, SISA3
	mov	(sp)+, SISD2
	mov	(sp)+, SISA2
#endif
2:
	mov	(sp)+,r0		/ restore the used register
	rtt				/   and return from the interrupt
#endif PROF

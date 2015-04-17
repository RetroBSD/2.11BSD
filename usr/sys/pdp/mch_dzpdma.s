/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_dzpdma.s	1.1 (2.10BSD Berkeley) 2/10/87
 */
#include "DEFS.h"
#include "../machine/mch_iopage.h"
#include "../machine/koverlay.h"
#include "dz.h"


#if NDZ > 0
/*
 * DZ-11 pseudo-DMA interrupt routine.  Called directly from the
 * interrupt vector; the device number is in the low bits of the PS.
 * Calls dzxint when the end of the buffer is reached.  The pdma
 * structure is known to be:
 *
 * struct pdma {
 * 	struct	dzdevice *pd_addr;	/ address of controlling device
 *	char	*p_mem;			/ start of buffer
 *	char	*p_end;			/ end of buffer
 *	struct	tty *p_arg;		/ tty structure for this line
 * };
 */
ASENTRY(dzdma)
	mov	PS,-(sp)		/ save new PS, r0-r3 and __ovno (r0
	mov	r0,-(sp)		/   saved before __ovno so we can
	mov	__ovno,-(sp)		/   use it to restore overlay
	mov	r1,-(sp)		/   mapping if necessary)
	mov	r2,-(sp)
	mov	r3,-(sp)
#ifdef UCB_METER
	/*
	 * 4.3BSD doesn't count this as an interrupt (cnt.v_intr) so we don't
	 * either.
	 */
	inc	_cnt+V_PDMA		/ cnt.v_pdma++
#endif
	mov	12(sp),r3		/ new PS
	bic	$!37,r3			/ extract device number
	ash	$3+3,r3			/ r3 = &_dzpdma[dev*8] - 8 lines per DZ
	add	$_dzpdma,r3
	mov	(r3)+,r2		/ pd_addr in r2; r3 points to p_mem
#ifdef UCB_CLIST
	mov	KDSA5,-(sp)		/ save previous mapping
	mov	KDSD5,-(sp)
	mov	_clststrt,KDSA5		/ map in clists
	mov	_clstdesc,KDSD5
#endif
1:					/ loop until no line is ready
	movb	1(r2),r1		/ dzcsr high byte
	bge	3f			/ test TRDY; branch if none
	bic	$!7,r1			/ extract line number
	ash	$3,r1			/ convert to pdma offset
	add	r3,r1			/ r1 is pointer to pdma.p_mem for line
	mov	(r1)+,r0		/ pdma->p_mem
	cmp	r0,(r1)+		/ cmp p_mem to p_end
	bhis	2f			/ if p_mem >= p_end
	movb	(r0)+,6(r2)		/ dztbuf = *p_mem++
	mov	r0,-4(r1)		/ update p_mem
	br	1b
2:					/ buffer is empty; call dzxint
	mov	(r1),-(sp)		/ p_arg
	jsr	pc,_dzxint		/ r0, r1 are modified!
	tst	(sp)+
	br	1b
					/ no more lines ready; return
3:
#ifdef UCB_CLIST
	mov	(sp)+,KDSD5
	mov	(sp)+,KDSA5		/ restore previous mapping
#endif
	mov	(sp)+,r3		/ restore saved registers
	mov	(sp)+,r2
	mov	(sp)+,r1
	SPL7
	mov	(sp)+,r0		/ overlays get switched while we were
	cmp	r0,__ovno		/   doing our thing?
	beq	4f
	mov	r0,__ovno		/ yes, have to restore the earlier
	asl	r0			/   overlay mapping
	mov	ova(r0),OVLY_PAR
	mov	ovd(r0),OVLY_PDR
4:
	mov	(sp)+,r0		/ restore r0, toss new PS value
	tst	(sp)+			/   and return from the interrupt
	rtt
#endif NDZ > 0

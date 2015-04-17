/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_click.s	1.4 (2.11BSD GTE) 1995/11/22
 */

#include "DEFS.h"
#include "../machine/mch_iopage.h"


/*
 * copy(src, dst, count)
 *	memaddr	src, dst;
 *	int	count;
 *
 * Copy count clicks from src to dst.  Uses KDSA5 and 6 to copy with mov
 * instructions.  Interrupt routines must restore segmentation registers
 * if needed; see seg.h.
 */
ENTRY(copy)
	jsr	r5, csv
#ifdef INET
	mov	PS,-(sp)		/ have to lock out interrupts...
	bit	$0340,(sp)		/ Are we currently at spl0?
	bne	1f
	SPLNET				/ No, lock out network interrupts.
1:
#endif
	mov	KDSA5,-(sp)		/ saveseg5(sp)
	mov	KDSD5,-(sp)
	mov	10(r5),r3		/ r3 = count
	beq	3f			/ (exit early if zero)
	mov	4(r5),KDSA5		/ seg5 = (src, 1 click read only)
	mov	$RO,KDSD5
	mov	sp,r4			/ save current sp
	mov	$eintstk,sp		/   and switch to intstk
	mov	KDSA6,_kdsa6		/ save u area address, mark as mapped
	mov	6(r5),KDSA6		/   out and seg6 = (dst, 1 click read/
	mov	$RW,KDSD6		/   write)
1:
	mov	$5*8192.,r0		/ r0 = SEG5 { src }
	mov	$6*8192.,r1		/ r1 = SEG6 { dst }
	mov	$8.,r2			/ copy one click (8*8)
2:
	mov	(r0)+,(r1)+
	mov	(r0)+,(r1)+
	mov	(r0)+,(r1)+
	mov	(r0)+,(r1)+
	sob	r2,2b

	inc	KDSA5			/ next click
	inc	KDSA6
	sob	r3,1b

	mov	_kdsa6,KDSA6		/ restore u area mapping
	mov	$USIZE-1\<8|RW, KDSD6
	clr	_kdsa6
	mov	r4,sp			/ back to normal stack
3:
	mov	(sp)+,KDSD5		/ restorseg5(sp)
	mov	(sp)+,KDSA5
#ifdef INET
	mov	(sp)+,PS		/ back to normal priority
#endif
	jmp	cret

/*
 * Clear count clicks at dst.  Uses KDSA5.  Interrupt routines must restore
 * segmentation registers if needed; see seg.h.
 *
 * clear(dst, count)
 * 	memaddr dst;
 * 	u_int count;
 */
ENTRY(clear)
	jsr	r5, csv
	mov	KDSA5,-(sp)		/ saveseg5(sp)
	mov	KDSD5,-(sp)
	mov	4(r5),KDSA5		/ point KDSA5 at source
	mov	$RW,KDSD5		/ 64 bytes, read-write
	mov	6(r5),r3		/ count
	beq	3f
1:
	mov	$5*8192.,r0		/ point r0 at KDSA5 map
	mov	$8.,r2			/ clear one click (8*8)
2:
	clr	(r0)+
	clr	(r0)+
	clr	(r0)+
	clr	(r0)+
	sob	r2,2b

	inc	KDSA5			/ next click
	sob	r3,1b
3:
	mov	(sp)+,KDSD5		/ restore seg5
	mov	(sp)+,KDSA5		/ restore seg5
	jmp	cret

/*
 * copyv(fromaddr, toaddr, count)
 *	virtual_addr	fromaddr,
 *			toaddr;
 *	u_int		count;
 *
 * typedef struct {
 *	memaddr	click;
 *	u_int	offset;
 * } virtual_addr;
 *
 * Copy two arbitrary pieces of PDP11 virtual memory from one location
 * to another.  Up to 8K bytes can be copied at one time.
 *
 * A PDP11 virtual address is a two word value; a 16 bit "click" that
 * defines the start in physical memory of an 8KB segment and an offset.
 */
ENTRY(copyv)
	mov	10.(sp),r0		/ exit early if count == 0 or >= 8K
	beq	4f
	cmp	r0,$8192.
	bhis	4f
	mov	r2,-(sp)		/ need a register for the stack switch
	/*
	 * Note: the switched stack is only for use of a fatal kernel trap
	 * occurring during the copy; otherwise we might conflict with the
	 * other copy routines.
	 */
	mov	sp,r2			/ switch stacks
	cmp	sp,$eintstk		/ are we already in the intstk? 
	blo	1f
	mov	$eintstk,sp		/ No, point sp to intstk
1:
	mov	PS,-(sp)		/ save current PS
	bit	$0340,(sp)		/ are we currently at SPL?
	bne	2f
	SPLNET				/ nope, lock out the network
2:
	mov	KDSA5,-(sp)		/ save seg5
	mov	KDSD5,-(sp)
	mov	_kdsa6,-(sp)		/ save the current saved kdsa6
	bne	3f			/ was it set to anything?
	mov	KDSA6,_kdsa6		/ nope, set it to the current kdsa6 
3:
	mov	KDSA6,-(sp)		/ save user area
	mov	KDSD6,-(sp)
	mov	r2,r0			/ set up bcopy arguments on new stack
	add	$14.,r0			/   and grab click addresses ...
	mov	-(r0),-(sp)		/ copy count
	mov	-(r0),-(sp)		/ copy and translate toaddr.offset
	add	$6*8192.,(sp)
	mov	-(r0),r1		/ pick up and save toaddr.click
	mov	-(r0),-(sp)		/ copy and translate fromaddr.offset
	add	$5*8192.,(sp)

	mov	-(r0),KDSA5		/ use fromaddr.click to remap seg5
	mov	$128.-1\<8.|RO,KDSD5	/   for 8K read-only
	mov	r1,KDSA6		/ use toaddr.click to remap seg6
	mov	$128.-1\<8.|RW,KDSD6	/   for 8K read/write
	jsr	pc,_bcopy		/ finally, do the copy
	add	$6.,sp			/ toss the arguments to bcopy
	mov	(sp)+,KDSD6		/ restore user area
	mov	(sp)+,KDSA6
	mov	(sp)+,_kdsa6		/ restore old _kdsa6 value
	mov	(sp)+,KDSD5		/ restore seg5
	mov	(sp)+,KDSA5
	mov	(sp)+,PS		/ unlock interrupts
	mov	r2,sp			/ restore stack pointer and retrieve
	mov	(sp)+,r2		/   old register value
4:
	clr	r0			/ clear r0 and r1 (why?)
	rts	pc			/   and return

/*
 * fmove(par, pdr, from, to, length)
 *	u_short	par, pdr;
 *	caddr_t	from, to;
 *	u_short	length;
 *
 * Map the par/pdr into segment 6 and perform a bcopy(from, to, length).
 * Returns zero on success, EFAULT on failure.  See uiofmove for more
 * information.
 *
 * As written, fmove may only be called from the high kernel and is *not*
 * re-entrant: it doesn't save the previous fault trap, uses the intstk and
 * kdsa6 without checking to see if they're already in use (see the fault
 * handler at 2f below) and assumes that a user structure is currently mapped
 * in (see the restore of KDSD6 below).
 */
ENTRY(fmove)
	mov	r2,-(sp)		/ need a few registers for the
	mov	r3,-(sp)		/  copy and stack switch
	mov	r4,-(sp)
	mov	sp,r4			/ switch stacks (SEG6 is out)
	mov	$eintstk,sp
	mov	r4,r0			/ grab parameters
	add	$8.,r0
	mov	(r0)+,-(sp)		/ push par and pdr onto new stack
	mov	(r0)+,-(sp)
	mov	(r0)+,r1		/ r1 = from
	mov	(r0)+,r2		/ r2 = to
	mov	(r0),r0			/ r0 = length
	mov	KDSA6,_kdsa6		/ save current segment 6 mapping
	mov	(sp)+,KDSD6		/   passed par/pdr
	mov	(sp)+,KDSA6		/   and map segment 6 with the
	mov	$8f,nofault		/ catch any faults during the copy,

	/*
	 * The following code has been lifted almost intact from "bcopy".
	 * The biggest reason is for error recovery in the case of a user
	 * memory fault.
	 */
	tst	r0		/ if (length == 0)
	beq	7f		/	return
	cmp	r0,$10.		/ if (length > 10)
	bhi	2f		/	try words
1:
	movb	(r1)+,(r2)+	/ do  *dst++ = *src++
	sob	r0,1b		/ while (--length)
	br	7f
2:
	bit	$1,r1		/ if (src&1 != dst&1)
	beq	3f		/	do bytes
	bit	$1,r2
	beq	1b		/ (src odd, dst even - do bytes)
	movb	(r1)+,(r2)+	/ copy leading odd byte
	dec	r0
	br	4f
3:
	bit	$1,r2
	bne	1b		/ (src even, dst odd - do bytes)
4:
	mov	r0,r3		/ save trailing byte indicator
	clc
	ror	r0		/ length >>= 1 (unsigned)
	asr	r0		/ if (length >>= 1, wasodd(length))
	bcc	5f		/	handle leading non multiple of four
	mov	(r1)+,(r2)+
5:
	asr	r0		/ if (length >>= 1, wasodd(length))
	bcc	6f		/	handle leading non multiple of eight
	mov	(r1)+,(r2)+
	mov	(r1)+,(r2)+
6:
	mov	(r1)+,(r2)+	/ do
	mov	(r1)+,(r2)+	/	move eight bytes
	mov	(r1)+,(r2)+
	mov	(r1)+,(r2)+
	sob	r0,6b		/ while (--length)
	asr	r3		/ if (odd trailing byte)
	bcc	7f
	movb	(r1)+,(r2)+	/	copy it
	/*
	 * End of stolen bcopy code.
	 */

7:
	clr	nofault			/ clear the fault trap and return
					/   status (zero from successful copy)
	mov	_kdsa6,KDSA6		/ restore the previous segment 6
	mov	$USIZE-1\<8|RW, KDSD6	/   mapping (user structure)
	clr	_kdsa6			/   (segment 6 not mapped out any more)
	mov	r4,sp			/ restore previous stack pointer
	mov	(sp)+,r4		/   and saved registers
	mov	(sp)+,r3
	mov	(sp)+,r2
	rts	pc			/ and return
8:
	mov	$EFAULT,r0		/ copy failed, return EFAULT
	br	7b

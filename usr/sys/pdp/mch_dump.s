/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_dump.s	1.3 (2.11BSD GTE) 12/24/92
 */
#include "DEFS.h"
#include "../machine/mch_iopage.h"

/*
 * Save regs r0, r1, r2, r3, r4, r5, r6, K[DI]SA6
 * starting at data location 0300, in preparation for dumping core.
 */
ENTRY(saveregs)
#ifdef KERN_NONSEP
	movb	$RW, KISD0		/ write enable
#endif KERN_NONSEP
	mov	r0,300
	mov	$302,r0
	mov	r1,(r0)+
	mov	r2,(r0)+
	mov	r3,(r0)+
	mov	r4,(r0)+
	mov	r5,(r0)+
	mov	sp,(r0)+
	mov	KDSA6,(r0)+
	mov	KDSA5,(r0)+
	rts	pc

#ifdef	INET

SPACE(GLOBAL, suprsav, 32)

/*
 * Save ALL registers, KDSA[5,6], SDSA[5,6], SSR3.  Expressly for
 * network crashes where the state at the time of initial trap is
 * desired rather than after thrashing about on the way to a 'panic'.
 * Also, this is extensible so that as much volatile information as
 * required may be saved.  Currently 14 of the 16 words allocated are used.
 * Multiple entries into this routine should be blocked by making the
 * call to this routine conditional on 'netoff' being set and 
 * setting 'netoff' on the first call.  Must be at splhigh upon entry.
*/

ENTRY(savestate)
	mov	r0,suprsav
	mov	$suprsav+2,r0
	mov	r1,(r0)+
	mov	r2,(r0)+
	mov	r3,(r0)+
	mov	r4,(r0)+
	mov	r5,(r0)+
	mov	sp,(r0)+
	mov	PS,-(sp)
	mov	$010340,PS	/previous super, spl7
	mfpd	sp		/fetch supervisor stack pointer
	mov	(sp)+,(r0)+
	mov	$030340,PS	/previous user, spl7
	mfpd	sp		/fetch user stack pointer
	mov	(sp)+,(r0)+
	mov	(sp)+,PS
	mov	KDSA5,(r0)+
	mov	KDSA6,(r0)+
	mov	SDSA5,(r0)+
	mov	SDSA6,(r0)+
	mov	SSR3,(r0)+
	rts	pc
#endif

#include "ht.h"
#include "tm.h"
#include "ts.h"

/*
 * Mag tape dump -- save registers in low core and write core (up to 248K)
 * onto mag tape.  Entry is through 044 (physical) with memory management off.
 */

#ifndef KERN_NONSEP
.data
#endif !KERN_NONSEP

ASENTRY(dump)
#if NHT > 0 || NTM > 0 || NTS > 0
	/*
	 * save regs r0, r1, r2, r3, r4, r5, r6, KIA6
	 * starting at location 4 (physical)
	 */
	inc	$-1			/ check for first time
	bne	1f			/ if not, don't save registers again
	mov	r0,300
	mov	$302,r0
	mov	r1,(r0)+
	mov	r2,(r0)+
	mov	r3,(r0)+
	mov	r4,(r0)+
	mov	r5,(r0)+
	mov	sp,(r0)+
	mov	KDSA6,(r0)+
1:

	/*
	 * dump all of core (i.e. to first mt error)
	 * onto mag tape. (9 track or 7 track 'binary')
	 */

#if NHT > 0

HT	= 0172440
HTCS1	= HT+0
HTWC	= HT+2
HTBA	= HT+4
HTFC	= HT+6
HTCS2	= HT+10
HTTC	= HT+32

	mov	$HTCS1,r0
	mov	$40,*$HTCS2
	mov	$2300,*$HTTC
	clr	*$HTBA
	mov	$1,(r0)
1:
	mov	$-512.,*$HTFC
	mov	$-256.,*$HTWC
	movb	$61,(r0)
2:
	tstb	(r0)
	bge	2b
	bit	$1,(r0)
	bne	2b
	bit	$40000,(r0)
	beq	1b
	mov	$27,(r0)
#else !NHT > 0

#if NTM > 0

MTC	= 172522
IO	= 0177600
UBMR0	= 0170200

	/*
	 * register usage is as follows:
	 *
	 * reg 0 -- holds the tm11 CSR address 
	 * reg 1 -- points to UBMAP register 0 low 
	 * reg 2 -- is used to contain and calculate memory pointer 
	 *	    for UBMAP register 0 low 
	 * reg 3 -- is used to contain and calculate memory pointer 
	 *	    for UBMAP register 0 high
	 * reg 4 -- r4 = 1 for map used, r4 = 0 for map not used 
	 * reg 5 -- is used as an interation counter when mapping is enabled 
	 */

	movb	_ubmap,r4		/ UB map used indicator
	beq	2f			/ no UBMAP - br
1:
	/*
	 * This section of code initializes the Unibus map registers and
	 * and the memory management registers.
	 * UBMAP reg 0 gets updated to point to the current memory area.
	 * Kernal I space 0 points to low memory
	 * Kernal I space 7 points to the I/O page.
	 */

	mov	$UBMR0,r1		/ point to  map register 0
	clr	r2			/ init for low map reg
	clr	r3			/ init for high map reg
	mov	$77406,*$KISD0		/ set KISDR0
	mov	$77406,*$KISD7		/ set KISDR7
	clr	*$KISA0			/ point KISAR0 to low memory
	mov	$IO,*$KISA7		/ point KISAR7 to IO page
	inc	*$SSR0			/ turn on memory mngt
	mov	$60,*$SSR3		/ enable 22 bit mapping
	mov	r2,(r1)			/ load map reg 1 low
	mov	r3,2(r1)		/ load map reg 1 high
2:
	/ this section of code initializes the TM11

	mov	$MTC,r0
	mov	$60004,(r0)
	clr	4(r0)
	mov	$20,r5			/ set up SOB counter for UBMAP

	/*
	 * This section does the write; if mapping is needed the sob loop
	 * comes in play here.  When the sob falls through the UBMAP reg
	 * will be updated by 20000 to point to next loop section.
	 * If mapping not needed then just let bus address register increment.
	 */
3:
	mov	$-512.,2(r0)		/ set byte count
	inc	(r0)			/ start xfer
1:
	tstb	(r0)			/ wait for tm11 ready
	bge	1b */
	tst	(r0)			/ any error
	bge	2f			/ no, continue xfer
	bit	$200,-2(r0)		/ yes, must be NXM error
	beq	.			/ hang if not
	reset				/ error was NXM
	mov	$60007,(r0)		/ write EOF
	halt				/ halt on good dump
2:
	tst	r4			/ mapping
	beq	3b			/ branch if not
	sob	r5,3b			/ yes, continue loop
	mov	$20,r5			/ reset loop count
	add	$20000,r2		/ bump low map
	adc	r3			/ carry to high map
	mov	r2,(r1)			/ load map reg 0 low
	mov	r3,2(r1)		/ load map reg 0 high
	clr	4(r0)			/ set bus address to 0
	br	3b			/ do some more

#else !NTM > 0

#if NTS > 0

TSDB	= 0172520
TSSR	= 0172522
IO	= 0177600
UBMR0	= 0170200

	/*
	 * register usage is as follows:
	 *
	 * reg 0 -- points to UBMAP register 1 low
	 * reg 1 -- is used to calculate the current memory address
	 *	    for each 512 byte transfer.
	 * reg 2 -- is used to contain and calculate memory pointer
	 *	    for UBMAP register 1 low
	 * reg 3 -- is used to contain and calculate memory pointer
	 *	    for UBMAP register 1 high
	 * reg 4 -- points to the command packet
	 * reg 5 -- is used as an interation counter when mapping is enabled
	 */

	movb	_ubmap,setmap		/ unibus map present?
	beq	2f			/ no, skip map init

	/*
	 * This section of code initializes the Unibus map registers and
	 * and the memory management registers.
	 * UBMAP reg 0 points to low memory for the TS11 command,
	 * characteristics, and message buffers.
	 * UBMAP reg 1 gets updated to point to the current memory area.
	 * Kernal I space 0 points to low memory
	 * Kernal I space 7 points to the I/O page.
	 */

	mov	$UBMR0,r0		/ point to  map register 0
	clr	r2			/ init for low map reg
	clr	r3			/ init for high map reg
	clr	(r0)+			/ load map reg 0 low
	clr	(r0)+			/ load map reg 0 high
	mov	$77406,*$KISD0		/ set KISDR0
	mov	$77406,*$KISD7		/ set KISDR7
	clr	*$KISA0			/ point KISAR0 to low memory
	mov	$IO,*$KISA7		/ point KISAR7 to IO page
	inc	*$SSR0			/ turn on memory mngt
	mov	$60,*$SSR3		/ enable 22 bit mapping
	mov	r2,(r0)			/ load map reg 1 low
	mov	r3,2(r0)		/ load map reg 1 high
2:
	/ this section of code initializes the TS11

	tstb	*$TSSR			/ make sure
	bpl	2b			/ drive is ready
	mov	$comts,r4		/ point to command packet
	add	$2,r4			/ set up mod 4
	bic	$3,r4			/ alignment
	mov	$140004,(r4)		/ write characteristics command
	mov	$chrts,2(r4)		/ characteristics buffer
	clr	4(r4)			/ clear ext mem addr (packet)
	clr	tsxma			/ clear extended memory save loc
	mov	$10,6(r4)		/ set byte count for command
	mov	$mests,*$chrts		/ show where message buffer is
	clr	*$chrts+2		/ clear extended memory bits here too
	mov	$16,*$chrts+4		/ set message buffer length
	mov	r4,*$TSDB		/ start command
	mov	$20,r5			/ set up SOB counter for UBMAP
	clr	r1			/ init r1 beginning memory address
1:
	tstb	*$TSSR			/ wait for ready
	bpl	1b			/ not yet
	mov	*$TSSR,tstcc		/ error condition (SC)?
	bpl	2f			/ no error
	bic	$!16,tstcc		/ yes error, get TCC
	cmp	tstcc,$10		/ recoverable error?
	bne	8f			/ no
	mov	$101005,(r4)		/ yes, load write data retry command
	clr	4(r4)			/ clear packet ext mem addr
	mov	r4,*$TSDB		/ start retry
	br	1b
8:
	bit	$4000,*$TSSR		/ is error NXM?
	beq	.			/ no, hang (not sure of good dump)
	mov	$140013,(r4)		/ load a TS init command
	mov	r4,*$TSDB		/ to clear NXM error
6:
	tstb	*$TSSR			/ wait for ready
	bpl	6b
	mov	$1,6(r4)		/ set word count = 1
	mov	$100011,(r4)		/ load write EOF command
	mov	r4,*$TSDB		/ do write EOF
7:
	tstb	*$TSSR			/ wait for ready
	bpl	7b
	halt				/ halt after good dump
9:
	br	1b
2:
	/*
	 * If mapping is needed this section calculates the base address to
	 * be loaded into map reg 1; the algorithm is (!(r5 - 21))*1000) |
	 * 20000.  The complement is required because an SOB loop is being
	 * used for the counter.  This loop causes 20000 bytes to be written
	 * before the UBMAP is updated.
	 */

	tst	setmap			/ UBMAP?
	beq	3f			/ no map
	mov	r2,(r0)			/ load map reg 1 low
	mov	r3,2(r0)		/ load map reg 1 high
	mov	r5,r1			/ calculate
	sub	$21,r1			/ address for this pass
	com	r1			/ based on current
	mul	$1000,r1		/ interation
	bis	$20000,r1		/ select map register 1
	clr	4(r4)			/ clear extended memory bits
3:
	/*
	 * This section does the write.  If mapping is needed the sob loop
	 * comes in play here.  When the sob falls through the UBAMP reg will
	 * be updated by 20000 to point to next loop section.  If mapping not
	 * needed then just calculate the next 512 byte address pointer.
	 */

	mov	r1,2(r4)		/ load mem address
	mov	tsxma,4(r4)		/ load ext mem address
	mov	$512.,6(r4)		/ set byte count
	mov	$100005,(r4)		/ set write command
	mov	r4,*$TSDB		/ initiate xfer
	tst	setmap			/ mapping?
	beq	4f			/ branch if not
	sob	r5,9b			/ yes continue loop
	mov	$20,r5			/ reset loop count
	add	$20000,r2		/ bump low map
	adc	r3			/ carry to high map
	br	1b			/ do some more
4:
	add	$512.,r1		/ bump address for no mapping
	adc	tsxma			/ carry to extended memory bits
	br	1b			/ do again

/*
 * The following TS11 command and message buffers, must be in initialized data
 * space instead of bss space.  This allows them to be mapped by the first
 * M/M mapping register, which is the only one used durring a core dump.
 */

tsxma:	0				/ ts11 extended memory address bits
setmap:	0				/ UB map usage indicator
tstcc:	0				/ ts11 temp location for TCC
comts:					/ ts11 command packet
	0 ; 0 ; 0 ; 0 ; 0
chrts:					/ ts11 characteristics
	0 ; 0 ; 0 ; 0
mests:					/ ts11 message buffer
	0 ; 0 ; 0 ; 0 ; 0 ; 0 ; 0

#endif NTS
#endif NTM
#endif NHT
#endif NHT || NTM || NTS

br	.				/ If no tape drive, fall through to here

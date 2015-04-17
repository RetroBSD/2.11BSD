/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)mch_start.s	1.4 (2.11BSD GTE) 8/23/93
 */

#include "DEFS.h"
#include "../machine/mch_iopage.h"
#include "../machine/mch_cpu.h"
#include "../machine/trap.h"

ASENTRY(start)
	bit	$1,SSR0			/ is memory management enabled?
	beq	.			/ better be !!!

	mov	r0,_cputype		/ save cpu type passed by boot
	/*
	 * The following two instructions change the contents of the "sys"
	 * instruction vector (034-037) to read:
	 *	syscall; br0+T_SYSCALL
	 */
	mov	$syscall,34
	mov	$0+T_SYSCALL,36

#ifdef KERN_NONSEP
	movb	$RO,KISD0		/ Turn off write permission on kernel
					/   low text
#endif

	mov	$USIZE-1\<8|RW,KDSD6	/ Get a stack pointer (_u + 64*USIZE)
	mov	$_u+[USIZE*64.],sp

#ifdef INET
	/*
	 * Initial set up for SUPERVISOR space networking: set SUPERVISOR
	 * space as split I&D, set stack pointer and map user area and I/O
	 * page.
	 */
	bis	$2,SSR3			/ split i/d for supervisor network
	mov	PS,-(sp)		/ set SUPERVISOR sp to NET_STOP
	mov	$010340,PS
	mov	$NET_STOP,-(sp)
	mtpd	sp
	mov	(sp)+,PS
	mov	KDSA6,SDSA6		/ map user area and I/O page
	mov	KDSD6,SDSD6
	mov	KDSD7,SDSD7
	mov	KDSA7,SDSA7
#endif

	mov	$_u,r0			/ Clear user block
1:
	clr	(r0)+
	cmp	r0,sp
	blo	1b

	/*
	 * Get bootflags and leave them in _initflags; the bootstrap leaves
 	 * them in r4.  R2 should be the complement of bootflags.
	 */
	com	r2			/ if r4 != ~r2
	cmp	r4,r2
	beq	1f
	mov	$RB_SINGLE,r4		/   r4 = RB_SINGLE
1:
	mov	r1,_bootcsr		/ save boot controller csr
	mov	r3,_bootdev		/ save boot device major,unit
	mov	r4,_boothowto		/ save boot flags
	mov	$_initflags+6,r2	/ get a pointer to the \0 in _initflags
	mov	r4,r1			/ r1 = boot options
1:
	clr	r0			/ r0:r1 = (long)r1
	div	$10.,r0			/ r0 = r0:r1 / 10; r1 = r0:r1 % 10
	add	$'0,r1			/ bias by ASCII '0'
	movb	r1,-(r2)		/   and stuff into _initflags
	mov	r0,r1			/ shift quotient and continue
	bne	1b			/   if non-zero

	/* Check out (and if necessary, set up) the hardware. */
	jsr	pc,hardprobe

	/*
	 * Set up previous mode and call main.  On return, set user stack
	 * pointer and enter user mode at location zero to execute the
	 * init thunk _icode copied out in main.
	 */
	mov	$30340,PS		/ set current kernel mode, previous
					/   mode to user, spl7, make adb
	clr	r5			/   find end of kernel stack frames
	jsr	pc,_main		/ call main
	mov	$177776,-(sp)		/ set init thunk stack pointer to top
	mtpd	sp			/   of memory ...
	mov	$170000,-(sp)		/ set up pseudo trap frame: user/user
	clr	-(sp)			/   mode, location zero,
	rtt				/   and book ...

	.data
/*
 * Icode is copied out to process 1 to exec /etc/init.
 * If the exec fails, process 1 exits.
 */
.globl	_initflags, _szicode, _boothowto, _bootcsr, _bootdev

ENTRY(icode)
	mov	$argv-_icode,-(sp)
	mov	$init-_icode,-(sp)
	tst	-(sp)			/ simulate return address stack spacing
	sys	SYS_execv
	sys	SYS_exit
init:
	</etc/init\0>
_initflags:
	<-00000\0>			/ decimal ASCII initflags
	.even
argv:
	init+5-_icode			/ address of "init\0"
	_initflags-_icode
	0
_szicode:
	_szicode-_icode
_boothowto:
	0				/ boot flags passed by boot
_bootdev:
	0				/ boot major#,unit
_bootcsr:
	0				/ csr of booting controller
	.text

/*
 * Determine a couple of facts about the hardware and finishing setting
 * up what 'boot' hasn't done already.

 * We use the cpu type passed thru from /boot.  No sense in duplicating 
 * that code here in the kernel.  We do have to repeat the KDJ-11 test
 * (for use in trap.c) though.  /boot also stuffed the right bits into 
 * the MSCR register to disable cache and unibus traps.
 */
hardprobe:
	mov	$1f,nofault
	setd
	inc	_fpp
1:

	/*
	 * Test for SSR3 and UNIBUS map capability.  If there is no SSR3, the
	 * first test of SSR3 will trap and we skip past the separate I/D test.
	 * 2.11BSD will be _seriously_ upset if I/D is not available!
	 */
	mov	$2f,nofault
	bit	$40,SSR3
	beq	1f
	incb	_ubmap
1:
	bit	$1,SSR3			/ Test for separate I/D capability
	beq	2f
	incb	_sep_id
2:
		/ Test for stack limit register; set it if present.
	mov	$1f,nofault
	mov	$intstk-256.,STACKLIM
1:
	clr	_kdj11
	mov	$1f,nofault
	mfpt
	cmp	r0,$5			/ KDJ-11 returns 5 (11/44 returns 1)
	bne	1f
	mov	r0,_kdj11
1:
#ifdef ENABLE34
	/*
	 * Test for an ENABLE/34.  We are very cautious since the ENABLE's
	 * PARs are in the range of the floating addresses.
	 */
	tstb	_ubmap
	bne	2f
	mov	$2f,nofault
	mov	$32.,r0
	mov	$ENABLE_KISA0,r1
1:
	tst	(r1)+
	sob	r0,1b

	tst	*$PDP1170_LEAR
	tst	*$ENABLE_SSR3
	tst	*$ENABLE_SSR4
	incb	_enable34
	incb	_ubmap

	/*
	 * Turn on an ENABLE/34.  Enableon() is a C routine which does a PAR
	 * shuffle and turns mapping on.
	 */

.data
_UISA:	DEC_UISA
_UDSA:	DEC_UDSA
_KISA0:	DEC_KISA0
_KISA6:	DEC_KISA6
_KDSA1:	DEC_KDSA1
_KDSA2:	DEC_KDSA2
_KDSA5:	DEC_KDSA5
_KDSA6:	DEC_KDSA6

.text
	mov	$ENABLE_UISA, _UISA
	mov	$ENABLE_UDSA, _UDSA
	mov	$ENABLE_KISA0, _KISA0
	mov	$ENABLE_KISA6, _KISA6
	mov	$ENABLE_KDSA1, _KDSA1
	mov	$ENABLE_KDSA2, _KDSA2
	mov	$ENABLE_KDSA5, _KDSA5
	mov	$ENABLE_KDSA6, _KDSA6
	mov	$ENABLE_KDSA6, _ka6
	jsr	pc, _enableon

2:
#endif ENABLE34

	clr	nofault			/ clear fault trap and return
	rts	pc

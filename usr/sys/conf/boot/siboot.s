/*
 *	SCCS id	@(#)siboot.s	2.0 (2.11BSD)	4/13/91
 */
#include "localopts.h"

/  The boot options and device are placed in the last SZFLAGS bytes
/  at the end of core for the bootstrap.
ENDCORE=	160000		/ end of core, mem. management off
SZFLAGS=	6		/ size of boot flags
BOOTOPTS=	2		/ location of options, bytes below ENDCORE
BOOTDEV=	4		/ boot unit
CHECKWORD=	6

.globl	_doboot, hardboot, _bootcsr
.text
_doboot:
	mov	4(sp),r4	/ boot options
	mov	2(sp),r3	/ boot device

#ifndef	KERN_NONSEP
/  If running separate I/D, need to turn off memory management.
/  Call the routine unmap in low text, after setting up a jump
/  in low data where the PC will be pointing.
.globl	unmap
	mov	$137,*$unmap+2		/ jmp *$hardboot
	mov	$hardboot,*$unmap+4
	jmp	unmap
	/ "return" from unmap will be to hardboot in data
.data
#else
/  Reset to turn off memory management
	reset
#endif

/  On power fail, hardboot is the entry point (map is already off)
/  and the args are in r4 (RB_POWRFAIL), r3 (rootdev)

hardboot:
	mov	r4, ENDCORE-BOOTOPTS
	ash	$-3,r3		/ shift out the partition number
	bic	$!7,r3		/ save only the drive number
	mov	r3, ENDCORE-BOOTDEV
	com	r4		/ if CHECKWORD == ~bootopts, flags are believed
	mov	r4, ENDCORE-CHECKWORD
1:
	reset

/  The remainder of the code is dependent on the boot device.
/  If you have a bootstrap ROM, just jump to the correct entry.
/  Otherwise, use a BOOT opcode, if available;
/  if necessary, read in block 0 to location 0 "by hand".

/ si 9500 bootstrap - bmarsh@nosc

WC	= 256.
READ	= 4
GO	= 1

sicnr	= 0
siwcr	= sicnr+2
sipcr	= sicnr+4
sihsr	= sicnr+6
simar	= sicnr+10
sierr	= sicnr+12
siscr	= sicnr+24

	mov	_bootcsr,r1
	bit	$200,siscr(r1)	/ see if grant set
	bne	1f		/ if set, is dual ported controller
	mov	sierr(r1),r0	/ load error register
	bic	$037777,r0	/ clear all but contention and error bits
	cmp	$140000,r0	/ see if we have a contention error
	bne	2f		/ if not, controller is not dual ported
1:
	bit	$200,siscr(r1)	/ test for grant
	bne	2f		/ if set, ok for read
	clr	sicnr(r1)	/ send logic master clear
	mov	$1,siscr(r1)	/ request grant
	br	1b		/ loop until grant
2:
	mov	ENDCORE-BOOTDEV,r0
	ash	$10.,r0
	mov	r0,sipcr(r1)	/ port 0 cylinder 0, unit
	clr	sihsr(r1)	/ head 0 sector 0
	clr	simar(r1)	/ address 0
	mov	$WC,siwcr(r1)	/ word count
	mov	$READ+GO,sicnr(r1)
1:
	tstb	sicnr(r1)
	bge	1b
	mov	ENDCORE-BOOTDEV,r0
	clr	pc

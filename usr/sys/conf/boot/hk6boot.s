/*
 *	SCCS id	@(#)hk6boot.s	2.0 (2.11BSD)	4/13/91
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

/ rk06 disk driver

WC = -256.

hkcs1 = 0	/ offsets from base csr, control & status 1
hkda  = 6	/ desired track/sector address
hkcs2 = 10	/ control & status 2
hkca  = 20	/ desired cylinder

/ RK06 constants.
ack = 03	/ pack acknowledge
clear = 040	/ subsystem clear
iocom = 021	/ read + go

/ initialize hk
	mov	_bootcsr,r1
	mov	$clear,hkcs2(r1)
	mov	$ack,hkcs1(r1)
0:
	tstb	hkcs1(r1)
	bpl	0b		/ wait for acknowledge to complete

	clr	hkca(r1)
	add	$hkcs2,r1
	mov	ENDCORE-BOOTDEV,(r1)
	clr	-(r1)		/ sector and track (hkda)
	clr	-(r1)		/ bus address
	mov	$WC,-(r1)	/ word count
	mov	$iocom,-(r1)
1:
	tstb	(r1)
	bge	1b		/ wait for iocom to complete
	mov	ENDCORE-BOOTDEV,r0
	clr	pc

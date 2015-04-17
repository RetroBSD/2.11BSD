/*
 *	SCCS id	@(#)dvhpboot.s	2.0 (2.11BSD)	4/13/91
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

/
/ Bootstrap for DIVA Comp/V controller without boot opcode
/
HPCSR=	0		/ offset from base csr
HPCS2=	10		/ Control/status register 2
HPDC=	34		/ Desired cylinder
HPBAE=	50		/ Bus extension address (RH70)
READIT=	71

	mov	_bootcsr,r1	/ boot device csr
1:
	tstb	(r1)		/ wait for ready (HPCSR is offset 0)
	bpl	1b

	clr	HPDC(r1)	/ Cylinder 0
	clr	HPBAE		/ Bus extension address = 0
	add	$HPCS2,r1
	mov	ENDCORE-BOOTDEV,(r1)	/ unit number
	clr	-(r1)		/ hpda = 0 (desired address 0)
	clr	-(r1)		/ hpba = 0 (buf address 0)
	mov	$-256.,-(r1)	/ hpwc = -256 (one block)
	mov	$READIT,-(r1)	/ hpcs1 = HP_RCOM|HP_GO

1:
	tstb	(r1)		/ wait for done
	bpl	1b
	mov	ENDCORE-BOOTDEV,r0
	clr	pc

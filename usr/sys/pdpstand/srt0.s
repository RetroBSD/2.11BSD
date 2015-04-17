/ 2000/04/06 - make 'nofault' global so toyset.o can use it
/ 1995/06/04 - devsw[] entries are 14. bytes, need a tape seek routine entry.
/ 1995/06/02 - Modifications for split I/D to work.  The vectors need to be
/	       in 'data' space.
/ 1995/06/01 - Make copy of SSR3 so we can tell if split I/D is enabled.
/ 1995/05/30 - devsw[] entries are 12. bytes now.
/
/ Startup code for standalone utilities
/ sms - mods to pass boot device and csr on to program
/
/      Note that the bootstrap passes the cputype through in r0.

PS	= 177776
SSR3	= 172516

.globl	_main, __rtt, _devsw, _ADJcsr

#ifdef	SPLIT_ID
	.data
#else
	.text
#endif
ZERO:
	jmp	start

/
/ trap vectors
/
	trap;340	/ 004 - bus error
	trap;341	/ 010 - illegal instruction
	trap;342	/ 014 - BPT
	trap;343	/ 020 - IOT
	trap;344	/ 024 - POWER FAIL
	trap;345	/ 030 - EMT
	start;346	/ 034 - TRAP
.= ZERO + 400
	.text

start:
	mov	$340,*$PS
	mov	$trap,*$034

/ Save information which Boot has passed thru to us

	mov	r0,_cputype	/ assume that the boot left this in r0
	mov	r1,_bootcsr	/ assume that boot left csr in r1
	mov	r3,_bootdev	/ and boot device major,minor in r3

/ Make a copy of SSR3.  If this register does not exist it is probably better
/ to trap now rather than later - the absence of this register means no  split
/ I/D space and the kernel won't run.

	mov	*$SSR3,_ssr3copy

	mov	$157772,sp	/ return address,psw at 157774&6

/ controller number is in bits 6&7 of r3 (_bootdev).  major device number
/ is in bits 8-15.  what we need to do now is place the csr into
/ the appropriate driver's csrlist which is indexed by controller number.

	ash	$-5,r3			/ r3 = controller# * 2
	mov	r3,r0			/ controller number in bits 1,2
	bic	$!6,r0			/ make a word index
	ash	$-3,r3			/ major device # to bits 0-7
	mov	r3,r2			/ save major for later
	mul	$14.,r3			/ devsw[] members are 14. bytes each
	mov	_devsw+10(r3),r3	/ get csrlist for driver
	add	r0,r3			/ point to controller's entry
	asr	r0			/ controller number in bits 0,1
	mov	r0,_bootctlr		/ set default controller number

/ the CSR passed from the ROMs is not necessarily the first address
/ of a device!  We therefore have to adjust the CSR so that the structure
/ pointers in the drivers are origined at the base address rather than
/ the ROM supplied address.  The adjustment was not stored in devsw[] to
/ save space (can get by with an array of bytes instead of words).

	movb	_ADJcsr(r2),r2		/ adjust (possibly) the CSR
	mov	_bootcsr,r1
	sub	r2,r1

	mov	r1,(r3)			/ store controller's csr in table
	jsr	pc,_main

/ fix up stack to point at trap ps-pc pair located at top of memory
/ so we can return to the bootstrap.
/
/ the return protocol originally changed from 140000 because icheck was
/ too large.  icheck was reduced in size but the 157774 return protocol
/ was retained to allow for slightly oversized programs.

__rtt:
	mov	$157774,sp
	rtt				/ we hope!
	br	.


.globl	_trap
trap:
	mov	*$PS,-(sp)
	tst	nofault
	bne	3f
	mov	r0,-(sp)
	mov	r1,-(sp)
2:	jsr	pc,_trap
	mov	(sp)+,r1
	mov	(sp)+,r0
	tst	(sp)+
	rtt
3:	tst	(sp)+
	mov	nofault,(sp)
	rtt

	.data
.globl	nofault, _cputype, _bootcsr, _bootdev, _ssr3copy, _bootctlr

nofault:	.=.+2	/ where to go on predicted trap
_cputype:	.=.+2	/ cpu type (currently 44, 70, 73)
_bootdev:	.=.+2	/ makedev(major,unit) for boot device
_bootcsr:	.=.+2	/ csr of boot controller
_bootctlr:	.=.+2	/ number of boot controller (bits 6 and 7 of minor)
_ssr3copy:	.=.+2	/ copy of SSR3

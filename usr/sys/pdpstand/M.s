/
/	SCCS id	@(#)M.s	1.7 (Berkeley)	7/11/83
/		@(#)M.s	3.1 (2.11BSD)	1995/06/01 (sms@wlv.iipo.gtegsc.com)
/
/ Startup code for two-stage bootstrap with support for autoboot.
/ Supports 11/45, 11/70, 11/53, 11/73, 11/83, 11/84, 11/93, 11/94

/  The boot options and device are placed in the last SZFLAGS bytes
/  at the end of core by the kernel if this is an autoboot.
/  The first-stage boot will leave them in registers for us;
/  we clobber possible old flags so that they don't get re-used.
ENDCORE=	160000		/ end of core, mem. management off
SZFLAGS=	6		/ size of boot flags
BOOTOPTS=	2		/ location of options, bytes below ENDCORE
BOOTDEV=	4		/ makedev(major,unit)
CHECKWORD=	6		/ ~BOOTOPTS

.globl	_end
.globl	_main,_ubmapset
	jmp	start

/
/ trap vectors
/
	trap;340	/ bus error -- grok!
	trap;341	/ illegal instruction
	trap;342	/ BPT
	trap;343	/ IOT
	trap;344	/ POWER FAIL
	trap;345	/ EMT
tvec:
	start;346	/ TRAP

/	NOTE	NOTE	NOTE	NOTE	NOTE	NOTE
/
/ Here is a totally tacky ``fix'' if your kernel is larger than
/ 192K and therefore overwrites boot here.  Just change the .=400^.
/ below to something like .=10240^.  This will move the critical
/ sections of boot up far enough so that the load can finish.
/ We can't actually load boot too much higher because it can't use memory
/ above 256-8K (to avoid UNIBUS mapping problems).
/
.=400^.

start:
	reset
	mov	$340,PS
	mov	$_end+512.,sp

/save boot options, if present
	mov	r4,_bootopts
	mov	r3,_bootdev
	mov	r2,_checkword
	mov	r1,_bootcsr		/ 'boot' will apply ADJcsr[] correction
/clobber any boot flags left in memory
	clr	ENDCORE-BOOTOPTS
	clr	ENDCORE-BOOTDEV
	clr	ENDCORE-CHECKWORD
/
/ determine what kind of cpu we are running on.  this was totally rewritten
/ when support for the 93 and 94 was added.

	clrb	_sep_id
	clrb	_ubmap
	jsr	pc,cpuprobe	/ fill in _cputype, _ubmap and _sep_id
				/ also sets MSCR bits
	clr	nofault

/
/	Set kernel I space registers to physical 0 and I/O page
/
	clr	r1
	mov	$77406, r2
	mov	$KISA0, r3
	mov	$KISD0, r4
	jsr	pc, setseg
	mov	$IO, -(r3)


/ Set user I space registers to physical N*64kb and I/O page.  This is
/ where boot will copy itself to.  Boot is less simple minded about its
/ I/O addressing than it used to be.  Physical memory addresses are now
/ calculated (because support was needed for running split I/D utilities)
/ rather than assuming that boot is loaded on a 64kb boundary. 
/
/ The constraint forcing us to keep boot in the bottom 248Kb of
/ memory is UNIBUS mapping.  There would be little difficulty in relocating
/ Boot much higher on a Qbus system.
/
/ Unless boot's method of managing its I/O addressing and physical addressing 
/ is reworked some more, 3*64Kb +/- a couple Kb is probably the highest
/ we'll ever relocate boot.  This means that the maximum size
/ of any program boot can load is ~192Kb.  That size includes text, data
/ and bss.

N	= 3			/ 3*64Kb = 192Kb

	mov	$N*64.*16., r1	/ N*64 * btoc(1024)
	mov	$UISA0, r3
	mov	$UISD0, r4
	jsr	pc, setseg
	mov	$IO, -(r3)

/
/	If 11/40 class processor, only need set the I space registers
/
	movb	_sep_id, _ksep
	jeq	1f

/
/	Set kernel D space registers to physical 0 and I/O page
/
	clr	r1
	mov	$KDSA0, r3
	mov	$KDSD0, r4
	jsr	pc, setseg
	mov	$IO, -(r3)

/
/	Set user D space registers to physical N*64kb and I/O page
/
	mov	$N*64.*16., r1	/ N*64 * btoc(1024)
	mov	$UDSA0, r3
	mov	$UDSD0, r4
	jsr	pc, setseg
	mov	$IO, -(r3)

1:
/ enable map
	tstb	_ubmap
	beq	2f
	jsr	pc,_ubmapset	/ 24, 44, 70 -> ubmap
	tstb	_sep_id
	bne	3f
	mov	$60,SSR3	/ 24 -> !I/D
	br	1f
3:
	mov	$65,SSR3	/ 44, 70 -> ubmap, I/D
	br	1f
2:
	tstb	_sep_id		/ 23, 34, 40, 45, 60, 73 -> no ubmap
	beq	1f
	mov	$25,SSR3	/ 45, 73 -> no ubmap, I/D; maybe 22-bit
1:
	mov	$30340,PS
	inc	SSR0		/ turn on memory management

/ copy program to user I space
	mov	$_end,r0
	clc
	ror	r0
	clr	r1
1:
	mov	(r1),-(sp)
	mtpi	(r1)+
	sob	r0,1b

/ continue execution in user space copy.  No sense in loading sp with
/ anything special since the call to _main below overwrites low core.

	mov	$140340,-(sp)
	mov	$user,-(sp)
	rtt
user:
/ clear bss
	mov	$_edata,r0
	mov	$_end,r1
	sub	r0,r1
	inc	r1
	clc
	ror	r1
1:
	clr	(r0)+
	sob	r1,1b
	mov	$_end+512.,sp
	mov	sp,r5

	jsr	pc,_main
	mov	_cputype,r0
	mov	_bootcsr,r1	/ csr of boot controller (from ROMs)
	mov	_bootdev,r3	/ makedev(major,unit) (from ROMs & bootblock)
	mov	_bootopts,r4
	mov	r4,r2
	com	r2		/ checkword
	mov	$160000,-(sp)	/ set ksp to very top so that the trap
	mtpi	sp		/ puts the return address and psw at 157774,6
	sys	0		/ can't use "trap" because that's a label

	br	user

cpuprobe:
	mov	$1f,nofault	/ catch possible trap
	tst	*$UBMAP		/ look for unibus map
	incb	_ubmap		/ we've got one, note that and continue on
1:
	mov	$1f,nofault
	tst	*$KDSA6		/ look for split I/D
	incb	_sep_id
1:
	mov	$nomfpt,nofault	/ catch possible fault from instruction
	mfpt			/ 23/24, 44, and KDJ-11 have this instruction
	cmp	r0,$1		/ 44?
	bne	1f		/ no - br
	mov	$1,*$MSCR	/ disable cache parity traps
	mov	$44.,_cputype
	rts	pc
1:
	cmp	r0,$5		/ KDJ-11?
	bne	2f		/ no - br
	mov	*$MAINT,r0	/ get system maint register
	ash	$-4,r0		/ move down and
	bic	$177760,r0	/  isolate the module id
	mov	$1,*$MSCR	/ disable cache parity traps
	movb	j11typ(r0),r0	/ lookup cpu type
	movb	_ubmap,r1	/ unibus?
	beq	1f		/ nope - br
	bis	$2,*$MSCR	/ disable unibus traps
1:
	add	r1,r0		/ bump the cpu type (93 -> 94, 83 ->84)
	br	out
2:
	cmp	r0,$3		/ 23 or 24?
	bne	nomfpt		/ mfpt returned other than 1,3,5 - HELP!
	mov	$23.,r0		/ assume 23
	movb	_ubmap,r1	/ add in...
	add	r1,r0		/ the unibus flag (23 -> 24)
	br	out
nomfpt:
	tstb	_sep_id		/ split I/D present?
	beq	2f		/ no - br
	mov	$45.,r0		/ assume 45
	tstb	_ubmap		/ is that correct?
	beq	out		/ yes - br
	mov	$3,*$MSCR	/ disable unibus and cache traps
	mov	$70.,r0
	br	out
2:
	mov	$40.,r0		/ assume 40
	mov	$out,nofault
	tst	*$MSCR		/ 60 has MSCR, 40 doesn't
	mov	$60.,r0
	mov	$1,*$MSCR
out:
	mov	r0,_cputype
	rts	pc

setseg:
	mov	$8,r0
1:
	mov	r1,(r3)+
	add	$200,r1
	mov	r2,(r4)+
	sob	r0,1b
	rts	pc

.globl	_setseg
_setseg:
	mov	2(sp),r1
	mov	r2,-(sp)
	mov	r3,-(sp)
	mov	r4,-(sp)
	mov	$77406,r2
	mov	$KISA0,r3
	mov	$KISD0,r4
	jsr	pc,setseg
	tstb	_ksep
	bne	1f
	mov	$IO,-(r3)
1:
	mov	(sp)+,r4
	mov	(sp)+,r3
	mov	(sp)+,r2
	rts	pc

.globl	_setnosep
_setnosep:
	bic	$4,SSR3	/ turn off kernel i/d sep
	clrb	_ksep
	rts pc

.globl	_setsep
_setsep:
	bis	$4,SSR3	/ turn on kernel i/d sep (if not already)
	movb	$1,_ksep
	rts pc

/ clrseg(addr,count)
.globl	_clrseg
_clrseg:
	mov	4(sp),r0
	asr	r0
	bic	$!77777,r0
	beq	2f
	mov	2(sp),r1
1:
	clr	-(sp)
	mtpi	(r1)+
	sob	r0,1b
2:
	rts	pc

/ mtpd(word,addr)
.globl	_mtpd
_mtpd:
	mov	4(sp),r0
	mov	2(sp),-(sp)
	mtpd	(r0)+
	rts	pc

/ mtpi(word,addr)
.globl	_mtpi
_mtpi:
	mov	4(sp),r0
	mov	2(sp),-(sp)
	mtpi	(r0)+
	rts	pc

.globl	__rtt
__rtt:
	br	.		/ Can't do halt because that is an illegal
				/   instruction in 'user mode' (which Boot
				/   runs in).

	.globl	_trap

trap:
	mov	*$PS,-(sp)
	tst	nofault
	bne	3f
	mov	r0,-(sp)
	mov	r1,-(sp)
	jsr	pc,_trap
	mov	(sp)+,r1
	mov	(sp)+,r0
	tst	(sp)+
	rtt
3:	tst	(sp)+
	mov	nofault,(sp)
	rtt

PS	= 177776
SSR0	= 177572
SSR1	= 177574
SSR2	= 177576
SSR3	= 172516
KISA0	= 172340
KISA6	= 172354
KISD0	= 172300
KISD7	= 172316
KDSA0	= 172360
KDSA6	= 172374
KDSD0	= 172320
UISA0	= 177640
UISD0	= 177600
UDSA0	= 177660
UDSD0	= 177620
MSCR	= 177746	/ 11/44/60/70 memory system cache control register
MAINT	= 177750	/ KDJ-11 system maintenance register
IO	= 177600
UBMAP	= 170200

.data
.globl	_cputype
.globl	_ksep, _sep_id, _ubmap, _ssr3copy
.globl	_bootopts, _bootdev, _checkword, _bootcsr, _bootctlr

_ssr3copy:	.=.+2	/ copy of SSR3.  Always 0 in Boot because that runs
			/ in user mode.  The standalone utilities which run
			/ in kernel mode have their copy of SSR3 in srt0.s
nofault:	.=.+2	/ where to go on predicted trap
_cputype:	.=.+2	/ cpu type
_sep_id:	.=.+1	/ 1 if we have separate I and D
_ksep:		.=.+1	/ 1 if kernel mode has sep I/D enabled
_ubmap:		.=.+2	/ 1 if we have a unibus map
_bootopts:	.=.+2	/ flags if an autoboot
_bootdev:	.=.+2	/ device booted from
_bootcsr:	.=.+2	/ csr of device booted from
_bootctlr:	.=.+2	/ number of controller booted from
_checkword:	.=.+2	/ saved r2, complement of bootopts if an autoboot
j11typ:	.byte 0, 73., 83., 0, 53., 93.

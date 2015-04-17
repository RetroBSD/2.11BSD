/*
 *		LICENSED FROM DIGITAL EQUIPMENT CORPORATION
 *			       COPYRIGHT (c)
 *		       DIGITAL EQUIPMENT CORPORATION
 *			  MAYNARD, MASSACHUSETTS
 *			      1985, 1986, 1987
 *			   ALL RIGHTS RESERVED
 *
 *	THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT
 *	NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL
 *	EQUIPMENT CORPORATION.  DIGITAL MAKES NO REPRESENTATIONS ABOUT
 *	THE SUITABILITY OF THIS SOFTWARE FOR ANY PURPOSE.  IT IS
 *	SUPPLIED "AS IS" WITHOUT EXPRESSED OR IMPLIED WARRANTY.
 *
 *	IF THE REGENTS OF THE UNIVERSITY OF CALIFORNIA OR ITS LICENSEES
 *	MODIFY THE SOFTWARE IN A MANNER CREATING DERIVATIVE COPYRIGHT
 *	RIGHTS, APPROPRIATE COPYRIGHT LEGENDS MAY BE PLACED ON THE
 *	DERIVATIVE WORK IN ADDITION TO THAT SET FORTH ABOVE.
 *
 *	@(#)mch_fpsim.s	1.2 (2.11BSD GTE) 12/26/92
 */
#include "DEFS.h"

/*
 * Kernel floating point simulator
 */

#if defined(FPSIM) || defined(GENERIC)

m.ext = 200		/ long mode bit
m.lngi = 100		/ long integer mode

_u = 140000			/ XXX
uar0 = _u + U_AR0		/ u.u_ar0
fec  = _u + U_FPERR + F_FEC	/ u.u_fperr.f_fec
fea  = _u + U_FPERR + F_FEA	/ u.u_fperr.f_fea
fpsr = _u + U_FPSR		/ u.u_fps.u.fpsr
ac0  = _u + U_FPREGS + [0.*8.]	/ u.u_fsp.u_fpregs[0]
ac1  = _u + U_FPREGS + [1.*8.]	/ u.u_fsp.u_fpregs[1]
ac2  = _u + U_FPREGS + [2.*8.]	/ u.u_fsp.u_fpregs[2]
ac3  = _u + U_FPREGS + [3.*8.]	/ u.u_fsp.u_fpregs[3]

/*
 * fptrap()
 *
 * Return Status:
 *	0 successful simulation
 *	otherwise the signal number.
 */
ENTRY(fptrap)
	jsr	r5,csv
	sub	$74,sp
	instr = -12		/	int	instr;
	trapins = -14		/	int	trapins;
	pctmp = -24		/	double	pctmp;
	modctl2 = -26		/	int	(*modctl2)();
	modctl = -30		/	int	(*modctl)();
	local = -32		/	int	local;
	bexp = -34		/	int	bexp;
	breg = -44		/	double	breg;
	bsign = -46		/	int	bsign;
	aexp = -50		/	int	aexp;
	areg = -60		/	double	areg;
	asign = -62		/	int	asign;
	sps = -64		/	int	sps;
	spc = -66		/	int	spc;
	ssp = -70		/	int	ssp;
/	sr5 = -72		/	int	sr5;
/	sr4 = -74		/	int	sr5;
/	sr3 = -76		/	int	sr5;
/	sr2 = -100		/	int	sr5;
	sr1 = -102		/	int	sr1;
	sr0 = -104		/	int	sr0;

/ make copies of all the registers - see trap.c (regloc) for the offsets
	mov	$sr0,r1
	add	r5,r1
	mov	$_regloc,r3	/ see trap.c
	mov	$9.,r4		/ r0,1,2,3,4,5,sp,pc,psw
1:
	movb	(r3)+,r2	/ fetch next register offset from u_ar0
	asl	r2		/ make word index
	add	uar0,r2		/ add in u_ar0 
	mov	(r2),(r1)+	/ save register
	sob	r4,1b

/ get the offending instruction
	mov	spc(r5),r1
	dec	r1
	dec	r1
	jsr	pc,ffuiword
	mov	r0,instr(r5)

again:
	sub	$8,sp		/ room for double push  /hmm....
	clr	local(r5)
	mov	instr(r5),r4
	bic	$7777,r4
	cmp	r4,$170000
	beq	1f
	jmp	badins
1:
	/ clear fp error
	bic	$100000,fpsr
	bic	$170000,instr(r5)
	mov	instr(r5),r4
	bit	$7000,r4
	bne	class3
	bit	$700,r4
	bne	class2
	cmp	r4,$12
	blos	1f
	jmp	badins
1:
	asl	r4
	jmp	*agndat(r4)

class2:
	cmp	instr(r5),$400
	bge	1f
	mov	$mod0rx,modctl(r5)
	mov	$mod242,modctl2(r5)
	br	2f
1:
	mov	$mod0f,modctl(r5)
	mov	$mod24f,modctl2(r5)
2:
	jsr	pc,fsrc			/##	jsr	r1,fsrc
	mov	r3,instr(r5)
	asl	r4		/	r4 = (r4&0700)>>7;
	asl	r4
	clrb	r4
	swab	r4
	asl	r4
	jsr	pc,*cls2dat(r4)
	jmp	sret


class3:
	cmp	instr(r5),$5000
	blt	1f
	mov	instr(r5),r2
	clrb	r2
	cmp	r2,$6400
	blt	2f
	sub	$1400,r2
2:
	cmp	r2,$5000
	bne	2f
	mov	$mod0rx,modctl(r5)
	mov	$mod242,modctl2(r5)
	br	3f
2:
	cmp	r2,$5400
	bne	2f
	mov	$mod0ra,modctl(r5)
	mov	$mod24i,modctl2(r5)
	br	3f
2:
	mov	$mod0f,modctl(r5)
	mov	$mod24d,modctl2(r5)
	br	3f
1:
	mov	$mod0f,modctl(r5)
	mov	$mod24f,modctl2(r5)
3:
	jsr	pc,fsrc			/###	jsr	r1,fsrc
	jsr	pc,freg
	mov	r2,instr(r5)
	bis	$2,local(r5)	/ mark as local, since (r2) is always local
	clrb	r4		/	r4 = r4 & ~0377 >> 8;
	swab	r4
	asl	r4
	jsr	pc,*cls3dat(r4)
	br	sret


i.cfcc:
	mov	fpsr,r1		/ get the FPP reg
	bic	$!17,r1		/ clear everything except the NZVC bits
	bic	$17,sps(r5)	/ clear the old NZVC bits
	bis	r1,sps(r5)	/ set the new NZVC bits
	br	ret

i.setf:
	bic	$m.ext,fpsr
	br	ret

i.setd:
	bis	$m.ext,fpsr
	br	ret

i.seti:
	bic	$m.lngi,fpsr
	br	ret

i.setl:
	bis	$m.lngi,fpsr
	br	ret

sret:
	mov	$fpsr,r2
	bic	$17,(r2)
	bit	$2,local(r5)
	bne	1f
	mov	instr(r5),r1
	jsr	pc,ffuword
	br	2f
1:
	mov	*instr(r5),r0
2:
	swab	r0
	tstb	r0
	bpl	1f
	bis	$10,(r2)
	br	ret
1:
	bne	ret
	bis	$4,(r2)

ret:
	/ restore all the new register values
	mov	$sr0,r1; add	r5,r1
	mov	$_regloc,r3
	mov	$9.,r4
1:
	movb	(r3)+,r2
	asl	r2
	add	uar0,r0
	mov	(r1)+,(r0)
	sob	r4,1b	

	bit	$020,sps(r5)	/ Check to see if T bit was set.
	bne	1f
	mov	spc(r5),r1	/ Check the next instruction
	jsr	pc,ffuiword	/ to see if it is another
	cmp	r0,$170000	/ floating point instruction.
	blo	3f
	mov	r0,instr(r5)
	add	$2,spc(r5)	/ Update our copy of pc,
	mov	uar0,r0		/ update
	add	$2,2.(r0)	/ the real pc,
	jbr	again		/ and save the trap.
3:
	clr	r0		/ Normal Return
2:
	jmp	cret
1:
	mov	$SIGTRAP.,r0
	br	2b
badins:				/ Illegal Instruction
	mov	$SIGILL.,r0
	br	2b
segfault:			/ Segmentation Violation
	mov	uar0,r0		/ Don't update any registers, but
	sub	$2,2.(r0)	/ back up the pc to point to the instruction.
	mov	$SIGSEGV.,r0
	br	2b
fpexcept:			/ Floating Point Exception
	/ restore all the new register values, and then
	/ return an error.
	mov	$sr0,r1; add	r5,r1
	mov	uar0,r0
	mov	(r1)+,(r0)	/ r0
	mov	(r1)+,-4.(r0)	/ r1
	mov	(r1)+,-20.(r0)	/ r2
	mov	(r1)+,-18.(r0)	/ r3
	mov	(r1)+,-16.(r0)	/ r4
	mov	(r1)+,-12.(r0)	/ r5
	mov	(r1)+,-6.(r0)	/ sp (r6)
	mov	(r1)+,2.(r0)	/ pc (r7)
	mov	(r1)+,4.(r0)	/ psw
	mov	$SIGFPE.,r0
	jmp	cret

freg:
	mov	instr(r5),r2
	bic	$!300,r2
	asr	r2
	asr	r2
	asr	r2
	add	$ac0,r2
	rts	pc

fsrc:
	mov	instr(r5),r3
	bic	$!7,r3			/ register
	asl	r3
	add	$sr0,r3; add	r5,r3
	mov	instr(r5),r0
	bic	$!70,r0			/ mode
	asr	r0
	asr	r0
	jmp	*moddat(r0)


mod24f:
	mov	$4,r0
	bit	$m.ext,fpsr
	beq	1f
	add	$4,r0
1:
	rts	pc

mod24d:
	mov	$8,r0
	bit	$m.ext,fpsr
	beq	1f
	sub	$4,r0
1:
	rts	pc

mod242:
	mov	$2,r0
	rts	pc

mod24i:
	mov	$2,r0
	bit	$m.lngi,fpsr
	beq	1f
	add	$2,r0
1:
	rts	pc

mod0:
	jmp	*modctl(r5)

mod0f:
	sub	$sr0,r3
	sub	r5,r3
	cmp	r3,$6*2
	bhis	badi1
	asl	r3
	asl	r3
	add	$ac0,r3
	br	mod0rx

mod0ra:
	bit	$m.lngi,fpsr
	bne	badi1

mod0r:
	mov	$ssp,-(sp); add	r5,(sp)
	cmp	r3,(sp)+
	bhis	badi1
mod0rx:
	bis	$3,local(r5)	/ mark it as a local addr, not a user addr
	rts	pc		/###	rts	r1

mod1:				/ register deferred *rn or (rn)
	mov	$spc,-(sp); add	r5,(sp)
	cmp	r3,(sp)+
	beq	badi1
	mov	(r3),r3
	br	check

mod2:
	mov	(r3),-(sp)
	jsr	pc,*modctl2(r5)
	mov	$spc,-(sp); add	r5,(sp)
	cmp	r3,(sp)+
	bne	1f
	mov	(r3),r1		/ PC relative - immediate $n
	jsr	pc,ffuiword
	mov	r0,pctmp(r5)
	mov	$pctmp,(sp); add	r5,(sp)
	/ need to clean garbage out of rest of pctmp(r5)
	mov	(sp),r0
	tst	(r0)+
	clr	(r0)+
	clr	(r0)+
	clr	(r0)+
	mov	$2,r0
	bis	$3,local(r5)	/ signify address is not in user space

1:				/ Auto increment (rn)+
	add	r0,(r3)
	mov	(sp)+,r3
	br	check

mod3:
	mov	(r3),r1
	mov	$spc,-(sp); add	r5,(sp)
	cmp	r3,(sp)+
	bne	1f
	jsr	pc,ffuiword	/ PC Absolute	*$A
	br	2f
1:
	jsr	pc,ffuword	/autoincrement deferred *(rn)+
2:
	add	$2,(r3)
	mov	r0,r3
	br	check

mod4:	/ Autodecrement -(rn)
	mov	$spc,-(sp); add	r5,(sp)
	cmp	r3,(sp)+	/ test pc
	beq	badi1
	jsr	pc,*modctl2(r5)
	sub	r0,(r3)
	mov	(r3),r3
	br	check

mod5:	/ Autodecrement Deferred *-(rn)
	mov	$spc,-(sp); add	r5,(sp)
	cmp	r3,(sp)+
	beq	badi1
	sub	$2,(r3)
	mov	(r3),r1
	jsr	pc,ffuword
	mov	r0,r3
	br	check

mod6:	/ Index or PC relative
	mov	spc(r5),r1
	jsr	pc,ffuiword
	add	$2,spc(r5)
	add	(r3),r0
	mov	r0,r3
	br	check

mod7:	/ Index Deferred or PC Relative Deferred
	jsr	pc,mod6		/###	jsr	r1,mod6
	mov	r3,r1
	jsr	pc,ffuword
	mov	r0,r3
	br	check

badi1:
	jmp	badins

check:
	bit	$1,r3
	bne	1f
	rts	pc		/###	rts	r1
1:
	jmp	segfault

setab:
	bis	$4,local(r5)
	mov	$asign,r0; add	r5,r0
	jsr	pc,seta
	mov	r3,r2
	bit	$1,local(r5)
	bne	1f
	bic	$4,local(r5)
1:
	mov	$bsign,r0; add	r5,r0

seta:
	clr	(r0)
	bit	$4,local(r5)
	bne	4f
	mov	r0,-(sp)
	mov	r2,r1; jsr	pc,ffuword; mov	r0,r1
	add	$2,r2
	mov	(sp)+,r0
	br	5f
4:
	mov	(r2)+,r1
5:
	mov	r1,-(sp)
	beq	1f
	blt	2f
	inc	(r0)+
	br	3f
2:
	dec	(r0)+
3:
	bic	$!177,r1
	bis	$200,r1
	br	2f
1:
	clr	(r0)+
2:
	mov	r1,(r0)+
	bit	$4,local(r5)
	bne	4f
	mov	r1,-(sp)
	mov	r3,-(sp)
	mov	r0,r3
	mov	r2,r1
	jsr	pc,ffuword; add	$2,r1; mov	r0,(r3)+
	bit	$m.ext,fpsr
	beq	5f
	jsr	pc,ffuword; add	$2,r1; mov	r0,(r3)+
	jsr	pc,ffuword; add	$2,r1; mov	r0,(r3)+
	br	6f
5:
	clr	(r3)+
	clr	(r3)+
6:
	mov	r1,r2
	mov	r3,r0
	mov	(sp)+,r3
	mov	(sp)+,r1
	br	3f
4:
	mov	(r2)+,(r0)+
	bit	$m.ext,fpsr
	beq	2f
	mov	(r2)+,(r0)+
	mov	(r2)+,(r0)+
	br	3f
2:
	clr	(r0)+
	clr	(r0)+
3:
	mov	(sp)+,r1
	asl	r1
	clrb	r1
	swab	r1
	sub	$200,r1
	mov	r1,(r0)+	/ exp
	rts	pc

norm:
	mov	$areg,r0; add	r5,r0
	mov	(r0)+,r1
	mov	r1,-(sp)
	mov	(r0)+,r2
	bis	r2,(sp)
	mov	(r0)+,r3
	bis	r3,(sp)
	mov	(r0)+,r4
	bis	r4,(sp)+
	bne	1f
	clr	asign(r5)
	rts	pc
1:
	bit	$!377,r1
	beq	1f
	clc
	ror	r1
	ror	r2
	ror	r3
	ror	r4
	inc	(r0)
	br	1b
1:
	bit	$200,r1
	bne	1f
	asl	r4
	rol	r3
	rol	r2
	rol	r1
	dec	(r0)
	br	1b
1:
	mov	r4,-(r0)
	mov	r3,-(r0)
	mov	r2,-(r0)
	mov	r1,-(r0)
	rts	pc

.globl	_grow, nofault
PS = 177776

ffuword:
	mov	$1f,trapins(r5)
	mov	PS,-(sp)
	SPLHIGH
	mov	nofault,-(sp)
	mov	$ferr1,nofault
1:
	mfpd	(r1)
	mov	(sp)+,r0
	br	2f

ffuiword:
	mov	PS,-(sp)
	SPLHIGH
	mov	nofault,-(sp)
	mov	$ferr2,nofault	/stack isn't in I space, so just bomb out.
	mfpi	(r1)
	mov	(sp)+,r0
	br	2f

fsuword:
	mov	$1f,trapins(r5)
	mov	PS,-(sp)
	SPLHIGH
	mov	nofault,-(sp)
	mov	$ferr1,nofault
1:
	mov	r0,-(sp)
	mtpd	(r1)
2:
	mov	(sp)+,nofault
	mov	(sp)+,PS
	rts	pc

ferr1:
	/first fault could be because we need to grow the stack.
	mov	(sp)+,nofault
	mov	(sp)+,PS
	mov	r0,-(sp)	/save r0 and r1 because
	mov	r1,-(sp)	/grow() will muck them up
	mov	ssp(r5),-(sp)
	jsr	pc,_grow
	tst	(sp)+
	tst	r0
	bne	1f
	jmp	segfault
1:
	mov	(sp)+,r1	/restore r1
	mov	(sp)+,r0	/and r0
	mov	PS,-(sp)
	SPLHIGH
	mov	nofault,-(sp)
	mov	$ferr2,nofault
	jmp	*trapins(r5)

ferr2:
	/second fault, we have a valid memory fault now,
	/so make the users program bomb out!
	mov	(sp)+,nofault
	mov	(sp)+,PS
	jmp	segfault


/ class 2 instructions

fiuv	= 04000

i.ldfps:
	bit	$1,local(r5)
	beq	1f
	mov	(r3),fpsr
	br	2f
1:
	mov	r3,r1
	jsr	pc,ffuword
	mov	r0,fpsr
2:
	jmp	ret

i.stfps:
	bit	$1,local(r5)
	beq	1f
	mov	fpsr,(r3)
	br	2f
1:
	mov	fpsr,r0
	mov	r3,r1
	jsr	pc,fsuword
2:
	jmp	ret

i.stst:
	bit	$1,local(r5)
	beq	1f
	/ must be either a register or immediate mode, only save fec
	mov	fec,(r3)+
	br	2f
1:
	mov	fec,r0
	mov	r3,r1
	jsr	pc,fsuword
	mov	fea,r0
	add	$2,r1
	jsr	pc,fsuword
2:
	jmp	ret

i.clrx:
	bit	$1,local(r5)
	bne	1f
	clr	r0
	mov	r3,r1
	jsr	pc,fsuword
	add	$2,r1
	jsr	pc,fsuword
	bit	$m.ext,fpsr
	beq	2f
	add	$2,r1
	jsr	pc,fsuword
	add	$2,r1
	jsr	pc,fsuword
2:
	rts	pc
1:
	clr	(r3)+
	clr	(r3)+
	bit	$m.ext,fpsr
	beq	2f
	clr	(r3)+
	clr	(r3)+
2:
	rts	pc

i.tstx:
	bit	$fiuv,fpsr
	bne	2f
1:
	rts	pc
	/this could be real easy, except that the lousy tstx instruction
	/ does the fiuv trap AFTER execution, not before. So, since
	/ normally this instruction doesn't get done until after the rts pc,
	/ we explicitly do it here.
2:
	bit	$2,local(r5)
	bne	1b
	mov	$fpsr,r2
	bic	$17,(r2)
	mov	instr(r5),r1
	jsr	pc,ffuword
	swab	r0
	tstb	r0
	bpl	1f
	bis	$10,(r2)	/set negative flag
	br	2f
1:
	bne	2f
	bis	$4,(r2)		/set zero flag
2:
	jsr	pc,chkuv	/finally, check for sign bit and a biased 0 exp
	jmp	ret

i.absx:
	bit	$1,local(r5)
	beq	1f
	bic	$!77777,(r3)
	rts	pc
1:
	mov	r3,r1
	jsr	pc,ffuword
	bit	$fiuv,fpsr
	beq	2f
	jsr	pc,chkuv
2:
	bic	$!77777,r0
	mov	r3,r1
	jsr	pc,fsuword
	rts	pc

chkuv:
	mov	r0,-(sp)
	bic	$77,(sp)	/clear the fraction part
	bit	$140000,(sp)+	/check for sign bit and biased exponent of 0
	bne	1f
	jmp	undefvar
1:
	rts	pc

undefvar:
	mov	$12., fec
	jmp	fpexcept

i.negx:
	bit	$1,local(r5)
	bne	1f

	mov	r3,r1
	jsr	pc,ffuword
	bit	$fiuv,fpsr
	beq	2f
	jsr	pc,chkuv
2:
	tst	r0
	beq	2f
	add	$100000,r0
	mov	r3,r1
	jsr	pc,fsuword
2:
	rts	pc
1:
	tst	(r3)
	beq	2f
	add	$100000,(r3)
2:
	rts	pc

/ class 3

i.ldx:
	bit	$1,local(r5)
	bne	1f
	/user to kernel
	mov	r3,r1; jsr	pc,ffuword
	bit	$fiuv,fpsr
	beq	2f
	jsr	pc,chkuv
2:
	mov	r0,(r2)+
	add	$2,r1; jsr	pc,ffuword; mov	r0,(r2)+
	bit	$m.ext,fpsr
	beq	2f
	add	$2,r1; jsr	pc,ffuword; mov	r0,(r2)+
	add	$2,r1; jsr	pc,ffuword; mov	r0,(r2)+
	rts	pc
1:
	/kernel to kernel
	mov	(r3)+,(r2)+
	mov	(r3)+,(r2)+
	bit	$m.ext,fpsr
	beq	2f
	mov	(r3)+,(r2)+
	mov	(r3)+,(r2)+
	rts	pc
2:
	clr	(r2)+
	clr	(r2)+
	rts	pc

i.stx:
	bit	$1,local(r5)
	bne	1f
	/kernel to user
	mov	(r2)+,r0; mov	r3,r1; jsr	pc,fsuword;
	mov	(r2)+,r0; add	$2,r1; jsr	pc,fsuword
	bit	$m.ext,fpsr
	beq	2f
	mov	(r2)+,r0; add	$2,r1; jsr	pc,fsuword
	mov	(r2)+,r0; add	$2,r1; jsr	pc,fsuword
	br	2f
1:
	/kernel to kernel
	mov	(r2)+,(r3)+
	mov	(r2)+,(r3)+
	bit	$m.ext,fpsr
	beq	2f
	mov	(r2)+,(r3)+
	mov	(r2)+,(r3)+
2:
	jmp	ret			/ does not set cc's

i.cmpx:
	mov	$areg,r4; add	r5,r4
	mov	r4,instr(r5)	/ bit 2 of local(r5) is already set.
	bit	$1,local(r5)
	bne	9f
	mov	r3,r1; jsr	pc,ffuword
	bit	$fiuv,fpsr
	beq	1f
	jsr	pc,chkuv
1:
	tst	(r2)
	bge	1f
	tst	r0
	bge	1f
	cmp	(r2),r0
	bgt	4f
	blt	3f
1:
	cmp	(r2)+,r0
	bgt	3f
	blt	4f
	add	$2,r1; jsr	pc,ffuword; cmp	(r2)+,r0
	bne	5f
	bit	$m.ext,fpsr
	beq	2f
	add	$2,r1; jsr	pc,ffuword; cmp	(r2)+,r0
	bne	5f
	add	$2,r1; jsr	pc,ffuword; cmp	(r2)+,r0
	beq	2f
	br	5f

9:
	tst	(r2)
	bge	1f
	tst	(r3)
	bge	1f
	cmp	(r2),(r3)
	bgt	4f
	blt	3f
1:
	cmp	(r2)+,(r3)+
	bgt	3f
	blt	4f
	cmp	(r2)+,(r3)+
	bne	5f
	bit	$m.ext,fpsr
	beq	2f
	cmp	(r2)+,(r3)+
	bne	5f
	cmp	(r2)+,(r3)+
	beq	2f
5:
	bhi	3f
4:
	movb	$1,1(r4)
	rts	pc
3:
	mov	$-1,(r4)
	rts	pc
2:
	clr	(r4)
	rts	pc

i.ldcyx:	/ldcdf or ldcfd
	bit	$1,local(r5)
	bne	1f
	mov	r3,r1; jsr	pc,ffuword
	bit	$fiuv,fpsr
	beq	2f
	jsr	pc,chkuv
2:
	mov	r0,(r2)+
	add	$2,r1; jsr	pc,ffuword; mov	r0,(r2)+
	bit	$m.ext,fpsr
	bne	2f
	add	$2,r1; jsr	pc,ffuword; mov	r0,(r2)+
	add	$2,r1; jsr	pc,ffuword; mov	r0,(r2)+
	rts	pc
1:
	mov	(r3)+,(r2)+
	mov	(r3)+,(r2)+
	bit	$m.ext,fpsr
	bne	2f
	mov	(r3)+,(r2)+
	mov	(r3)+,(r2)+
	rts	pc
2:
	clr	(r2)+
	clr	(r2)+
	rts	pc

i.stcxy:
	bit	$1,local(r5)
	bne	1f
	mov	(r2)+,r0; mov	r3,r1; jsr	pc,fsuword
	mov	(r2)+,r0; add	$2,r1; jsr	pc,fsuword
	bit	$m.ext,fpsr
	bne	2f
	clr	r0
	add	$2,r1; jsr	pc,fsuword
	add	$2,r1; jsr	pc,fsuword
	br	2f
1:
	mov	(r2)+,(r3)+
	mov	(r2)+,(r3)+
	bit	$m.ext,fpsr
	bne	2f
	clr	(r3)+
	clr	(r3)+
2:
	rts	pc

i.ldcjx:
	mov	$asign,r2; add	r5,r2
	mov	$1,(r2)+
	bit	$1,local(r5)
	bne	1f
	mov	r3,r1; jsr	pc,ffuword
	bit	$fiuv,fpsr
	beq	2f
	jsr	pc,chkuv
2:
	mov	r0,(r2)+
	bit	$m.lngi,fpsr
	beq	3f
	add	$2,r1; jsr	pc,ffuword; mov	r0,(r2)+
	br	2f
1:
	mov	(r3)+,(r2)+
	bit	$m.lngi,fpsr
	beq	3f
	mov	(r3)+,(r2)+
2:
	clr	(r2)+
	clr	(r2)+
	mov	$32.-8,(r2)+
	jmp	saret
3:
	clr	(r2)+
	clr	(r2)+
	clr	(r2)+
	mov	$16.-8,(r2)
	jmp	saret

i.stcxj:
	mov	r3,instr(r5)
	bit	$1,local(r5)
	bne	1f		/bit 2 of local(r5) is already set
	bic	$2,local(r5)
1:
	mov	$asign,r0; add	r5,r0
	bis	$4,local(r5)	/tell seta that r2 is a local addr
	jsr	pc,seta
	clr	r4
	mov	$areg,r0; add	r5,r0
	mov	(r0)+,r1
	mov	(r0)+,r2
	mov	(r0)+,r3
	mov	aexp(r5),r0
1:
	cmp	r0,$48.-8
	bge	1f
	clc
	ror	r1
	ror	r2
	ror	r3
	inc	r0
	br	1b
1:
	bgt	7f
	tst	r1
	beq	1f
7:
	bis	$1,r4			/ C-bit
1:
	bit	$m.lngi,fpsr
	beq	1f
	tst	asign(r5)
	bge	2f
	neg	r3
	adc	r2
	bcs	2f
	neg	r2
	bis	$10,r4			/ N-bit
2:
	bit	$2,local(r5)
	bne	9f
	mov	r4,-(sp)	/ save r4
	mov	r1,-(sp)	/ save r1
	mov	r0,-(sp)	/ save r0
	mov	r2,r0; mov	instr(r5),r1; jsr	pc,fsuword
	mov	r3,r0; add	$2,r1; jsr	pc,fsuword
	mov	(sp)+,r0	/ restore r0
	mov	(sp)+,r1	/ restore r1
	br	t1
9:
	mov	r4,-(sp)	/ save r4
	mov	instr(r5),r4
	mov	r2,(r4)
	mov	r3,2(r4)
t1:
	mov	(sp)+,r4	/ restore r4
	bis	r2,r3
	br	8f
1:
	tst	r2
	beq	1f
	bis	$1,r4			/ C-bit
1:
	tst	asign(r5)
	bge	2f
	neg	r3
	bis	$10,r4			/ N-bit
2:
	bit	$1,local(r5)
	bne	9f
	mov	r3,r0
	mov	instr(r5),r1
	jsr	pc,fsuword
	tst	r3
	br	8f
9:
	mov	r3,*instr(r5)
8:
	bne	1f
	bis	$4,r4			/ Z-bit
1:
	bic	$17,sps(r5)
	bic	$17,fpsr
	bis	r4,sps(r5)
	bis	r4,fpsr
	jmp	ret

xoflo:
	bis	$1,fpsr		/ set fixed overflow (carry)
	jmp	ret

i.ldexp:
	mov	$asign,r0; add	r5,r0
	bis	$4,local(r5)	/tell seta that r2 is a local addr
	jsr	pc,seta
	bit	$1,local(r5)
	bne	1f
	mov	r3,r1; jsr	pc,ffuword; mov	r0,aexp(r5)
	br	2f
1:
	mov	(r3),aexp(r5)
2:
	jsr	pc,reta
	jmp	sret

i.stexp:
	mov	$asign,r0; add	r5,r0
	bis	$4,local(r5)	/tell seta that r2 is a local addr
	jsr	pc,seta
	bit	$1,local(r5)
	bne	1f
	mov	aexp(r5),r0; mov	r3,r1; jsr	pc,fsuword
	mov	r3,instr(r5)
	bic	$17,sps(r5)
	tst	aexp(r5)
	br	3f
1:
	mov	aexp(r5),(r3)
	mov	r3,instr(r5)
	bic	$17,sps(r5)
	tst	(r3)
3:
	bmi	1f
	bne	2f
	bis	$4,sps(r5)		/ Z-bit
	br	2f
1:
	bis	$10,sps(r5)		/ N-bit
2:
	/ next 4 lines because of previous mov r3,instr(r5)
	bit	$1,local(r5)	/ bit 2 of local(r5) is currently set.
	bne	1f
	bic	$2,local(r5)
1:
	jmp	sret


i.addx:
	jsr	pc,setab
	br	1f

i.subx:
	jsr	pc,setab
	neg	bsign(r5)
1:
	tst	bsign(r5)
	beq	reta
	tst	asign(r5)
	beq	retb
	mov	aexp(r5),r1
	sub	bexp(r5),r1
	blt	1f
	beq	2f
	cmp	r1,$56.
	bge	reta
	mov	$breg,r0; add	r5,r0
	br	4f
1:
	neg	r1
	cmp	r1,$56.
	bge	retb
	mov	$areg,r0; add	r5,r0
4:
	mov	r1,-(sp)
	mov	(r0)+,r1
	mov	(r0)+,r2
	mov	(r0)+,r3
	mov	(r0)+,r4
	add	(sp),(r0)
1:
	clc
	ror	r1
	ror	r2
	ror	r3
	ror	r4
	dec	(sp)
	bgt	1b
	mov	r4,-(r0)
	mov	r3,-(r0)
	mov	r2,-(r0)
	mov	r1,-(r0)
	tst	(sp)+
2:
	mov	$aexp,r1; add	r5,r1
	mov	$bexp,r2; add	r5,r2
	mov	$4,r0
	cmp	asign(r5),bsign(r5)
	bne	4f
	clc
1:
	adc	-(r1)
	bcs	3f
	add	-(r2),(r1)
2:
	dec	r0
	bne	1b
	br	5f
3:
	add	-(r2),(r1)
	sec
	br	2b
	br	5f
4:
	clc
1:
	sbc	-(r1)
	bcs	3f
	sub	-(r2),(r1)
2:
	dec	r0
	bne	1b
	br	5f
3:
	sub	-(r2),(r1)
	sec
	br	2b

saret:
	mov	$areg,r1; add	r5,r1
5:
	tst	(r1)
	bge	3f
	mov	$areg+8,r1; add	r5,r1
	mov	$4,r0
	clc
1:
	adc	-(r1)
	bcs	2f
	neg	(r1)
2:
	dec	r0
	bne	1b
	neg	-(r1)
3:
	jsr	pc,norm
	br	reta

retb:
	mov	$bsign,r1; add	r5,r1
	mov	$asign,r2; add	r5,r2
	mov	$6,r0
1:
	mov	(r1)+,(r2)+
	dec	r0
	bne	1b

reta:
	mov	instr(r5),r2
	mov	$asign,r0; add	r5,r0
	tst	(r0)
	beq	unflo
	mov	aexp(r5),r1
	cmp	r1,$177
	bgt	ovflo
	cmp	r1,$-177
	blt	unflo
	add	$200,r1
	swab	r1
	clc
	ror	r1
	tst	(r0)+
	bge	1f
	bis	$100000,r1
1:
	bic	$!177,(r0)
	bis	(r0)+,r1
	bit	$2,local(r5)
	bne	3f
	mov	r3,-(sp)	/save r3
	mov	r0,r3		/and move r0 to r3
	mov	r1,r0; mov	r2,r1; jsr	pc,fsuword
	mov	(r3)+,r0; add	$2,r1; jsr	pc,fsuword
	bit	$m.ext,fpsr
	beq	2f
	mov	(r3)+,r0; add	$2,r1; jsr	pc,fsuword
	mov	(r3)+,r0; add	$2,r1; jsr	pc,fsuword
2:
	mov	r3,r0		/move r3 back to r0
	mov	(sp)+,r3	/and restor r3
	rts	pc
3:
	mov	r1,(r2)+
	mov	(r0)+,(r2)+
	bit	$m.ext,fpsr
	beq	1f
	mov	(r0)+,(r2)+
	mov	(r0)+,(r2)+
1:
	rts	pc

unflo:
	bit	$2,local(r5)
	bne	1f
	clr	r0
	mov	r2,r1; jsr	pc,fsuword
	add	$2,r1; jsr	pc,fsuword
	bit	$m.ext,fpsr
	beq	2f
	add	$2,r1; jsr	pc,fsuword
	add	$2,r1; jsr	pc,fsuword
	br	2f
1:
	clr	(r2)+
	clr	(r2)+
	bit	$m.ext,fpsr
	beq	2f
	clr	(r2)+
	clr	(r2)+
2:
	rts	pc

ovflo:
	bis	$2,fpsr		/ set v-bit (overflow)
	jmp	ret

i.mulx:
	jsr	pc,i.mul
	jbr	saret

i.modx:
	jsr	pc,i.mul
	jsr	pc,norm
	mov	$asign,r0; add	r5,r0
	mov	$bsign,r1; add	r5,r1
	mov	$6,r2
1:
	mov	(r0)+,(r1)+
	dec	r2
	bne	1b
	clr	r0		/ count
	mov	$200,r1		/ bit
	clr	r2		/ reg offset
1:
	add	r5,r2
	cmp	r0,aexp(r5)
	bge	2f		/ in fraction
	bic	r1,areg(r2)
	br	3f
2:
	bic	r1,breg(r2)
3:
	sub	r5,r2
	inc	r0
	clc
	ror	r1
	bne	1b
	mov	$100000,r1
	add	$2,r2
	cmp	r2,$8
	blt	1b
	jsr	pc,norm
	jsr	pc,reta
	cmp	instr(r5),$ac1
	beq	1f
	cmp	instr(r5),$ac3
	beq	1f
	bit	$200,breg(r5)
	bne	2f
	clr	bsign(r5)
2:
	add	$8,instr(r5)
	jsr	pc,retb
	sub	$8,instr(r5)
1:
	rts	pc

i.divx:
	jsr	pc,setab
	tst	bsign(r5)
	beq	zerodiv
	sub	bexp(r5),aexp(r5)
	jsr	pc,xorsign
	mov	instr(r5),-(sp)
	mov	$areg,r0; add	r5,r0
	mov	(r0),r1
	clr	(r0)+
	mov	(r0),r2
	clr	(r0)+
	mov	(r0),r3
	clr	(r0)+
	mov	(r0),r4
	clr	(r0)+
	mov	$areg,instr(r5); add	r5,instr(r5)
	mov	$400,-(sp)
1:
	mov	$breg,r0; add	r5,r0
	cmp	(r0)+,r1
	blt	2f
	bgt	3f
	cmp	(r0)+,r2
	blo	2f
	bhi	3f
	cmp	(r0)+,r3
	blo	2f
	bhi	3f
	cmp	(r0)+,r4
	bhi	3f
2:
	mov	$breg,r0; add	r5,r0
	sub	(r0)+,r1
	clr	-(sp)
	sub	(r0)+,r2
	adc	(sp)
	clr	-(sp)
	sub	(r0)+,r3
	adc	(sp)
	sub	(r0)+,r4
	sbc	r3
	adc	(sp)
	sub	(sp)+,r2
	adc	(sp)
	sub	(sp)+,r1
	bis	(sp),*instr(r5)
3:
	asl	r4
	rol	r3
	rol	r2
	rol	r1
	clc
	ror	(sp)
	bne	1b
	mov	$100000,(sp)
	add	$2,instr(r5)
	mov	$aexp,-(sp); add	r5,(sp)
	cmp	instr(r5),(sp)+
	blo	1b
	tst	(sp)+
	mov	(sp)+,instr(r5)
	jmp	saret

zerodiv:
	mov	$4,fec
	jmp	fpexcept

i.mul:
	jsr	pc,setab
	add	bexp(r5),aexp(r5)
	dec	aexp(r5)
	jsr	pc,xorsign
	mov	instr(r5),-(sp)
	mov	$breg+4,instr(r5); add	r5,instr(r5)
	bit	$m.ext,fpsr
	beq	1f
	add	$4,instr(r5)
1:
	clr	r0
	clr	r1
	clr	r2
	clr	r3
	clr	r4
1:
	asl	r0
	bne	2f
	inc	r0
	sub	$2,instr(r5)
2:
	cmp	r0,$400
	bne	2f
	mov	$breg,-(sp); add	r5,(sp)
	cmp	instr(r5),(sp)+
	bhi	2f
	mov	$areg,r0; add	r5,r0
	mov	r1,(r0)+
	mov	r2,(r0)+
	mov	r3,(r0)+
	mov	r4,(r0)+
	mov	(sp)+,instr(r5)
	rts	pc
2:
	clc
	ror	r1
	ror	r2
	ror	r3
	ror	r4
	bit	r0,*instr(r5)
	beq	1b
	mov	r0,-(sp)
	mov	$areg,r0; add	r5,r0
	add	(r0)+,r1
	clr	-(sp)
	add	(r0)+,r2
	adc	(sp)
	clr	-(sp)
	add	(r0)+,r3
	adc	(sp)
	add	(r0)+,r4
	adc	r3
	adc	(sp)
	add	(sp)+,r2
	adc	(sp)
	add	(sp)+,r1
	mov	(sp)+,r0
	br	1b

xorsign:
	cmp	asign(r5),bsign(r5)
	beq	1f
	mov	$-1,asign(r5)
	rts	pc
1:
	mov	$1,asign(r5)
	rts	pc


.data
agndat:
	i.cfcc		/ 170000
	i.setf		/ 170001
	i.seti		/ 170002
	badins
	badins
	badins
	badins
	badins
	badins
	i.setd		/ 170011
	i.setl		/ 170012
cls2dat:
	badins		/ 1700xx
	i.ldfps		/ 1701xx
	i.stfps		/ 1702xx
	i.stst		/ 1703xx
	i.clrx		/ 1704xx
	i.tstx		/ 1705xx
	i.absx		/ 1706xx
	i.negx		/ 1707xx
cls3dat:
	badins		/ 1700xx
	badins		/ 1704xx
	i.mulx		/ 1710xx
	i.modx		/ 1714xx
	i.addx		/ 1720xx
	i.ldx		/ 1724xx
	i.subx		/ 1730xx
	i.cmpx		/ 1734xx
	i.stx		/ 1740xx
	i.divx		/ 1744xx
	i.stexp		/ 1750xx
	i.stcxj		/ 1754xx
	i.stcxy		/ 1760xx
	i.ldexp		/ 1764xx
	i.ldcjx		/ 1770xx
	i.ldcyx		/ 1774xx
moddat:
	mod0
	mod1
	mod2
	mod3
	mod4
	mod5
	mod6
	mod7

#endif /* FPSIM || GENERIC */

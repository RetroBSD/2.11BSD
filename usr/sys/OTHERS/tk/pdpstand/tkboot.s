/
//////////////////////////////////////////////////////////////////////
/   Copyright (c) Digital Equipment Corporation 1984, 1985, 1986.    /
/   All Rights Reserved. 					     /
/   Reference "/usr/src/COPYRIGHT" for applicable restrictions.      /
//////////////////////////////////////////////////////////////////////
/
/ ULTRIX-11 Block Zero Bootstrap for TMSCP Magtape
/
/ SCCSID: @(#)tkboot.s	3.0	4/21/86
/
/ On entry boot leaves:
/	r0 = unit #
/	r1 = aip register address
/
/ magtape boot program to load and transfer
/ to a unix entry
/
/ Chung_wu Lee		2/8/85
/

nop	= 240
s1	= 4000
go	= 1

core = 28.
.. = [core*2048.]-512.

/ establish sp and check if running below
/ intended origin, if so, copy
/ program up to 'core' K words.
start:
	nop		/ DEC boot block standard
	br	1f	/ "
1:
	mov	$..,sp
	clr	r4
	mov	sp,r5
	cmp	pc,r5
	bhis	2f
	cmp	(r4),$407
	bne	1f
	mov	$20,r4
1:
	mov	(r4)+,(r5)+
	cmp	r5,$end
	blo	1b
	jmp	(sp)

/ Clear core to make things clean,
2:
	clr	(r4)+
	cmp	r4,sp
	blo	2b

/ TK initialization

	mov	r0,bdunit	/ save unit # booted from
	mov	r1,tkaip	/ save aip register address
	clr	(r1)+		/ start tk init sequence
				/ move pointer to tksa register
	mov	$s1,r5		/ set tk state test bit to step 1
	mov	$1f,r4		/ address of init seq table
	br	2f		/ branch around table
1:
	100000			/ TK_ERR, init step 1
	ring			/ address of ringbase
	0			/ hi ringbase address
	go			/ TK go bit
2:
	tst	(r1)		/ error ?
	bmi	.		/ yes, hang - can't restart !!!
	bit	r5,(r1)		/ current step done ?
	beq	2b		/ no
	mov	(r4)+,(r1)	/ yes, load next step info from table
	asl	r5		/ change state test bit to next step
	bpl	2b		/ if all steps not done, go back
				/ r5 now = 100000, TK_OWN bit
	mov	$400,cmdhdr+2	/ tape VCID = 1
	mov	$36.,cmdhdr	/ command packet length
				/ don't set response packet length,
				/ little shakey but it works.
	mov	r0,tkcmd+4.	/ load unit number
	mov	$11,tkcmd+8.	/ on-line command opcode
	mov	$20000,tkcmd+10.	/ set clear serious exception
	mov	$ring,r2	/ initialize cmd/rsp ring
	mov	$tkrsp,(r2)+	/ address of response packet
	mov	r5,(r2)+	/ set TK owner
	mov	$tkcmd,(r2)+	/ address of command packet
	mov	r5,(r2)+	/ set TK owner
	mov	-2(r1),r0	/ start TK polling
3:
	tst	ring+2		/ wait for response, TK_OWN goes to zero
	bmi	3b
	tstb	tkrsp+10.	/ does returned status = SUCCESS ?
	bne	.		/ no, hang (try it again ???)

/ Pass boot device type ID and unit number to Boot:
	mov	tkrsp+28.,bdmtil	/ media type ID lo
	mov	tkrsp+30.,bdmtih	/ media type ID hi

/ rewind tape to BOT
tstart:
	jsr	pc,tkrew
	clr	r1
	mov	$2,bc
	jsr	pc,tkread	/* skip the first two blocks
	clr	r1
	mov	$1,bc
	jsr	pc,tkread	/* read the first block

/ Find out how big boot is
	mov	*$2,r0		/ text segment size
	add	*$4,r0		/ data segment size
/	sub	$512.,r0	/ They forgot to skip the a.out header!
	sub	$496.,r0
	add	$511.,r0	/ Convert size to block count
	clc			/ UNSIGNED!
	ror	r0
	ash	$-8.,r0
	beq	1f		/ In case boot size < 496 bytes (FAT CHANCE!)
	mov	r0,bc
	jsr	pc,tkread	/ read file into core

/ load boot device type info into r0 -> r4
/ relocate core around
/ assembler header
1:
	jsr	pc,tkrew
	mov	bdunit,r0	/ unit number
	mov	$12.,r1		/ boot device type code 12 = TK (TMSCP)
	mov	bdmtil,r2	/ media type ID
	mov	bdmtih,r3
	mov	tkaip,r4	/ TMSCP controller CSR address
	clr	r5
1:
	mov	20(r5),(r5)+
	cmp	r5,sp
	blo	1b

/ enter program
	clr	pc

/ TK driver rewind routine
tkrew:
/	mov	$24.,cmdhdr		/ length of command packet
	mov	$45,tkcmd+8.		/ reposition opcode
	mov	$0,tkcmd+12.		/ clear record/object count
	mov	$0,tkcmd+16.		/ clear tape mark count
	mov	$20002,tkcmd+10.	/ set rewind & clear serious exception
	mov	$100000,ring+2		/ set TK owner of response
	mov	$100000,ring+6		/ set TK owner of command
	mov	*tkaip,r0		/ start TK polling
1:
	tst	ring+2			/ wait for response
	bmi	1b
	tstb	tkrsp+10.		/ does returned status = SUCCESS ?
	beq	1f			/ yes, return
	jmp	tstart			/ no, error (try it again)
1:
	rts	pc


tkread:
	mov	bc,r3
/	mov	$32.,cmdhdr		/ length of command packet
	mov	$41,tkcmd+8.		/ read opcode
	mov	$20000,tkcmd+10.	/ set clear serious exception
	mov	$512.,tkcmd+12.		/ byte count
	mov	$buf,tkcmd+16.		/ buffer descriptor
1:
	mov	$100000,ring+2		/ set TK owner of response
	mov	$100000,ring+6		/ set TK owner of command
	mov	*tkaip,r0		/ start TK polling
2:
	tst	ring+2			/ wait for response
	bmi	2b
	tstb	tkrsp+10.		/ does returned status = SUCCESS ?
	beq	2f			/ yes
	jmp	tstart			/ no, error (try it again)
2:
	mov	$buf,r2
	mov	tkrsp+12.,r4		/ byte count
	asr	r4			/ word count
3:
	dec	r4			/ decrement word count
	bmi	3f
	mov	(r2)+,(r1)+
	br	3b
3:
	sob	r3,1b
	rts	pc

end:
tkaip = ..-1024.
cmdint = tkaip+2.
rspint = cmdint+2.
ring = rspint+2.
rsphdr = ring+8.
tkrsp = rsphdr+4.
cmdhdr = tkrsp+48.
tkcmd = cmdhdr+4.
bdunit = tkcmd+48.
bdmtil = bdunit+2.
bdmtih = bdmtil+2.
buf = bdmtih+2.
bc = buf+512.

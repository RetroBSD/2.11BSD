/*
 * Primary tape boot program to load and execute secondary boot.
 *
 * 1995/05/31 - unit number changed to be in bits 3-5 of 'bootdev'
 *
 * This is a universal tape boot which can handle HT, TM, TS and TMSCP
 * tapes.  This boot is FULL.  Some of the more extended error
 * checking had to be left out to get all the drivers to fit.
 *
 * Two copies of the primary boot are stored in the first and second records
 * of the first tape file (some boot proms execute the second one when
 * booting a tape).  The secondary boot is also stored in the first tape
 * file, starting at record #3.
 *
 * Also note that the struct exec header must be removed from this bootstrap.
 * This needs to be done so taking the address of the tape read and rewind
 * functions will work.
 *
 * Due to size constraints and the rather destructive way in which
 * all the registers are used, this boot does not support the 
 * "jsr pc,0; br restart" convention.
 */
NEWLOC	= [48.*1024.]			/ we relocate ourselves to this address
OURSIZE = 512.				/ assume we are up to this size

HT_MAJOR = 0				/ major device number from bdevsw[]
TM_MAJOR = 1
TS_MAJOR = 2
TMS_MAJOR = 12.

a_text	= 02				/ a_text (struct exec) field offset
a_data	= 04				/ a_data (struct exec) field offset

csr	= r5				/ saved csr of boot device
tread	= r4				/ pointer at tread routine
blkcnt	= r3				/ number of blocks to read
memaddr	= r2				/ memory location to read into
					/ r1 & r0 are junk registers

.. = NEWLOC				/ so absolute address work ...
start:
	nop				/ DEC boot block standard
	br	1f			/ "   "    "     "
1:
	mov	$NEWLOC,sp		/ give ourselves a stack to work with
	clr	r4
	mov	sp,r5
	mov	$OURSIZE\/2,r3		/ primary boot size in words
2:
	clr	OURSIZE(r5)		/ clear work area (major & TS/MSCP area)
	mov	(r4)+,(r5)+		/ move primary boot to just above
	sob	r3,2b			/   the stack
	jmp	*$3f			/ bypass the relocation code
3:
	mov	r0,unit			/ save unit number
	mov	r1,csr			/ save the csr
	cmp	r1,$172440		/ HT is always at this address
	beq	common			/ r3 is table index
	inc	r3			/ index for TMSCP
	cmp	r1,$172522		/ is this a TS?
	blo	common			/ no - br, likely TMSCP
	cmp	r1,$172522+[7*4]	/ is CSR in the TS range?
	bhi	common			/ no, is a TMSCP - br
	inc	r3			/ adjust index to TS
	mov	(r1),r4			/ save contents of csr in case of a TM
	clr	(r1)			/ poke the controller
/	clr	r2			/ now we delay
/1:
/	sob	r2,1b			/ time for TS to run diagnostics
2:
	tstb	(r1)			/ is controller ready?
	bpl	2b			/ no - br
	bit	$2000,(r1)		/ TS "address required" bit
	bne	common			/ if a TS - br
	mov	r4,(r1)			/ is a TM, restore unit/density select
	inc	r3			/ make TM index
common:
	movb	table1(r3),major+1	/ save major device number to high byte
	asl	r3			/ make a word index
	mov	table2(r3),tread	/ fetch read routine address
	jsr	pc,*table3(r3)		/ call rewind routine (must preserve r4)
readit:
	clr	memaddr			/ load starting at 0
	jsr	pc,*tread		/ skip the two copies of this
	jsr	pc,*tread		/ program on the tape
	clr	memaddr			/ reset memory address
	jsr	pc,*tread		/ read first block of boot

	mov	*$a_text,blkcnt		/ compute remaining amount to read:
	add	*$a_data,blkcnt		/   (a_text + a_data
	add	$15.,blkcnt		/    + sizeof(struct exec) + 511
	ash	$-9.,blkcnt		/    - 512) [already read one block]
	bic	$177600,blkcnt		/   / 512 [[unsigned]]
	beq	done			/ already done if == 0 [not likely]
2:
	jsr	pc,*tread
	sob	blkcnt,2b
done:
1:					/   down by sizeof(struct exec)
	mov	20(blkcnt),(blkcnt)+	/ r3 cleared by loop above
	cmp	blkcnt,sp
	blo	1b
	mov	csr,r1			/ put things where 'boot'
	mov	unit,r3			/  expects them
	ash	$3,r3			/ unit # in bits 3-5
	bis	major,r3		/ the major device to high byte
	clr	pc			/ go to location 0 ... no return

/*
 *	HT tape driver
 */
htcs1	= 0				/ offset from base csr
htwc	= 2
htba	= 4
htfc	= 6
htcs2	= 10
htds	= 12
hter	= 14
httc	= 32

RESET	= 040
READ	= 071
REW	= 07

hrrec:
	mov	memaddr,htba(csr)
	mov	$-256.,htwc(csr)
	mov	$READ,(csr)		/ htcs1
htcmd:
	tstb	(csr)			/ controller ready?
	bpl	htcmd
	tst	htds(csr)		/ drive ready?
	bpl	htcmd
	tstb	htcs2+1(csr)		/ any controller errors?
	bne	ctlerr
	bit	$!1000,hter(csr)	/ any drive errors except HTER_FCE?
	beq	bumpaddr		/ no, go bump address
ctlerr:
	halt

/*
 * Rewind tape.  This routine is only done once and must preceed any reads.
 */
htrew:
	tstb	(csr)			/ controller ready?
	bpl	htrew			/ no - go try again
	mov	htcs2(csr),r1		/ boot unit(formatter) number
	bic	$!7,r1			/ only the low bits
	mov	httc(csr),r0		/ save format,slave,density
	bic	$!3767,r0		/ only the bits we're interested in
	mov	$RESET,htcs2(csr)	/ reset controller
	movb	r1,htcs2(csr)		/ reselect boot unit(formatter)
	mov	r0,httc(csr)		/ reselect density/format/slave(drive)
	mov	$REW,(csr)
	br	htcmd			/ join common code

/*
 *	TM tape driver
 */
tmer	= -2				/ offset from base csr
tmcs	= 0
tmbc	= 2
tmba	= 4
tmdb	= 6
tmrd	= 10
tmmr	= 12

tmrrec:
	mov	$-512.,tmbc(csr)	/ bytecount
	mov	memaddr,tmba(csr)	/ bus address
	mov	$3,r1			/ 'read'
tmcmd:
	mov	(csr),r0
	bic	$!63400,r0		/ save the density and unit
	bis	r1,r0			/ merge in the function code
	mov	r0,(csr)		/ tmcs - give command
/1:
/	bit	$100,tmer(csr)		/ unit still selected? (TMER_SELR)
/	beq	ctlerr			/ nope, go halt
/	bit	$1,tmer(csr)		/ unit ready? (TMER_TUR)
/	beq	1b			/ no, keep waiting
tmtscom:
	bit	$100200,(csr)		/ error or ready?
	beq	tmtscom			/ neither, keep looking
	bmi	ctlerr			/ error - go halt

bumpaddr:
	add	$512.,memaddr
	rts	pc

/*
 * Rewind tape.
 * Side effects:
 *	just what the first line says ...
 */
tmrew:
	mov	$17,r1			/ 'rewind'
	br	tmcmd			/ join common code

/*
 *	TS tape driver
 */
tsdb = -2				/ offset from ROM supplied address
tssr = 0

TSCHAR = 140004
TSREW  = 102010
TSREAD = 100001

tsrrec:
	mov	$tsdbuf+6,r0
	mov	$512.,(r0)
	clr	-(r0)
	mov	memaddr,-(r0)
	mov	$TSREAD,-(r0)
	mov	r0,tsdb(csr)
	br	tmtscom

/*
 * Rewind and initialize tape - only done once.
 * Side effects:
 *	just what the first line says ...
 */
tsrew:
	mov	$tsdbuf,tsdb(csr)
	br	tmtscom			/ go join common code

.if [.-start]&2
	.blkb	2			/ tsdbuf must be on a mod 4 boundary
.endif
tsdbuf:
	TSCHAR				/ command
	tsdbuf+10			/ buffer address (lo)
	0				/ buffer address (hi - always 0)
	10				/ length of command buffer

/ from here on is used only at initialization/rewind time.  The Set
/ Charactistics command only looks at the word following the buffer length,
/ part of the TMSCP code is used as the remainder of the characteristics packet.

	tsdbuf+10			/ buffer address (lo)
	0				/ buffer address (hi - always 0)
	16				/ minimum length packet
	0				/ characteristics to set (lo byte)

/*
 * Copyright (c) Digital Equipment Corporation 1984, 1985, 1986.
 * All Rights Reserved.
 * Reference "/usr/src/COPYRIGHT" for applicable restrictions.
 *
 * ULTRIX-11 Block Zero Bootstrap for TMSCP Magtape
 *
 * SCCSID: @(#)tkboot.s	3.0	4/21/86
 *
 * Chung_wu Lee		2/8/85
 *
 * sms 4/27/91 - merged into universal tape boot
 * sms 4/12/91 - saved some more space.  the major device number and unit
 *	number are now passed to the secondary boot along with the csr of
 *	the booting controller.
 *
 * Steven M. Schultz (sms@wlv.imsd.contel.com) Aug 20 1990.  Port to 2.11BSD
*/

s1	= 4000
go	= 1

/ TK initialization (and rewind) - only done once
tkrew:
	clr	(csr)+		/ start tk init sequence
				/ move pointer to tksa register
	mov	$s1,r0		/ set tk state test bit to step 1
	mov	$cmdtbl,r1	/ address of init seq table
2:
	tst	(csr)		/ error ?
	bmi	.		/ yes, hang - can't restart !!!
	bit	r0,(csr)	/ current step done ?
	beq	2b		/ no
	mov	(r1)+,(csr)	/ yes, load next step info from table
	asl	r0		/ change state test bit to next step
	bpl	2b		/ if all steps not done, go back
				/ r0 now = 100000, TK_OWN bit
	mov	$400,cmdhdr+2	/ tape VCID = 1
	mov	$36.,cmdhdr	/ command packet length
				/ don't set response packet length,
				/ little shakey but it works.
				/ unit is already loaded at tkcmd+4
	mov	$11,tkcmd+8.	/ on-line command opcode
	mov	$20000,tkcmd+10.	/ set clear serious exception
	mov	$ring,r2	/ initialize cmd/rsp ring
	mov	$tkrsp,(r2)+	/ address of response packet
	mov	r0,(r2)+	/ set TK owner
	mov	$tkcmd,(r2)+	/ address of command packet
	mov	r0,(r2)+	/ set TK owner
	mov	-(csr),r0	/ start TK polling
3:
	jsr	pc,tkready
	mov	$tkcmd+8.,r0
	mov	$45,(r0)+		/ reposition opcode
	mov	$20002,(r0)+		/ set rewind & clear serious exception
	clr	(r0)+			/ clear record/object count
	clr	(r0)+			/ zzz2
	clr	(r0)+			/ clear tape mark count
tkpoll:
	mov	$100000,ring+2		/ set TK owner of response
	mov	$100000,ring+6		/ set TK owner of command
	mov	(csr),r0		/ start TK polling
tkready:
	tst	ring+2			/ wait for response
	bmi	tkready
	tstb	tkrsp+10.		/ does returned status = SUCCESS ?
	bne	.			/ no, hang
	rts	pc
tkread:
	mov	$tkcmd+8.,r0
	mov	$41,(r0)+		/ read opcode
	mov	$20000,(r0)+		/ set clear serious exception
	mov	$512.,(r0)+		/ byte count
	clr	(r0)+			/ zzz2
	mov	memaddr,(r0)+		/ buffer address
	jsr	pc,bumpaddr		/ bump address
	br	tkpoll			/ wait for response

cmdtbl:
	100000			/ TK_ERR, init step 1
	ring			/ address of ringbase
	0			/ hi ringbase address
	go			/ TK go bit

table1:
	.byte HT_MAJOR
	.byte TMS_MAJOR
	.byte TS_MAJOR
	.byte TM_MAJOR
table2:
	hrrec
	tkread
	tsrrec
	tmrrec
table3:
	htrew
	tkrew
	tsrew
	tmrew
end:

major = NEWLOC+OURSIZE
cmdint = major+2		/ TMSCP stuff
rspint = cmdint+2.
ring = rspint+2.
rsphdr = ring+8.
tkrsp = rsphdr+4.
cmdhdr = tkrsp+48.
tkcmd = cmdhdr+4.
unit = tkcmd+4

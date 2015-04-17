/ April 9, 2000 - modified to probe for the TOY clock rather than checking
/	the cpu type for 93 or 94.

/ the notes say that the TOY clock uses 24 hour time, but then later on
/ mention flags dealing with AM/PM...  So, code is present but disabled
/ to handle 12 hour time.  If this code is needed change the 0 below to
/ a 1.

toy24=0

/ extraneous flag bit possible in the day field.  this "should never
/ happen", but if it does change the 0 to a 1 below.

dayflags=0

	.globl	TOYCSR
TOYCSR	= 177526

	.globl	_toyclk, nofault
	.text
_toyclk:
	jsr	r5,csv			/ callable from C, save regs
	sub	$8.,sp			/ need 8 byte scratch area
tdata = -20
	clr	-(sp)
	mov	$1f, nofault		/ catch trap if no toy clock
	tst	*$TOYCSR
	inc	(sp)
1:
	clr	nofault			/ done with trap catcher
	tst	(sp)+			/ did we see a toy clock?
	beq	err			/ no, go return "error"
	jsr	pc,initoy		/ initialize the clock for reading
	mov	r5,(sp)
	add	$tdata,(sp)		/ pointer to scratch area
	jsr	pc,_gettoy		/ read the toy clock
	mov	(sp),r2			/ pointer to scratch area
	mov	$8.,r3			/ number of bytes of clock data
2:
	clr	r1
	bisb	(r2),r1			/ fetch byte of clock data (!sign ext)
.if	toy24
	cmp	r3,$5			/ are we on the hours field?
	bne	3f			/ no - br
	bic	$240,r1			/ clear am/pm flags
3:
.endif
	jsr	pc,bcd			/ convert 2 nybbles bcd to binary
.if	toy24
	cmp	r3,$5			/ hours field?
	bne	4f			/ no - skip am/pm stuff
	tstb	(r2)			/ $200 = am/pm in use
	bpl	4f			/ not am/pm, skip it
	cmp	r1,$12.			/ exactly 12?
	bne	5f			/ no - br
	clr	r1			/ make midnight
5:
	bitb	$40,(r2)		/ PM?
	beq	4f			/ no  - br
	add	$12.,r1			/ convert to 24 hour time
.endif
4:
	movb	r1,(r2)+		/ store converted/correct binary value
	sob	r3,2b			/ continue on for rest of the bytes
	sub	$7,r2			/ back up to seconds field
.if	dayflags
	bicb	$177770,4(r2)		/ clear possible excess bits in day
.endif
	mov	$bounds,r3		/ do bounds checking now, not 100ths
1:
	movb	(r2)+,r1		/ get byte of clock data
	cmpb	r1,(r3)+		/ below lo bound?
	blo	err			/ yes - br
	cmpb	r1,(r3)+		/ above hi bound
	bhi	err			/ yes - br
	cmp	r3,$bounds+14.		/ at end (7 limits * 2 bytes per = 14)?
	blo	1b			/ no - br
	sub	$8.,r2			/ back to seconds field
	movb	7(r2),r0		/ fetch the year of century
	cmp	r0,$90.			/ are we a "90s" system?
	bhis	1f			/ yep - br
	add	$100.,r0		/ next century for years 00 - 89
1:
	movb	r0,7(r2)		/ store fixed up year
	decb	6(r2)			/ convert 1-12 month to 0-11
	mov	r2,(sp)			/ pointer to the toy clock data
	jsr	pc,_tm2t		/ convert to a 32bit # of seconds time
	br	ret
err:
	clr	r0			/ a double precision value
	clr	r1			/ of 0 signals an error
ret:
	jmp	cret			/ return
	.data
bounds:
	.byte	0,59.			/ seconds lo,hi
	.byte	0,59.			/ minutes lo,hi
	.byte	0,23.			/ hours lo,hi
	.byte	0,7			/ day of week lo,hi
	.byte	1,31.			/ day of month lo,hi
	.byte	1,12.			/ month of year lo,hi
	.byte	0,99.			/ year of century lo,hi
	.text
initoy:
	tst	*$TOYCSR		/ strobe the clock register
	clr	-(sp)			/ save previous high byte of register
	movb	*$TOYCSR+1,(sp)		/ only bit 8 belongs to TOY!
	bic	$1,(sp)			/ make sure bit 8 (TOY bit) is clear
	mov	$2,r2			/ number of double words to send clock
1:
	mov	$35305,r0		/ first word of recognition code
	jsr	pc,toyload		/ send it to clock
	mov	$56243,r0		/ second word
	jsr	pc,toyload		/ send it
	sob	r2,1b			/ do the whole thing twice
	tst	(sp)+			/ clean stack
	rts	pc

/ send contents of r0 to the TOY.  2(sp) has the old bits 9-15, bit 8
/ has been cleared.

toyload:
	mov	$16.,r1			/ number of bits to send
1:
	mov	r0,r3			/ scratch copy
	bic	$177776,r3		/ clear all but bit being sent
	bis	2(sp),r3		/ merge in old_csr_bits
	movb	r3,*$TOYCSR+1		/ send bit to clock
	asr	r0			/ shift pattern down
	sob	r1,1b			/ do all 16 bits in the word
	rts	pc

	.globl	_gettoy
_gettoy:				/ (void)gettoy(&char[8]);
	jsr	r5,csv			/ C callable
	mov	4(r5),r2		/ buffer address
	mov	$4,r3			/ number of words in buffer
1:
	mov	$16.,r4			/ number of bits in word
2:
	movb	*$TOYCSR+1,r0		/ low bit of top byte is a clock bit
	asr	r0			/ put it in carry
	ror	r1			/ ripple in at top end of r1
	sob	r4,2b			/ do all 16 bits
	mov	r1,(r2)+		/ store the word in the buffer
	sob	r3,1b			/ do all 4 words
	jmp	cret			/ and return

bcd:
	clr	r0
	div	$16.,r0
	mov	r1,-(sp)
	mov	r0,r1
	mul	$10.,r1
	add	(sp)+,r1
	rts	pc

	.globl	_tm2t
_tm2t:
	jsr	r5,csv
	mov	4(r5),r4
	movb	1(r4),r1
	mov	r1,-(sp)
	clr	-(sp)
	movb	2(r4),r0
	mul	$74,r0
	mov	r1,-(sp)
	mov	r0,-(sp)
	mov	$7020,-(sp)
	sxt	-(sp)
	movb	3(r4),r1
	mov	r1,-(sp)
	clr	-(sp)
	jsr	pc,lmul
	add	$10,sp
	mov	r1,-(sp)
	mov	r0,-(sp)
	mov	$50600,-(sp)
	mov	$1,-(sp)
	mov	r4,-(sp)
	jsr	pc,_ndays		/ return value in r1
	tst	(sp)+
	dec	r1
	mov	r1,-(sp)
	sxt	-(sp)
	jsr	pc,lmul
	add	$10,sp
	add	(sp)+,r0
	add	(sp)+,r1
	adc	r0
	add	(sp)+,r0
	add	(sp)+,r1
	adc	r0
	add	(sp)+,r0
	add	(sp)+,r1
	adc	r0
	jmp	cret

_leap:				/ r2 = year number, r1 clobbered
	mov	r2,r1
	add	$3554,r1
	sxt	r0
	div	$620,r0
	tst	r1
	bne	2f
	br	3f
1:
	clr	r0		/ return false
	br	4f
2:
	mov	r2,r1
	sxt	r0
	div	$144,r0
	tst	r1
	jeq	1b
	bit	$3,r2
	jne	1b
3:
	mov	$1,r0		/ return true
4:
	rts	pc
	.data
mdays:
	.byte 37,34,37,36,37,36,37,37,36,37,36,37,0
	.even
	.text
_ndays:
	jsr	r5,csv
	mov	4(r5),r0
	movb	5(r0),r4
	mov	$106,r2
	jbr	3f
1:
	add	$555,r4
	jsr	pc,*$_leap		/ r2 has year in it already
	add	r0,r4
	inc	r2
3:
	mov	4(r5),r0
	movb	7(r0),r0
	cmp	r2,r0
	jlt	1b
	clr	r3
	jbr	8f
4:
	clr	r0
	cmp	$1,r3
	jne	5f
	jsr	pc,*$_leap	/ r2 has year
5:
	movb	mdays(r3),r1
	add	r1,r0
	add	r0,r4
	inc	r3
8:
	mov	4(r5),r0
	movb	6(r0),r0
	cmp	r3,r0
	jlt	4b
	mov	r4,r1		/ return value in r1 not r0
	jmp	cret

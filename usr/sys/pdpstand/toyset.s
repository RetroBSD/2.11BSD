TOYCSR	= 177526

/ April 6, 2000 - sms@moe.2bsd.com
/ Remove the cpu type (93 or 94) restriction and probe for the TOY register.
/ Some systems have clock boards added _and_ P11 (Begemot Computer Associates'
/ PDP-11 emulator) have support for the TOY clock.
/
/ April 10, 1997 - sms@moe.2bsd.com
/ The day of week calculation was incorrect and would return -1 for Saturday
/ rather than 6.  Alan Sieving spotted this one too (toyset.s must be favorite
/ reading material ;-))
/
/ February 6, 1997 - sms@moe.2bsd.com
/ Forgot that May has 31 days.  Thanks to Alan Sieving (ars@quickware.com) for
/ spotting this.
/
/ August 21, 1993 - Steven M. Schultz (sms@wlv.iipo.gtegsc.com)
/ This is a standalone program which is used to set the TOY (Time Of Year)
/ clock on a PDP-11/93 or 11/94.  If this program is run on other than a
/ 93 or 94 an error is printed and the program 'exits' back to the boot
/ runtime.
/
/ The current date is printed (in the same format used to enter the new date)
/ and the prompt "Toyset> " is displayed.  At that time a string of the form:
/ YYMMDDHHMM[.SS]\n is entered.  The seconds "SS" are optional, if not entered
/ the seconds will be set to 0.  Any invalid input string simply loops back
/ to the top of the program.
/
/ To not change the date and time simply hit a return and the program will
/ exit, returning control to 'boot'.

	.globl	_main, csv, cret, _printf, _gets, _exit, _module
	.globl	nofault

_main:
main:
	jsr	r5,csv			/ srt0.o sets up a C frame...

	jsr	pc,init			/ probe for clock, display current TOY

	clrb	line			/ init buffer
	mov	$line,-(sp)
	jsr	pc,_gets		/ get input from user
	tst	r0
	bgt	1f
leave:
	jsr	pc,_exit		/ exit on error
1:
	tstb	line			/ did we get anything?
	beq	leave			/ nope - go exit

	clr	r3			/ clear '.' seen flag
	mov	$line,r4		/ point to input data
2:
	movb	(r4)+,r0
	beq	1f			/ end of string - go validate it
	cmpb	r0,$'.			/ are there seconds present?
	bne	2b			/ not yet - go try another byte
	clrb	-1(r4)			/ zap '.' - separating two parts of date
	mov	r4,r3			/ set seconds present flag
	tstb	(r3)+			/ must have two...
	beq	main			/ and only two...
	tstb	(r3)+			/ characters after...
	beq	main			/ the '.'...
	tstb	(r3)			/ followed by a ...
	bne	main			/ null character.
1:
	clrb	seconds			/ assume no seconds (start of minute)
	tst	r3			/ do we have seconds?
	beq	nosec			/ no - br
	cmpb	-(r3),-(r3)		/ back up to beginning of seconds string
	mov	r3,-(sp)
	jsr	pc,atoi2		/ convert two digits to binary
	tst	(sp)+
	cmp	r0,$59.			/ range check
	bhi	main			/ error - go back to top
	movb	r0,seconds		/ save for later
nosec:
	sub	$line+1,r4		/ number of characters in date string
	cmp	r4,$10.			/ _must_ have *exactly* "YYMMDDhhmm"
	bne	main			/ back to the top on error
	mov	$line,r4		/ point to start of date string
	mov	r4,-(sp)
	jsr	pc,atoi2		/ convert 2 digits to binary (year)
	tst	(sp)+
	cmp	r4,$-1			/ error?
	beq	main			/ yes - go back to top
	cmp	r0,$69.			/ before [19]69?
	bgt	1f			/ no - it's a 1970-1999 year - br
	add	$100.,r0		/ 21st century and 11s are still around!
1:
	movb	r0,year			/ save year for later
	cmpb	(r4)+,(r4)+		/ skip two digits, move to month
	mov	r4,-(sp)
	jsr	pc,atoi2		/ convert month to binary
	tst	(sp)+
	cmp	r0,$12.			/ range check
	bhi	main			/ back to top on too high
	cmpb	(r4)+,(r4)+		/ move on to day of month
	movb	r0,month		/ save month for later
	beq	main			/ can't have a month 0
	mov	r4,-(sp)
	jsr	pc,atoi2		/ convert day of month to binary
	tst	(sp)+
	movb	month,r1
	cmpb	r0,Mtab-1(r1)		/ crude check (no leap year case)
	bhi	main			/ on day of month
	movb	r0,day			/ save the day for later
	cmpb	(r4)+,(r4)+		/ move along to hours of day
	mov	r4,-(sp)
	jsr	pc,atoi2		/ convert hours of day to binary
	tst	(sp)+
	cmp	r0,$23.			/ can't have more than 23 hours
	bhi	main			/ but 00 is ok (midnight)
	movb	r0,hours		/ save hours for later
	cmpb	(r4)+,(r4)+		/ move over to minutes
	mov	r4,-(sp)
	jsr	pc,atoi2		/ convert minutes to binary
	tst	(sp)+
	cmp	r0,$59.			/ can't have more than 59 minutes
	bhi	main			/ back to top on out of range error
	movb	r0,minutes		/ save for later

/ need to compute the "day of week".  why the TOY clock couldn't figure
/ this out (or do without) itself i don't know.

	jsr	pc,t2dow		/ find out "day of week"
	movb	r0,dow			/ save for later

/ now we have to convert the binary data to BCD.  We needed (or preferred)
/ the binary form for ease of range checking but the TOY wants BCD.  Besides
/ i like to improve my typing skills ;-)

	 movb	seconds,r1
	 jsr	pc,tobcd
	 movb	r0,bcd+1		/ seconds

	 movb	minutes,r1
	 jsr	pc,tobcd
	 movb	r0,bcd+2		/ minutes

	 movb	hours,r1
	 jsr	pc,tobcd
	 movb	r0,bcd+3		/ hours

	 movb	dow,r1
	 jsr	pc,tobcd
	 movb	r0,bcd+4		/ day of week

	 movb	day,r1
	 jsr	pc,tobcd
	 movb	r0,bcd+5		/ day of month

	 movb	month,r1
	 jsr	pc,tobcd
	 movb	r0,bcd+6		/ month of year

	 movb	year,r1
	 jsr	pc,tobcd
	 movb	r0,bcd+7

/ Now initialize the TOY by sending the 'recognition' sequence.  We have
/ to inline this because immediately after the recognition sequence must
/ come the 'write' of data - a 'read' to save the contents of the CSR
/ would tell the TOY we're reading data.  *sigh*

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

/ Now write the data to the TOY without an intervening 'tst' or 'movb'
/ to the CSR.

	mov	bcd,r0			/ first two bytes
	jsr	pc,toyload
	mov	bcd+2,r0		/ bytes 3 and 4
	jsr	pc,toyload
	mov	bcd+4,r0
	jsr	pc,toyload		/ bytes 5 and 6
	mov	bcd+6,r0
	jsr	pc,toyload		/ bytes 7 and 8

	tst	(sp)+			/ clean stack now, we're done
	clr	r0			/ "exit" status.  ha! ;-)
	jsr	pc,_exit

/ Probe for the TOY register.  If present initialize the TOY and read the 
/ current date otherwise print an error message and exit.  Convert the date 
/ into printable form and print it out along with the prompt.

init:
	mov	$1f, nofault		/ catch fault if no TOY clock
	tst	*$TOYCSR
	br	2f			/ we have a clock
1:
	mov	$errmsg1,-(sp)
	jsr	pc,_printf
	mov	$1,r0
	jsr	pc,_exit
2:
	clr	nofault			/ faults are serious again
	jsr	pc,initoy		/ init the TOY clock
	mov	$bcd,-(sp)		/ buffer for the date
	jsr	pc,_gettoy		/ read the TOY
	tst	(sp)+
	mov	$timbuf,r3		/ where to put printable form of date
	clr	r1
	bisb	bcd+7,r1		/ year in bcd
	jsr	pc,bcd2msg
	movb	bcd+6,r1		/ month in bcd
	jsr	pc,bcd2msg
	movb	bcd+5,r1		/ day of month in bcd
	jsr	pc,bcd2msg
	movb	bcd+3,r1		/ hour of day in bcd
	jsr	pc,bcd2msg
	movb	bcd+2,r1		/ minute of hour in bcd
	jsr	pc,bcd2msg
	movb	$'.,(r3)+
	movb	bcd+1,r1		/ seconds of minute in bcd
	jsr	pc,bcd2msg
	mov	$timmsg,-(sp)
	jsr	pc,_printf
	tst	(sp)+
	rts	pc

/ convert two bytes of ascii pointed to by 2(sp) into a binary number.
/ the return value is in r0.

atoi2:
	movb	*2(sp),r1
	inc	2(sp)
	movb	*2(sp),r0
	sub	$'0,r1
	sub	$'0,r0
	mul	$10.,r1
	add	r1,r0
	rts	pc

/ convert a byte of BCD (in r1) two to ascii digits and place those
/ at (r3)+ and (r3)+ respectively.

bcd2msg:
	clr	r0
	div	$16.,r0
	add	$'0,r0
	add	$'0,r1
	movb	r0,(r3)+
	movb	r1,(r3)+
	rts	pc

/ convert a binary number (in r1) to a byte containing two bcd digits.
/ return result in r0.

tobcd:
	clr	r0
	div	$100.,r0		/ truncate to max of 99
	clr	r0
	div	$10.,r0
	ash	$4,r0
	bis	r1,r0
	rts	pc

/ To calculate the day of the week (Sunday = 1) an algorithm found in
/ "The Ready Reference (r) Weekly Planner 1986 ((c) 1986 Ready Reference)"
/ is used.

t2dow:
	jsr	r5,csv			/ save registers
	movb	year,r4			/ low two digits (93) of year
	mov	r4,r0
	asr	r0			/ divide by 4
	asr	r0			/  ignoring any remainder
	add	r0,r4			/ add to year
	movb	day,r0
	add	r0,r4			/ add day of month
	movb	month,r0
	movb	m_magic-1(r0),r3
	cmp	r0,$2			/ February Or January?
	bgt	1f			/ no - br
	movb	year,r0
	add	$1900.,r0
	mov	r0,-(sp)
	jsr	pc,isleap		/ are we in a leap year?
	mov	r0,(sp)+
	beq	1f			/ no - br
	dec	r3			/ adjust number from magic month table
1:
	add	r3,r4			/ add magic number to total
	cmpb	year,$69.		/ before 1969?
	bgt	2f			/ no - br, we're in the 20th century
	add	$6,r4			/ adjustment for 21st century
2:
	mov	r4,r1
	clr	r0
	div	$7,r0			/ divide total by 7
	dec	r1			/ Saturday or Sunday?
	bgt	3f			/ no - br
	add	$7,r1			/ yes - set Saturday to 6, Sunday to 7
3:
	mov	r1,r0			/ put return value in right place
	jmp	cret			/ 1 = Monday ... 7 = Sunday

/ (((y) % 4) == 0 && ((y) % 100) != 0 || ((y) % 400) == 0)

isleap:
	clr	r0
	mov	2(sp),r1		/ get year
	bit	$3,r1			/ easy check for "mod 4"
	bne	1f			/ can't be a leap - br
	div	$100.,r0		/ % 100
	tst	r1			/ check remainder
	bne	2f
	bit	$3,r0			/ % 400
	beq	2f
1:
	clr	r0
	rts	pc
2:
	mov	$1,r0
	rts	pc

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

	.data
Mtab:
	.byte	31.,29.,31.,30.,31.,30.
	.byte	31.,31.,30.,31.,30.,31.
m_magic:
	.byte	1,4,4,0,2,5,0,3,6,1,4,6
errmsg1:
	<No TOY clock present\n\0>
timmsg:
	<Current TOY: >
timbuf:
	.=.+13.				/ room for YYMMDDhhmm.ss
	<\n>
	<Toyset\> \0>
_module:
	<Toyset\0>
	.even
	.bss
seconds:
	.=.+1
minutes:
	.=.+1
hours:
	.=.+1
day:
	.=.+1
month:
	.=.+1
year:
	.=.+1
dow:
	.=.+1
	.even
bcd:
	.=.+8.
line:
	.=.+64.

/**********
/
/	boot rom for si 9500 disk drive
/
/**********

reset	= 5
nop	= 240

	<SI>				/ name code of boot rom
	0176				/ offset to next rom

	sec				/ entry point, unit 0, no diagnostics
	mov	$0,r0			/ entry point, unit 0, with diagnostics
	mov	$176700,r1		/ load csr into r1
	mov	pc,r4			/ for link
	bcc	diag			/ go do diagnostics if C set
	br	begin			/ skip next stuff

	0173000				/ pc on power up boot
	0340				/ psw on power up boot

begin:
	reset				/ reset unibus devices
	mov	r0,r3			/ move unit to r3
	bis	$01300,sp		/ get some stack

	mov	24(r1),r0		/ load shared computer register to r0
	bit	$200,r0			/ see if grant bit is set
	bne	1f			/ set, is dual ported
	mov	12(r1),r0		/ move error register to r0
	bic	$037777,r0		/ clear all but error and contention
	cmp	$140000,r0		/ see if contention error
	bne	2f			/ nope, must be normal 9500
1:
	bit	$200,24(r1)		/ test for grant
	bne	2f			/ got it
	clr	(r1)			/ issue login master clear to 9500
	mov	$1,24(r1)		/ set request
	br	1b			/ loop until grant
2:
	mov	$400,2(r1)		/ set for 512 bytes
	clr	6(r1)			/ set for head 0, sector 0
	clr	10(r1)			/ set to read at zero
	bic	$177774,r3		/ clear all but bottom bits
	ash	$12,r3			/ shift unit number
	mov	r3,4(r1)		/ set unit number
	mov	$5,(r1)			/ start the read
3:
	tstb	(r1)			/ test for done
	bpl	3b			/ wait for it
	tst	12(r1)			/ see if error occured
	bpl	4f			/ nope, all ok
	br	begin			/ try again
4:
	clr	pc			/ jump to zero
diag:	jmp	*$165564		/ go do diagnostics

	nop
	nop

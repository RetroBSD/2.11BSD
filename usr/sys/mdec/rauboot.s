MAJOR = 5			/ major # from bdevsw[]

/ RA bootstrap.
/
/ 1995/05/31 - The unit number needs to go in bits 3-5 of bootdev
/	       because the partition number now goes into bits 0-2.
/
/ disk boot program to load and transfer to a unix entry.
/ for use with 1 KB byte blocks, CLSIZE is 2.
/ NDIRIN is the number of direct inode addresses (currently 4)
/
/ Note: this is a complex boot, but then MSCP is complex!!!!
/
/ assembled size must be <= 512
/ a.out header must be removed from boot block!

MSCPSIZE =	64.	/ One MSCP command packet is 64bytes long (need 2)

RASEMAP	=	140000	/ RA controller owner semaphore

RAERR =		100000	/ error bit 
RASTEP1 =	04000	/ step1 has started
RAGO =		01	/ start operation, after init
RASTCON	= 	4	/ Setup controller info 
RAONLIN	=	11	/ Put unit on line
RAREAD =	41	/ Read command code
RAWRITE =	42	/ Write command code
RAEND =		200	/ End command code

RACMDI =	4.	/ Command Interrupt
RARSPI =	6.	/ Response Interrupt
RARING =	8.	/ Ring base
RARSPL =	8.	/ Response Command low
RARSPH = 	10.	/ Response Command high
RACMDL =	12.	/ Command to controller low
RACMDH =	14.	/ Command to controller high
RARSPS =	16.	/ Response packet length (location)
RARSPREF =	20.	/ Response reference number
RACMDS =	80.	/ Command packet length (location)
RACMDREF =	84.	/ Command reference number
RAUNIT = 	88.	/ Command packet unit 
RAOPCODE =	92.	/ Command opcode offset
RABYTECT =	96.	/ Transfer byte count
RABUFL =	100.	/ Buffer location (16 bit addressing only)
RABUFH = 	102.	/ Buffer location high 6 bits
RALBNL =	112.	/ Logical block number low
RALBNH = 	114.	/ Logical block number high

/ options: none.  all options of reading an alternate name or echoing to
/		  the keyboard had to be removed to make room for the 
/		  code which understands the new directory structure on disc
/		  also, this is the single largest boot around to begin with.

/ constants:
/
CLSIZE	= 2.			/ physical disk blocks per logical block
CLSHFT	= 1.			/ shift to multiply by CLSIZE
BSIZE	= 512.*CLSIZE		/ logical block size
INOSIZ	= 64.			/ size of inode in bytes
NDIRIN	= 4.			/ number of direct inode addresses
ADDROFF	= 12.			/ offset of first address in inode
INOPB	= BSIZE\/INOSIZ		/ inodes per logical block
INOFF	= 31.			/ inode offset = (INOPB * (SUPERB+1)) - 1
PBSHFT	= -4			/ shift to divide by inodes per block
/
/  The boot options and device are placed in the last SZFLAGS bytes
/  at the end of core by the kernel for autobooting.
/
ENDCORE=	160000		/ end of core, mem. management off
SZFLAGS=	6		/ size of boot flags
BOOTOPTS=	2		/ location of options, bytes below ENDCORE
BOOTDEV=	4	
CHECKWORD=	6

.. = ENDCORE-512.-SZFLAGS	/ save room for boot flags

/ entry is made by jsr pc,*$0
/ so return can be rts pc

/ establish sp, copy
/ program up to end of core.

	nop			/ These two lines must be present or DEC
	br	start		/ boot ROMs will refuse to run boot block!
start:
	mov	r0,unit		/ Save unit number passed by ROMs(and kernel)
	mov	r1,raip		/ save csr passed by ROMs (and kernel)
	mov	$..,sp
	mov	sp,r1
	clr	r0
1:
	mov	(r0)+,(r1)+
	cmp	r1,$end
	blo	1b
	jmp	*$2f

/ On error, restart from here.
restart:
	clr	r0
/ clear core to make things clean
2:
	clr	(r0)+
	cmp	r0,sp
	blo	2b
/
/ RA initialize controller
/
	mov	$RASTEP1,r0
	mov	raip,r1
	clr	(r1)+			/ go through controller init seq.
	mov	$icons,r2
1:
	bit	r0,(r1)
	beq	1b
	mov	(r2)+,(r1)
	asl	r0
	bpl	1b
	mov	$ra+RARSPREF,*$ra+RARSPL / set controller characteristics
	mov	$ra+RACMDREF,*$ra+RACMDL
	mov	$RASTCON,r0
	jsr	pc,racmd
	mov	unit,*$ra+RAUNIT	/ bring boot unit online
	mov	$RAONLIN,r0
	jsr	pc,racmd

	mov	$bootnm, r1
	mov	$2,r0			/ ROOTINO
	jsr	pc,iget
	clr	r2			/ offset
again:
	jsr	pc,readdir
	beq	restart			/ error - restart
	mov	4(r0),r4		/ dp->d_namlen
	cmp	r4,$bootlen		/ if (bootlen == dp->d_namlen)
	bne	again			/    nope, go try next entry
	mov	r0,r3
	add	$6,r3			/ r3 = dp->d_name
	mov	r1,r5			/ r5 = filename
9:
	cmpb	(r3)+,(r5)+
	bne	again			/ no match - go read next entry
	sob	r4,9b
	mov	(r0),r0			/ r0 = dp->d_ino
	jsr	pc,iget			/ fetch boot's inode
	br	loadfile		/ 'boot'- go read it

/ get the inode specified in r0
iget:
	add	$INOFF,r0
	mov	r0,r5
	ash	$PBSHFT,r0
	bic	$!7777,r0
	mov	r0,dno
	clr	r0
	jsr	pc,rblk
	bic	$!17,r5
	mov	$INOSIZ,r0
	mul	r0,r5
	add	$buf,r5
	mov	$inod,r4
1:
	movb	(r5)+,(r4)+
	sob	r0,1b
	rts	pc

readdir:
	bit	$BSIZE-1,r2
	bne	1f
	jsr	pc,rmblk		/ read mapped block (bno)
		br err			/ end of file branch
	clr	r2			/ start at beginning of buf
1:
	mov	$buf,r0
	add	r2,r0			/ dp = buf+offset
	add	buf+2(r2),r2		/ dp += dp->d_reclen
	tst	(r0)			/ dp->d_ino == 0?
	beq	readdir			/ yes - go look at next
	rts	pc			/ return with r0 = &dp->d_ino
err:
	clr	r0			/ return with
	rts	pc			/ dp = NULL

loadfile:
	clr	bno			/ start at block 0 of inode in 'inod'
/ read file into core until
/ a mapping error, (no disk address)
	clr	r1
1:
	jsr	pc,rmblk
		br 1f
	mov	$buf,r2
2:
	mov	(r2)+,(r1)+
	cmp	r2,$buf+BSIZE
	blo	2b
	br	1b
/ relocate core around
/ assembler header
1:
	clr	r0
	cmp	(r0),$407
	bne	2f
1:
	mov	20(r0),(r0)+
	cmp	r0,sp
	blo	1b
/ enter program and
/ restart if return
2:
	mov	ENDCORE-BOOTOPTS, r4
	mov	unit, r3
	ash	$3,r3			/ unit # in bits 3-5, partition # is 0
	bis	$MAJOR\<8.,r3
	mov	ENDCORE-CHECKWORD, r2
	mov	raip,r1
	jsr	pc,*$0
	jmp	restart

/ read a mapped block
/ offset in file is in bno.
/ skip if success, no skip if fail
/ the algorithm only handles a single
/ indirect block. that means that
/ files longer than NDIRIN+256 blocks (260kb) cannot
/ be loaded.
rmblk:
	add	$2,(sp)
	mov	bno,r0
	cmp	r0,$NDIRIN
	blt	1f
	mov	$NDIRIN,r0
1:
	ash	$2,r0
	mov	addr+2(r0),dno
	mov	addr(r0),r0
	bne	1f
	tst	dno
	beq	2f
1:
	jsr	pc,rblk
	mov	bno,r0
	inc	bno
	sub	$NDIRIN,r0
	blt	1f
	ash	$2,r0
	mov	buf+2(r0),dno
	mov	buf(r0),r0
	bne	rblk
	tst	dno
	bne	rblk
2:
	sub	$2,(sp)
1:
	rts	pc

read	= 4
go	= 1

/
/ RA MSCP read block routine.  This is very primative, so don't expect
/ too much from it.  Note that MSCP requires large data communications
/ space at end of ADDROFF for command area.
/ N.B.  This MUST preceed racmd - a "jsr/rts" sequence is saved by
/	falling thru!
/
/	dno	->	1k block # to load (low half)
/	buf	->	address of buffer to put block into
/	BSIZE	->	size of block to read
/ 
/ Tim Tucker, Gould Electronics, August 23rd 1985
/
rblk:
	mov	dno,r0
	asl	r0
	mov	r0,*$ra+RALBNL		/ Put in disk block number
	mov	$BSIZE,*$ra+RABYTECT	/ Put in byte to transfer
	mov	$buf,*$ra+RABUFL	/ Put in disk buffer location
	mov	$RAREAD,r0

/
/ perform MSCP command -> response poll version
/
racmd:
	movb	r0,*$ra+RAOPCODE	/ fill in command type
	mov	$MSCPSIZE,*$ra+RARSPS	/ give controller struct sizes
	mov	$MSCPSIZE,*$ra+RACMDS
	mov	$RASEMAP,*$ra+RARSPH	/ set mscp semaphores
	mov	$RASEMAP,*$ra+RACMDH
	mov	*raip,r0		/ tap controllers shoulder
	mov	$ra+RACMDI,r0
1:
	tst	(r0)
	beq	1b			/ Wait till command read
	clr	(r0)+			/ Tell controller we saw it, ok.
2:
	tst	(r0)
	beq	2b			/ Wait till response written
	clr	(r0)			/ Tell controller we go it
	rts	pc

icons:	RAERR
	ra+RARING
	0
	RAGO

bootnm:	<boot\0\0>
bootlen = 4			/ strlen(bootnm)
unit: 0				/ unit number from ROMs
raip: 0				/ csr address from ROMs
end:

inod = ..-512.-BSIZE		/ room for inod, buf, stack
addr = inod+ADDROFF		/ first address in inod
buf = inod+INOSIZ
bno = buf+BSIZE
dno = bno+2
ra = dno + 2			/ ra mscp communications area (BIG!)

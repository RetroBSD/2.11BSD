From osmail!egdorf@LANL.ARPA Tue Jan 28 14:10:11 1986
Received: by seismo.CSS.GOV; Tue, 28 Jan 86 14:09:00 EST
Received: by LANL.ARPA (4.12/4.7)
	id AA25151; Tue, 28 Jan 86 10:48:19 mst
Date: Tue, 28 Jan 86 10:48:19 mst
From: osmail!egdorf@LANL.ARPA
Message-Id: <8601281748.AA25151@LANL.ARPA>
To: lanl!keith
Subject: mdec/rauboot.s
Status: R

/ RA bootstrap.
/
/ disk boot program to load and transfer to a unix entry.
/ for use with 1 KB byte blocks, CLSIZE is 2.
/ NDIRIN is the number of direct inode addresses (currently 4)
/ assembled size must be <= 512; if > 494, the 16-byte a.out header
/ must be removed
/
/ Note: this is a complex boot, but then MSCP is complex!!!!
/
/ a.out header must be removed from boot block!
/

MSCPSIZE =	64.	/ One MSCP command packet is 64bytes long (need 2)

RASEMAP	=	100000	/ RA controller owner semaphore
RAFLAG =	040000	/ RA controller is done with descriptor.

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

raip	= 172150	/ initialization and polling register
rasa	= 172152	/ address and status register

cyl	= 0.		/ cylinder offset of filesys to read from
/
/ options:
/
autoboot= 0		/ 1->code for autoboot. 0->no autoboot, saves 12 bytes

mxvboot	= 1		/ 0->normal, 1->adds check done by MXV11 boot ROMS

unit	= 0		/ # of unit to load boot from
/
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
/
/  The boot options and device are placed in the last SZFLAGS bytes
/  at the end of core by the kernel if this is an autoboot.
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
.if 	mxvboot
	0240			/ These two lines must be present or DEC
	br	start		/ boot ROMs will refuse to run boot block!
.endif
start:
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

/ clear core to make things clean
2:
	clr	(r0)+
	cmp	r0,sp
	blo	2b
/
/ RA initialize controller
/
	mov	$RASTEP1,r0
	mov	$raip,r1
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
	mov	$unit,*$ra+RAUNIT	/ bring boot unit online
	mov	$RAONLIN,r0
	jsr	pc,racmd

/ spread out in array 'names', one
/ component every 14 bytes.
	mov	$names,r1
1:
	mov	r1,r2
2:
	jsr	pc,getc
	cmp	r0,$'\n
	beq	1f
	cmp	r0,$'/
	beq	3f
	movb	r0,(r2)+
	br	2b
3:
	cmp	r1,r2
	beq	2b
	add	$14.,r1
	br	1b

/ now start reading the inodes
/ starting at the root and
/ going through directories
1:
	mov	$names,r1
	mov	$2,r0
1:
	clr	bno
	jsr	pc,iget
	tst	(r1)
	beq	1f
2:
	jsr	pc,rmblk
		br restart
	mov	$buf,r2
3:
	mov	r1,r3
	mov	r2,r4
	add	$16.,r2
	tst	(r4)+
	beq	5f
4:
	cmpb	(r3)+,(r4)+
	bne	5f
	cmp	r4,r2
	blo	4b
	mov	-16.(r2),r0
	add	$14.,r1
	br	1b
5:
	cmp	r2,$buf+BSIZE
	blo	3b
	br	2b

/ read file into core until
/ a mapping error, (no disk address)
1:
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
.if	autoboot
	mov	ENDCORE-BOOTOPTS, r4
	mov	ENDCORE-BOOTDEV, r3
	mov	ENDCORE-CHECKWORD, r2
.endif
	jsr	pc,*$0
/	br	restart

/ get the inode specified in r0
iget:
	add	$INOFF,r0
	mov	r0,r5
	ash	$-4.,r0
	bic	$!7777,r0
	mov	r0,dno
	clr	r0
	jsr	pc,rblk
	bic	$!17,r5
	mul	$INOSIZ,r5
	add	$buf,r5
	mov	$inod,r4
1:
	mov	(r5)+,(r4)+
	cmp	r4,$inod+INOSIZ
	blo	1b
	rts	pc

/ read a mapped block
/ offset in file is in bno.
/ skip if success, no skip if fail
/ the algorithm only handles a single
/ indirect block. that means that
/ files longer than NDIRIN+128 blocks cannot
/ be loaded.
rmblk:
	add	$2,(sp)
	mov	bno,r0
	cmp	r0,$NDIRIN
	blt	1f
	mov	$NDIRIN,r0
1:
	mov	r0,-(sp)
	asl	r0
	add	(sp)+,r0
	add	$addr+1,r0
	movb	(r0)+,dno
	movb	(r0)+,dno+1
	movb	-3(r0),r0
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
	jsr	pc,racmd
	rts	pc

/
/ perform MSCP command -> response poll version
/
/
/
racmd:
	movb	r0,*$ra+RAOPCODE	/ fill in command type
	mov	$MSCPSIZE,r0
	mov	r0,*$ra+RARSPS		/ give controller struct sizes
	mov	r0,*$ra+RACMDS
	mov	$RASEMAP,r0
	mov	r0,*$ra+RARSPH		/ set mscp semaphores
	mov	r0,*$ra+RACMDH
	mov	raip,r0			/ tap controllers shoulder
	mov	$ra+RACMDH,r0
1:
	bit	(r0),$RAFLAG
	beq	1b			/ Wait till command read
	mov	$ra+RARSPH,r0
2:
	bit	(r0),$RAFLAG
	beq	2b			/ Wait till response written
	rts	pc
/
/ Read a character at a time from the boot string, no console input
/ supported because boot block too large!
/
getc:
	movb	*cp, r0
	beq	2f
	inc	cp
2:
	cmp	r0,$'\r
	bne	1f
	mov	$'\n,r0
1:
	rts	pc

icons:	RAERR
	ra+RARING
	0
	RAGO
cp:	defnm
defnm:	<boot\r\0>
end:

inod = ..-512.-BSIZE		/ room for inod, buf, stack
addr = inod+ADDROFF		/ first address in inod
buf = inod+INOSIZ
bno = buf+BSIZE
dno = bno+2
ra = dno+2			/ ra mscp communications area (BIG!)
names = ra+146.



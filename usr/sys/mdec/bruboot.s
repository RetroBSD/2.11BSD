MAJOR = 11.			/ major # from bdevsw[]

/ BR bootstrap. supports the 32 sec/trk, 19 trk/cyl EATON 1537/1711 
/	Controller and 1538A/B/C/D drive. 
/
/ 1995/05/31 - The unit number needs to go in bits 3-5 of bootdev
/	       because the partition number now goes into bits 0-2.
/
/ disk boot program to load and transfer
/ to a unix entry.
/ for use with 1 KB byte blocks, CLSIZE is 2.
/ NDIRIN is the number of direct inode addresses (currently 4)
/ assembled size must be <= 512; if > 494, the 16-byte a.out header
/ must be removed

/ options: none.  all options of reading an alternate name or echoing to
/		  the keyboard had to be removed to make room for the 
/		  code which understands the new directory structure on disc

/ constants:
CLSIZE	= 2.			/ physical disk blocks per logical block
CLSHFT	= 1.			/ shift to multiply by CLSIZE
BSIZE	= 512.*CLSIZE		/ logical block size
INOSIZ	= 64.			/ size of inode in bytes
NDIRIN	= 4.			/ number of direct inode addresses
ADDROFF	= 12.			/ offset of first address in inode
INOPB	= BSIZE\/INOSIZ		/ inodes per logical block
INOFF	= 31.			/ inode offset = (INOPB * (SUPERB+1)) - 1
PBSHFT	= -4			/ shift to divide by inodes per block
WC	= -256.*CLSIZE		/ word count

/  The boot options and device are placed in the last SZFLAGS bytes
/  at the end of core by the kernel for autobooting.
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
	clr	r0		/ XXX - for Rome 11/70, boot card doesn't work
	mov	$176714,r1	/ XXX - for Rome 11/70, boot card doesn't work
	movb	r0,unit+1	/ save the unit in high byte (for brcs)
	mov	r1,csr		/  and csr from the ROMs (not base addr!)
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

/ initialize controller
	mov	csr,r1
	mov	unit,brcs(r1)		/ clr addr extension bits, select unit
	mov	$32.,r0			/ default # of sec/track
	bit	$2400,brae(r1)		/ is this a 22 sec/track drive
	beq	9f
	mov	$22.,r0
9:
	mov	$19.,r5			/ default # of tracks/cyl
	bit	$1400,brae(r1)		/ is this a 5 tr/cy drive?
	beq	9f			/ no - br
	mov	$5,r5
9:
	mov	r0,sectors
	mul	r0,r5			/ sectors/cyl
	mov	r5,seccyl

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
	movb	unit+1,r3
	ash	$3,r3			/ unit # in bits 3-5, partition # is 0
	bis	$MAJOR\<8.,r3
	mov	ENDCORE-CHECKWORD, r2
	mov	csr,r1
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

brds	= -4
brer	= -2
brcs	= 0		/ offset from base csr passed by ROMs
brwc	= 2
brba	= 4
brca	= 6
brda	= 10
brae	= 12
/ br disk driver.
/ low order address in dno,
/ high order in r0.
rblk:
	mov	r1,-(sp)
	mov	dno,r1
.if	CLSIZE-1
	ashc	$CLSHFT,r0		/ multiply by CLSIZE
.endif
	div	seccyl,r0
	mov	csr,r3
	mov	r0,brca(r3)
	clr	r0
	div	sectors,r0
	swab	r0
	bis	r1,r0
	mov	r0,brda(r3)
	mov	$buf,brba(r3)
	mov	$WC,brwc(r3)
	mov	unit,r0
	bis	$read+go,r0
	mov	r0,(r3)		/ brcs
1:
	tstb	(r3)		/ brcs
	bge	1b
	mov	(sp)+,r1
	rts	pc

bootnm:	<boot\0\0>
bootlen = 4		/ strlen(bootnm)
unit: 0
csr: 0
seccyl: 0
sectors: 0
end:

inod = ..-512.-BSIZE		/ room for inod, buf, stack
addr = inod+ADDROFF		/ first address in inod
buf = inod+INOSIZ
bno = buf+BSIZE
dno = bno+2

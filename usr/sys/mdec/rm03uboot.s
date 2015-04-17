MAJOR = 10.			/ major # from bdevsw[]

/ RM02/03/05 bootstrap
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

RM05	= 0		/ 0-> RM02/03, 1-> RM05

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
	mov	r0,unit
	mov	r1,csr
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

/ initialize disk
	mov	csr,r1
	mov	unit,rmcs2(r1)
	mov	$preset+go,rmcs1(r1)
	mov	$fmt22,rmof(r1)

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
	mov	unit,r3
	ash	$3,r3			/ unit # in bits 3-5, partition # is 0
	bis	$MAJOR\<8.,r3
	mov	ENDCORE-CHECKWORD, r2
	mov	csr,r1
	jsr	pc,*$0
	br	restart

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

read	= 70
preset	= 20
go	= 1
fmt22	= 10000
NSECT	= 32.
.if	RM05
NTRAK	= 19.		/ RM05
.endif
.if	RM05-1
NTRAK	= 5.		/ RM02/03
.endif

rmcs1	= 0
rmda	= rmcs1+6
rmcs2	= rmcs1+10
rmds	= rmcs1+12
rmof	= rmcs1+32
rmca	= rmcs1+34

/ rm02/3/5 disk driver.
/ low order address in dno,
/ high order in r0.
rblk:
	mov	r1,-(sp)
	mov	dno,r1
.if	CLSIZE-1
	ashc	$CLSHFT,r0		/ multiply by CLSIZE
.endif
	div	$NSECT*NTRAK,r0
	mov	csr,r3
	mov	r0,rmca(r3)
	clr	r0
	div	$NSECT,r0
	swab	r0
	bis	r1,r0
	add	$rmcs2,r3
	mov	unit,(r3)
	mov	r0,-(r3)
	mov	$buf,-(r3)
	mov	$WC,-(r3)
	mov	$read+go,-(r3)
1:
	tstb	(r3)
	bge	1b
	mov	(sp)+,r1
	rts	pc

bootnm:	<boot\0\0>
bootlen = 4			/ strlen(bootnm)
unit: 0
csr: 0
end:

inod = ..-512.-BSIZE		/ room for inod, buf, stack
addr = inod+ADDROFF		/ first address in inod
buf = inod+INOSIZ
bno = buf+BSIZE
dno = bno+2

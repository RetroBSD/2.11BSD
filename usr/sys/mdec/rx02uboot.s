MAJOR = 8			/ major # from bdevsw[]

/ RX02 bootstrap.
/
/ 1995/12/02 - Now we have a boot for RX02!
/	Based on the RK05 bootstrap.  Only 8 bytes
/	to spare, though.  Should prove useful for those
/	who have only RX02 boot ROM's and use a MSCP
/	or RP/RM type system disk.
/	 -Tim(shoppa@altair.krl.caltech.edu)
/
/ disk boot program to load and transfer
/ to a unix entry.
/ for use with 1 KB byte blocks, CLSIZE is 4.
/ NDIRIN is the number of direct inode addresses (currently 4)
/ assembled size must be <= 512; if > 494, the 16-byte a.out header
/ must be removed

/ options: none.  all options of reading an alternate name or echoing to
/		  the keyboard had to be removed to make room for the 
/		  code which understands the new directory structure on disc

/ constants:
BSIZE	= 1024.			/ logical block size
DENS	= 1.			/ 1 for double density, 0 for single density
				/ This is the only place you should have
				/ to change to switch this bootstrap for
				/ some other density.  
WC	= 64.*[1.+DENS]		/ word count per floppy sector
BC	= 2.*WC			/ byte count per floppy sector
BCSHFT	= 7.+DENS		/ shift to multiply by BC
CLSIZE	= BSIZE\/BC		/ physical floppy sectors per logical block
CLSHFT	= 3.-DENS		/ shift to multiply by CLSIZE
CLMASK	= CLSIZE-1.		/ mask for getting CLSHFT bits

INOSIZ	= 64.			/ size of inode in bytes
NDIRIN	= 4.			/ number of direct inode addresses
ADDROFF	= 12.			/ offset of first address in inode
INOPB	= BSIZE\/INOSIZ		/ inodes per logical block
INOFF	= 31.			/ inode offset = (INOPB * (SUPERB+1)) - 1
PBSHFT	= -4			/ shift to divide by inodes per block

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
	bis	$[MAJOR\<8.+DENS],r3	/ Density bit is partition #.
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

dmask	= DENS*0400
read	= dmask + 7  /density, read function, and go
empty	= dmask + 3  /density, empty function, and go

/ rx02 single and double density read block routine.
/ low order address in dno.

rblk:
	mov	r1,-(sp)
	mov	r2,-(sp)
	mov	r3,-(sp)
	mov	r4,-(sp)
	mov	r5,-(sp)	/we need a lot of registers for interleave
				/ calculations!  Can certainly be improved on!
	mov	dno,r5		/count up the CLSIZE physical sectors in here.
				/will use lowest CLSHFT bits to look for done
				/and to compute position in buffer
.if	CLSIZE-1
	ash	$CLSHFT,r5		/ multiply by CLSIZE
.endif
8:	mov	r5,r1
	clr	r0
	div	$26.,r0
	mov	r0,r4
	asl	r1
	cmp	$26.,r1
	bgt	1f
	inc	r1
1:	mov	r1,r3
	mov	r4,r1
	mul	$6,r1
	add	r3,r1
	clr	r0
	div	$26.,r0
	inc	r1  /physical sector
	inc	r4
	cmp	$77.,r4
	bgt	3f
	clr	r4
3:			/physical track now in r4
	mov	unit,r0
	ash	$4.,r0
	bis	$read,r0
	mov	csr,r3
	mov	r3,r2
	mov	r0,(r2)+	/now r3 is csr, r2 is db
1:	tstb	(r3)
	bpl	1b
	mov	r1,(r2)
1:	tstb	(r3)
	bpl	1b
	mov	r4,(r2)
1:	bit	(r3),$040
	beq 	1b
	mov	$empty,(r3)
1:	tstb	(r3)
	bpl	1b
	mov	$WC,(r2)
1:	tstb	(r3)
	bpl 	1b
	mov	r5,r0
	bic	$!CLMASK,r0	/lowest bits of r5 had the section of buffer
	ash	$BCSHFT,r0
	add	$buf,r0		
	mov	r0,(r2)
1:	bit	(r3),$040
	beq	1b
	inc	r5
	bit	r5,$CLMASK
	bne	8b
	mov	(sp)+,r5
	mov	(sp)+,r4
	mov	(sp)+,r3
	mov	(sp)+,r2
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

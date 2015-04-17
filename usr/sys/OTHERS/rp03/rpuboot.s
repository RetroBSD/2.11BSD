/ RP03 bootstrap.
/
/ disk boot program to load and transfer
/ to a unix entry.
/ for use with 1 KB byte blocks, CLSIZE is 2.
/ NDIRIN is the number of direct inode addresses (currently 4)
/ assembled size must be <= 512; if > 494, the 16-byte a.out header
/ must be removed

cyl	= 0.		/ cylinder offset of filesys to read from

/ options:
readname= 1		/ 1->normal, if default not found, read name
			/   from console. 0->loop on failure, saves 36 bytes
prompt	= 1		/ 1->prompt ('>') before reading from console
			/   0-> no prompt, saves 8 bytes

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
	clr	r0
2:
	clr	(r0)+
	cmp	r0,sp
	blo	2b

/ initialize rp
	clr	*$rpcs		/ selects drive zero

/ at origin, read pathname
.if	prompt
	mov	$'>, r0
	jsr	pc, putc
.endif

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
	mov	ENDCORE-BOOTOPTS, r4
	mov	ENDCORE-BOOTDEV, r3
	mov	ENDCORE-CHECKWORD, r2
	jsr	pc,*$0
	br	restart

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

rpcs	= 176710
rpda	= 176724
rpca	= 176722
rpba	= 176720
/ rp03 disk driver.
/ low order address in dno,
/ high order in r0.
rblk:
	mov	r1,-(sp)
	mov	dno,r1
.if	CLSIZE-1
	ashc	$CLSHFT,r0		/ multiply by CLSIZE
.endif
	div	$20.*10.,r0
.if	cyl
	add	$cyl,r0
.endif
	mov	r0,*$rpca
	clr	r0
	div	$10.,r0
	swab	r0
	bis	r1,r0
	mov	r0,*$rpda
	mov	$rpba,r1
	mov	$buf,(r1)
	mov	$WC,-(r1)
	mov	$read+go,-(r1)
1:
	tstb	(r1)
	bge	1b
	mov	(sp)+,r1
	rts	pc

tks = 177560
tkb = 177562
/ read and echo a teletype character
/ if *cp is nonzero, it is the next char to simulate typing
/ after the defnm is tried once, read a name from the console
getc:
	movb	*cp, r0
	beq	2f
	inc	cp
.if	readname
	br	putc
2:
	mov	$tks,r0
	inc	(r0)
1:
	tstb	(r0)
	bge	1b
	mov	tkb,r0
	bic	$!177,r0
	cmp	r0,$'A
	blo	2f
	cmp	r0,$'Z
	bhi	2f
	add	$'a-'A,r0
.endif
2:

tps = 177564
tpb = 177566
/ print a teletype character
putc:
	tstb	*$tps
	bge	putc
	mov	r0,*$tpb
	cmp	r0,$'\r
	bne	1f
	mov	$'\n,r0
	br	putc
1:
	rts	pc

cp:	defnm
defnm:	<boot\r\0>
end:

inod = ..-512.-BSIZE		/ room for inod, buf, stack
addr = inod+ADDROFF		/ first address in inod
buf = inod+INOSIZ
bno = buf+BSIZE
dno = bno+2
names = dno+2

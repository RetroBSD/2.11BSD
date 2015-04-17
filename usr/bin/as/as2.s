/ Sept 10, 1997 - fix coredump caused by using wrong error reporting
/	calling convention in three places.

	.globl	_signal, _close, _lseek, _unlink, _umask, _chmod, __exit
	.globl	_write, _read, _brk, _end, _open, _realloc, _fchmod
	.globl	pass1, hshsiz, outmod, dot, dotdot, error
	.globl	checkeos, curfb, savdot, ch, line, savop, inbuf, errflg
	.globl	fbptr, fbtbl, symnum, hshtab, symblk, symleft, dotrel
	.globl	symtab, aexit, overlaid, defund, a.outp, passno, filerr
	.globl	wrterr, argb, curfb, nxtfb, usymtab
	.globl	fin, fout, a.tmp1, ibufc, ibufp, obufp, outbuf, symbol
	.globl	PSTENTSZ, SYMENTSZ, SYMBLKSZ, Newsym

pass1:
	mov	fout,fin		/ tmp file is now input file
	mov	$666,-(sp)		/ mode
	mov	$3001 ,-(sp)		/ O_WRONLY|O_CREAT|O_TRUNC
	mov	a.outp,-(sp)		/ filename
	jsr	pc,_open
	add	$6,sp
	mov	r0,fout			/ file descriptor any good?
	bpl	1f			/ yes - br
	mov	a.outp,-(sp)
	jsr	pc,filerr
1:

/ 'symnum' has number of symbols.  The hashtable from pass 0 is no
/ longer needed - we can reuse it directly for 'usymtab' if there are less 
/ than 'hshsiz' symbols.  If there are more than 'hshsiz' (currently
/ 1500) symbols we have to realloc.
/
/ The 'fb' table (usually 0 long) is appended to the 'usymtab' (4 byte
/ entries per symbol).

	mov	fbptr,r0
	sub	fbtbl,r0		/ # bytes in 'fb' table
	asr	r0			/ convert to number of words
	add	symnum,r0		/ add in number of symbols twice
	add	symnum,r0		/ because we need 2 words per symbol
	inc	r0			/ one more for terminator word
	cmp	r0,$hshsiz		/ is hashtable big enough already?
	blo	1f			/ yes -br
	asl	r0			/ convert to bytes
	mov	r0,-(sp)
	mov	hshtab,-(sp)
	jsr	pc,_realloc		/ hshtab = realloc(hshtab, r0)
	mov	r0,hshtab
	bne	1f
	iot				/ should never happen
1:
	mov	hshtab,r1
	mov	usymtab,r2
9:
	mov	r2,symblk		/ save ptr to start of block
	tst	(r2)+			/ skip link word
	mov	$SYMBLKSZ,symleft	/ init amount left in block
1:
	tst	(r2)			/ end of symbol table block
	beq	4f			/ yes - br
	add	$8.,symsiz		/ size of symbol table
	tst	Newsym			/ are we doing new style?
	bne	8f			/ yes - br
	add	$4,symsiz		/ no, symbol table entries are bigger
8:
	mov	2(r2),r0		/ flags word
	bic	$!37,r0
	cmp	r0,$2			/text
	blo	2f
	cmp	r0,$3			/data
	bhi	2f
	add	$31,r0			/mark "estimated"
	mov	r0,(r1)+		/ store flags word
	mov	4(r2),(r1)+		/ copy value word
	br	3f
2:
	clr	(r1)+
	clr	(r1)+
3:
	add	$SYMENTSZ,r2		/ skip to next symbol entry
	sub	$SYMENTSZ,symleft	/ one symbol less in block
	cmp	symleft,$SYMENTSZ	/ room for another symbol?
	bge	1b			/ yes - br
4:
	mov	*symblk,r2		/ follow link to next block
	bne	9b			/ if not at end
1:

/ The 'fb' table needs to be appended to the 'usymtab' table now

	mov	fbtbl,r0
	mov	r1,fbbufp		/ save start of 'fb' table
1:
	cmp	r0,fbptr		/ at end of table?
	bhis	2f			/ yes - br
	mov	(r0)+,r4
	add	$31,r4			/ "estimated"
	mov	r4,(r1)+
	mov	(r0)+,(r1)+
	br	1b
2:
	mov	r1,endtable
	mov	$100000,(r1)+

	mov	$savdot,r0		/ reset the 'psect' (text,data,bss)
	clr	(r0)+			/ counters for the next pass
	clr	(r0)+
	clr	(r0)+
	jsr	pc,setup		/ init fb stuff

	jsr	pc,pass1_2		/ do pass 1

/ prepare for pass 2
	inc	passno
	cmp	outmod,$777
	beq	1f
	jsr	pc,aexit
/ not reached
1:
	jsr	pc,setup
	inc	bsssiz
	bic	$1,bsssiz
	mov	txtsiz,r1
	inc	r1
	bic	$1,r1
	mov	r1,txtsiz
	mov	datsiz,r2
	inc	r2
	bic	$1,r2
	mov	r2,datsiz
	mov	r1,r3
	mov	r3,datbase		/ txtsiz
	mov	r3,savdot+2
	add	r2,r3
	mov	r3,bssbase		/ txtsiz+datsiz
	mov	r3,savdot+4
	clr	r0
	asl	r3
	adc	r0
	add	$20,r3
	adc	r0
	mov	r3,symseek+2		/ 2*txtsiz+2*datsiz+20
	mov	r0,symseek
	sub	r2,r3
	sbc	r0
	mov	r3,drelseek+2		/ 2*txtsiz+datsiz
	mov	r0,drelseek
	sub	r1,r3
	sbc	r0
	mov	r3,trelseek+2		/ txtsiz+datsiz+20
	mov	r0,trelseek
	sub	r2,r3
	sbc	r0
	mov	r0,datseek
	mov	r3,datseek+2		/ txtsiz+20
	mov	hshtab,r1
1:
	jsr	pc,doreloc
	add	$4,r1
	cmp	r1,endtable
	blo	1b
	clr	r0
	clr	r1
	mov	$txtp,-(sp)
	jsr	pc,oset
	mov	trelseek,r0
	mov	trelseek+2,r1
	mov	$relp,-(sp)
	jsr	pc,oset
	mov	$8.,r2
	mov	$txtmagic,r1
1:
	mov	(r1)+,r0
	mov	$txtp,-(sp)
	jsr	pc,putw
	sob	r2,1b

	jsr	pc,pass1_2			/ do pass 2

/polish off text and relocation

	mov	$txtp,-(sp)
	jsr	pc,flush
	mov	$relp,-(sp)
	jsr	pc,flush

/ append full symbol table
	mov	symseek,r0
	mov	symseek+2,r1
	mov	$txtp,-(sp)
	jsr	pc,oset

	mov	usymtab,r2		/ pointer to first symbol block
	mov	hshtab,r1		/ 'type' and 'value' array

	tst	Newsym
	beq	8f
	jsr	pc,nsymout
	br	9f
8:
	jsr	pc,osymout
9:
	mov	$txtp,-(sp)
	jsr	pc,flush
	jsr	pc,aexit
/ not reached

saexit:
	mov	pc,errflg

aexit:
	mov	$a.tmp1,-(sp)		/ unlink(a.tmp1)
	jsr	pc,_unlink
	tst	errflg
	bne	2f

	clr	(sp)
	jsr	pc,_umask

	bic	r0,outmod		/ fchmod(fout, outmod&umask(0))
	mov	outmod,(sp)
	mov	fout,-(sp)
	jsr	pc,_fchmod
	tst	(sp)+
	clr	(sp)
	br	1f
2:
	mov	$2,(sp)
1:
	jsr	pc,__exit		/ _exit(errflg ? 2 : 0)

filerr:
	mov	2(sp),r0		/ filename string.  no need to clean
	tst	-(sp)			/ stack, this routine goes to saexit.
	mov	r0,-(sp)
	mov	$1,-(sp)
1:
	tstb	(r0)+
	bne	1b
	sub	2(sp),r0
	dec	r0
	mov	r0,4(sp)
	jsr	pc,_write
	add	$6,sp

	mov	$2,-(sp)		/ write(1, "?\n", 2)
	mov	$qnl,-(sp)
	mov	$1,-(sp)
	jsr	pc,_write
	add	$6,sp
	tst	passno
	bpl	saexit
	rts	pc

osymout:
9:
	mov	r2,symblk		/ save ptr to current sym block
	tst	(r2)+			/ skip link word
	mov	$SYMBLKSZ,symleft	/ space left in symbol block
1:
	mov	(r2),r4			/ pointer to symbol name
	beq	4f			/ end of block - br

	mov	$8.,r5			/ max number to copy
	mov	$symbol,r0		/ destination buffer
2:
	movb	(r4),(r0)+		/ copy a byte
	beq	6f
	inc	r4			/ non null - bump source ptr
6:
	sob	r5,2b

/ Now put four words of symbol name to the object file
	mov	$4,r5			/ number of words to do
	mov	$symbol,r4
6:
	mov	(r4)+,r0		/ word (2 chars) of symbol name
	mov	$txtp,-(sp)
	jsr	pc,putw
	sob	r5,6b

/ values from 'hshtab' (parallel array to symbol table) are retrieved now,
/ they take the place of the flags and value entries of the symbol table.
	mov	(r1)+,r0
	mov	$txtp,-(sp)
	jsr	pc,putw
	mov	(r1)+,r0
	mov	$txtp,-(sp)
	jsr	pc,putw
	add	$SYMENTSZ,r2		/ skip to next symbol
	sub	$SYMENTSZ,symleft	/ one less symbol in block
	cmp	symleft,$SYMENTSZ	/ room for another?
	bge	1b			/ yes - br
4:
	mov	*symblk,r2		/ no, follow link to next block
	bne	9b			/ unless it's end of list
	rts	pc

nsymout:
	clr	totalsz
	mov	$4,totalsz+2		/ string table min size is 4
9:
	mov	r2,symblk		/ save ptr to current symbol block
	tst	(r2)+			/ skip link word
	mov	$SYMBLKSZ,symleft	/ amount of space left in block
1:
	mov	(r2),r4			/ pointer to symbol's string
	beq	4f			/ end of block - br
	mov	totalsz,r0		/ now output the...
	mov	$txtp,-(sp)		/ high order of the string index...
	jsr	pc,putw			/ to the object file
	mov	totalsz+2,r0
	mov	$txtp,-(sp)
	jsr	pc,putw			/ and the low order word
2:
	tstb	(r4)+			/ find the end of the string
	bne	2b
	sub	(r2),r4			/ compute length including the null
	add	r4,totalsz+2		/ offset of next string
	adc	totalsz
	mov	(r1)+,r0		/ 'type' word of symbol
	mov	$txtp,-(sp)
	jsr	pc,putw
	mov	(r1)+,r0		/ 'value' word of symbol
	mov	$txtp,-(sp)
	jsr	pc,putw
	add	$SYMENTSZ,r2		/ advance to next symbol
	sub	$SYMENTSZ,symleft	/ adjust amount left in symbol block
	cmp	symleft,$SYMENTSZ	/ is there enough for another symbol?
	bge	1b			/ yes - br
4:
	mov	*symblk,r2		/ follow link to next symbol block
	bne	9b			/ more - br
	mov	totalsz,r0		/ now output the string table length
	mov	$txtp,-(sp)		/ high order word first
	jsr	pc,putw
	mov	totalsz+2,r0		/ followed by the low order
	mov	$txtp,-(sp)
	jsr	pc,putw

/ Now write the strings out

	mov	usymtab,r2		/ start at beginning of symbols
9:
	mov	r2,symblk		/ save pointer to current block
	tst	(r2)+			/ skip link word
	mov	$SYMBLKSZ,symleft	/ amount left in block
1:
	mov	(r2),r4			/ pointer to symbol's string
	beq	4f			/ at end of block - br
	jsr	pc,putstring		/ write out the string
	add	$SYMENTSZ,r2		/ advance to next symbol
	sub	$SYMENTSZ,symleft	/ adjust amount left in block
	cmp	symleft,$SYMENTSZ	/ enough for another symbol?
	bge	1b			/ yes - br
4:
	mov	*symblk,r2		/ move to next block of symbols
	bne	9b			/ any left - br

/ probably not necessary but let us leave the file size on an even
/ byte boundary.

	bit	$1,totalsz+2		/ odd number of bytes in string table?
	beq	5f			/ no - br
	mov	symblk,r4		/ we know 'symblk' points to a null
	jsr	pc,putstring		/ output a single null
5:
	rts	pc

/ R4 has the address of a null terminated string to write to the output
/ file.  The terminating null is included in the output.  This routine
/ "inlines" the 'txtp seek structure' manipulation because the 'putw'
/ routine was 1) not suitable to byte output and 2) symbol strings are
/ only written using the 'txtp' (as opposed to 'relp' - relocation info)
/ structure.

putstring:
	cmp	txtp,txtp+2		/ room for another byte?
	bhis	1f			/ no - br
3:
	movb	(r4),*txtp		/ put byte in buffer
	inc	txtp			/ advance output position
	tstb	(r4)+			/ did we just do the null?
	bne	putstring		/ no - go again
	rts	pc			/ yes - we're done, return
1:
	mov	r2,-(sp)		/ save r2 from being destroyed
	mov	$txtp,-(sp)		/ flush buffered output and...
	jsr	pc,flush		/ reset the pointers
	mov	(sp)+,r2		/ restore symbol pointer
	br	3b			/ go output a byte

doreloc:
	movb	(r1),r0
	bne	1f
	bisb	defund,(r1)
1:
	bic	$!37,r0
	cmp	r0,$5
	bhis	1f
	cmp	r0,$3
	blo	1f
	beq	2f
	add	bssbase,2(r1)
	rts	pc
2:
	add	datbase,2(r1)
1:
	rts	pc

setup:
	clr	dot
	mov	$2,dotrel
	mov	$..,dotdot
	clr	brtabp

	mov	$curfb,r4
1:
	clr	(r4)+
	cmp	r4,$curfb+40.
	blo	1b
	clr	r4
1:
	jsr	pc,fbadv
	inc	r4
	cmp	r4,$10.
	blt	1b
/ just rewind /tmp/atm1xx rather than close and re-open
	clr	-(sp)
	clr	-(sp)
	clr	-(sp)
	mov	fin,-(sp)
	jsr	pc,_lseek		/ lseek(fin, 0L, 0)
	add	$8.,sp
	clr	ibufc
	rts	pc

outw:
	cmp	dot-2,$4
	beq	9f
	bit	$1,dot
	bne	1f
	add	$2,dot
	tst	passno
	beq	8f
	clr	-(sp)
	rol	r3
	adc	(sp)
	asr	r3			/ get relative pc bit
	cmp	r3,$40
	bne	2f
/ external references
	mov	$666,outmod		/ make nonexecutable
	mov	xsymbol,r3
	sub	hshtab,r3
	asl	r3
	bis	$4,r3			/ external relocation
	br	3f
2:
	bic	$40,r3			/ clear any ext bits
	cmp	r3,$5
	blo	4f
	cmp	r3,$33			/ est. text, data
	beq	6f
	cmp	r3,$34
	bne	7f
6:
	mov	$'r,-(sp)
	jsr	pc,error
7:
	mov	$1,r3			/ make absolute
4:
	cmp	r3,$2
	blo	5f
	cmp	r3,$4
	bhi	5f
	tst	(sp)
	bne	4f
	add	dotdot,r2
	br	4f
5:
	tst	(sp)
	beq	4f
	sub	dotdot,r2
4:
	dec	r3
	bpl	3f
	clr	r3
3:
	asl	r3
	bis	(sp)+,r3
	mov	r2,r0
	mov	$txtp,-(sp)
	jsr	pc,putw
	mov	tseekp,r0
	add	$2,2(r0)
	adc	(r0)
	mov	r3,r0
	mov	$relp,-(sp)
	jsr	pc,putw
	mov	rseekp,r0
	add	$2,2(r0)
	adc	(r0)
8:
	rts	pc
1:
	mov	$'o,-(sp)
	jsr	pc,error
	clr	r3
	jsr	pc,outb
	rts	pc
9:
	mov	$'x,-(sp)
	jsr	pc,error
	rts	pc

outb:
	cmp	dot-2,$4		/ test bss mode
	beq	9b
	cmp	r3,$1
	blos	1f
	mov	$'r,-(sp)
	jsr	pc,error
1:
	tst	passno
	beq	2f
	mov	r2,r0
	bit	$1,dot
	bne	1f
	mov	$txtp,-(sp)
	jsr	pc,putw
	clr	r0
	mov	$relp,-(sp)
	jsr	pc,putw
	mov	tseekp,r0
	add	$2,2(r0)
	adc	(r0)
	mov	rseekp,r0
	add	$2,2(r0)
	adc	(r0)
	br	2f
1:
	mov	txtp,r0
	movb	r2,-1(r0)
2:
	inc	dot
	rts	pc

/ pass 1 and 2 common code

pass1_2:
	jsr	pc,readop
	cmp	r4,$5
	beq	2f
	cmp	r4,$'<
	beq	2f
	jsr	pc,checkeos
		br eal1
	mov	r4,-(sp)
	cmp	(sp),$1
	bne	1f
	mov	$2,(sp)
	jsr	pc,getw
	mov	r4,numval
1:
	jsr	pc,readop
	cmp	r4,$'=
	beq	4f
	cmp	r4,$':
	beq	1f
	mov	r4,savop
	mov	(sp)+,r4
2:
	jsr	pc,opline
dotmax:
	tst	passno
	bne	eal1
	movb	dotrel,r0
	asl	r0
	cmp	dot,txtsiz-4(r0)
	bhi	8f
	jmp	ealoop
8:
	mov	dot,txtsiz-4(r0)
eal1:
	jmp	ealoop
1:
	mov	(sp)+,r4
	cmp	r4,$200
	bhis	1f
	cmp	r4,$2
	beq	3f
	mov	$'x,-(sp)
	jsr	pc,error
	br	pass1_2
1:
	tst	passno
	bne	2f
	movb	(r4),r0
	bic	$!37,r0
	beq	5f
	cmp	r0,$33
	blt	6f
	cmp	r0,$34
	ble	5f
6:
	mov	$'m,-(sp)
	jsr	pc,error
5:
	bic	$37,(r4)
	bis	dotrel,(r4)
	mov	2(r4),brdelt
	sub	dot,brdelt
	mov	dot,2(r4)
	br	pass1_2
2:
	cmp	dot,2(r4)
	beq	pass1_2
	mov	$'p,-(sp)
	jsr	pc,error
	br	pass1_2
3:
	mov	numval,r4
	jsr	pc,fbadv
	asl	r4
	mov	curfb(r4),r0
	movb	dotrel,(r0)
	mov	2(r0),brdelt
	sub	dot,brdelt
	mov	dot,2(r0)
	br	pass1_2
4:
	jsr	pc,readop
	jsr	pc,expres
	mov	(sp)+,r1
	cmp	r1,$dotrel		/test for dot
	bne	1f
	bic	$40,r3
	cmp	r3,dotrel		/ can't change relocation
	bne	2f
	cmp	r3,$4			/ bss
	bne	3f
	mov	r2,dot
	br	dotmax
3:
	sub	dot,r2
	bmi	2f
	mov	r2,-(sp)
3:
	dec	(sp)
	bmi	3f
	clr	r2
	mov	$1,r3
	jsr	pc,outb
	br	3b
3:
	tst	(sp)+
	br	dotmax
2:
	mov	$'.,-(sp)
	jsr	pc,error
	br	ealoop
1:
	cmp	r3,$40
	bne	1f
	mov	$'r,-(sp)
	jsr	pc,error
1:
	bic	$37,(r1)
	bic	$!37,r3
	bne	1f
	clr	r2
1:
	bisb	r3,(r1)
	mov	r2,2(r1)

ealoop:
	cmp	r4,$'\n
	beq	1f
	cmp	r4,$'\e
	bne	9f
	rts	pc
1:
	inc	line
9:
	jmp	pass1_2

checkeos:
	cmp	r4,$'\n
	beq	1f
	cmp	r4,$';
	beq	1f
	cmp	r4,$'\e
	beq	1f
	add	$2,(sp)
1:
	rts	pc

fbadv:
	asl	r4
	mov	nxtfb(r4),r1
	mov	r1,curfb(r4)
	bne	1f
	mov	fbbufp,r1
	br	2f
1:
	add	$4,r1
2:
	cmpb	1(r1),r4
	beq	1f
	tst	(r1)
	bpl	1b
1:
	mov	r1,nxtfb(r4)
	asr	r4
	rts	pc

oset:
	mov	r2,-(sp)
	mov	r3,-(sp)
	mov	6(sp),r3
	mov	r1,r2
	bic	$!1777,r1
	add	r3,r1
	add	$8,r1
	mov	r1,(r3)+		/ next slot
	mov	r3,r1
	add	$2006,r1
	mov	r1,(r3)+		/ buf max
	mov	r0,(r3)+
	mov	r2,(r3)+		/ seek addr
	mov	(sp)+,r3
	mov	(sp)+,r2
	mov	(sp)+,(sp)
	rts	pc

putw:
	mov	r1,-(sp)
	mov	r2,-(sp)
	mov	6(sp),r2
	mov	(r2)+,r1		/ slot
	cmp	r1,(r2)			/ buf max
	bhis	1f
	mov	r0,(r1)+
	mov	r1,-(r2)
	br	2f
1:
	tst	(r2)+
	mov	r0,-(sp)
	jsr	pc,flush1
	mov	(sp)+,r0
	mov	r0,*(r2)+
	add	$2,-(r2)
2:
	mov	(sp)+,r2
	mov	(sp)+,r1
	mov	(sp)+,(sp)
ret:
	rts	pc

flush:
	mov	2(sp),r2
	mov	(sp)+,(sp)
	cmp	(r2)+,(r2)+
flush1:

	clr	-(sp)			/ lseek(fout, (r2)L+, L_SET)
	mov	2(r2),-(sp)
	mov	(r2)+,-(sp)
	tst	(r2)+
	mov	fout,-(sp)
	jsr	pc,_lseek
	add	$8.,sp

	cmp	-(sp),-(sp)		/ write(fout, <buf>, <len>)
	bic	$!1777,r1
	add	r2,r1			/ write address
	mov	r1,-(sp)		/ { <buf> }
	mov	r2,r0
	bis	$1777,-(r2)
	add	$1,(r2)			/ new seek addr
	adc	-(r2)
	cmp	-(r2),-(r2)
	sub	(r2),r1
	neg	r1
	mov	r1,2(sp)		/ count
	mov	r0,(r2)			/ new next slot

	mov	fout,-(sp)
	mov	r1,6(sp)		/ protect r1 from library
	jsr	pc,_write
	add	$6,sp
	mov	(sp)+,r1
	tst	r0
	bpl	ret
/ fall thru to wrterr

wrterr:
	mov	$8f-9f,-(sp)		/ write(1, 9f, 8f-9f)
	mov	$9f,-(sp)
	mov	$1,-(sp)
	jsr	pc,_write
	add	$6,sp
	jsr	pc,saexit
/ not reached

	.data
9:
	<as: write error\n>
8:
	.even
	.text

readop:
	mov	savop,r4
	beq	1f
	clr	savop
	rts	pc
1:
	jsr	pc,getw1
	cmp	r4,$200
	blo	1f
	cmp	r4,$4000
	blo	2f
	sub	$4000,r4
	asl	r4
	asl	r4
	add	hshtab,r4
	rts	pc
2:
/ remove PST flag (1000) then multiply by PSTENTSZ.  In pass 0 the PST
/ symbol number was divided by PSTENTSZ(to make it fit) - we now reverse
/ that process.
	mov	r5,-(sp)
	mov	r4,r5
	sub	$1000,r5
	mul	$PSTENTSZ,r5
	mov	r5,r4
	mov	(sp)+,r5
	add	$dotrel,r4		/ point at dot's flag field
1:
	rts	pc

getw:
	mov	savop,r4
	beq	getw1
	clr	savop
	rts	pc
getw1:
	dec	ibufc
	bgt	1f

	mov	r1,-(sp)		/ protect r1 from library
	mov	$1024.,-(sp)		/ read(fin, inbuf, 1024)
	mov	$inbuf,-(sp)
	mov	fin,-(sp)
	jsr	pc,_read
	add	$6,sp
	mov	(sp)+,r1
	asr	r0
	mov	r0,ibufc
	bgt	2f
	mov	$4,r4
	sev
	rts	pc
2:
	mov	$inbuf,ibufp
1:
	mov	*ibufp,r4
	add	$2,ibufp
	rts	pc

opline:
	mov	r4,r0
	bmi	2f
	cmp	r0,$177
	bgt	2f
	cmp	r4,$5
	beq	opeof
	cmp	r4,$'<
	bne	xpr
	jmp	opl17
xxpr:
	tst	(sp)+
xpr:
	jsr	pc,expres
	jsr	pc,outw
	rts	pc
2:
	movb	(r4),r0
	cmp	r0,$24			/reg
	beq	xpr
	cmp	r0,$33			/est text
	beq	xpr
	cmp	r0,$34			/ est data
	beq	xpr
	cmp	r0,$5
	blt	xpr
	cmp	r0,$36
	bgt	xpr
	mov	2(r4),-(sp)
	mov	r0,-(sp)
	jsr	pc,readop
	mov	(sp)+,r0
	asl	r0
	mov	$adrbuf,r5
	clr	swapf
	mov	$-1,rlimit
	jmp	*1f-10.(r0)
	.data
1:
	opl5
	opl6
	opl7
	opl10
	opl11
	opl12
	opl13
	opl14
	opl15
	opl16
	opl17
	opl20
	opl21
	opl22
	opl23
	xxpr
	opl25
	opl26
	opl27
	opl30
	opl31
	opl32
	xxpr
	xxpr
	opl35
	opl36
	.text

opeof:
	mov	$1,line
	mov	$20,-(sp)
	mov	$argb,r1
1:
	jsr	pc,getw
	tst	r4
	bmi	1f
	movb	r4,(r1)+
	dec	(sp)
	bgt	1b
	tstb	-(r1)
	br	1b
1:
	movb	$'\n,(r1)+
	clrb	(r1)+
	tst	(sp)+
	rts	pc

opl30:					/ mul, div etc
	inc	swapf
	mov	$1000,rlimit
	br	opl13

opl14:					/ flop freg,fsrc
	inc	swapf

opl5:					/ flop src,freg
	mov	$400,rlimit

opl13:					/double
	jsr	pc,addres
op2a:
	mov	r2,-(sp)
	jsr	pc,readop
op2b:
	jsr	pc,addres
	tst	swapf
	beq	1f
	mov	(sp),r0
	mov	r2,(sp)
	mov	r0,r2
1:
	swab	(sp)
	asr	(sp)
	asr	(sp)
	cmp	(sp),rlimit
	blo	1f
	mov	$'x,-(sp)
	jsr	pc,error
1:
	bis	(sp)+,r2
	bis	(sp)+,r2
	clr	r3
	jsr	pc,outw
	mov	$adrbuf,r1
1:
	cmp	r1,r5
	bhis	1f
	mov	(r1)+,r2
	mov	(r1)+,r3
	mov	(r1)+,xsymbol
	jsr	pc,outw
	br	1b
1:
	rts	pc

opl15:					/ single operand
	clr	-(sp)
	br	op2b

opl12:					/ movf
	mov	$400,rlimit
	jsr	pc,addres
	cmp	r2,$4			/ see if source is fregister
	blo	1f
	inc	swapf
	br	op2a
1:
	mov	$174000,(sp)
	br	op2a

opl35:					/ jbr
opl36:					/ jeq, jne, etc
	jsr	pc,expres
	tst	passno
	bne	1f
	mov	r2,r0
	jsr	pc,setbr
	tst	r2
	beq	2f
	cmp	(sp),$br
	beq	2f
	add	$2,r2
2:
	add	r2,dot			/ if doesn't fit
	add	$2,dot
	tst	(sp)+
	rts	pc
1:
	jsr	pc,getbr
	bcc	dobranch
	mov	(sp)+,r0
	mov	r2,-(sp)
	mov	r3,-(sp)
	cmp	r0,$br
	beq	2f
	mov	$402,r2
	xor	r0,r2			/ flip cond, add ".+6"
	mov	$1,r3
	jsr	pc,outw
2:
	mov	$1,r3
	mov	$jmp+37,r2
	jsr	pc,outw
	mov	(sp)+,r3
	mov	(sp)+,r2
	jsr	pc,outw
	rts	pc

opl31:					/ sob
	jsr	pc,expres
	jsr	pc,checkreg
	swab	r2
	asr	r2
	asr	r2
	bis	r2,(sp)
	jsr	pc,readop
	jsr	pc,expres
	tst	passno
	beq	3f
	sub	dot,r2
	neg	r2
	mov	r2,r0
	cmp	r0,$-2
	blt	2f
	cmp	r0,$175
	bgt	2f
	add	$4,r2
	br	1f

opl6:					/branch
	jsr	pc,expres
	tst	passno
	beq	3f
dobranch:
	sub	dot,r2
	mov	r2,r0
	cmp	r0,$-254.
	blt	2f
	cmp	r0,$256.
	bgt	2f
1:
	bit	$1,r2
	bne	2f
	cmp	r3,dot-2		/ same relocation as .
	bne	2f
	asr	r2
	dec	r2
	bic	$177400,r2
3:
	bis	(sp)+,r2
	clr	r3
	jsr	pc,outw
	rts	pc
2:
	mov	$'b,-(sp)
	jsr	pc,error
	clr	r2
	br	3b

opl7:					/jsr
	jsr	pc,expres
	jsr	pc,checkreg
	jmp	op2a

opl10:					/ rts
	jsr	pc,expres
	jsr	pc,checkreg
	br	1f

opl11:					/ sys
	jsr	pc,expres
	cmp	r2,$256.
	bhis	0f
	cmp	r3,$1
	ble	1f
0:
	mov	$'a,-(sp)
	jsr	pc,error
1:
	bis	(sp)+,r2
	jsr	pc,outw
	rts	pc

opl16:					/ .byte
	jsr	pc,expres
	jsr	pc,outb
	cmp	r4,$',
	bne	1f
	jsr	pc,readop
	br	opl16
1:
	tst	(sp)+
	rts	pc

opl17:					/ < (.ascii)
	jsr	pc,getw
	mov	$1,r3
	mov	r4,r2
	bmi	2f
	bic	$!377,r2
	jsr	pc,outb
	br	opl17
2:
	jsr	pc,getw
	rts	pc

opl20:					/.even
	bit	$1,dot
	beq	1f
	cmp	dot-2,$4
	beq	2f			/ bss mode
	clr	r2
	clr	r3
	jsr	pc,outb
	br	1f
2:
	inc	dot
1:
	tst	(sp)+
	rts	pc

opl21:					/if
	jsr	pc,expres
opl22:
oplret:
	tst	(sp)+
	rts	pc

opl23:					/.globl
	cmp	r4,$200
	blo	1f
	bisb	$40,(r4)
	jsr	pc,readop
	cmp	r4,$',
	bne	1f
	jsr	pc,readop
	br	opl23
1:
	tst	(sp)+
	rts	pc

opl25:					/ .text, .data, .bss
opl26:
opl27:
	inc	dot
	bic	$1,dot
	mov	r0,-(sp)
	mov	dot-2,r1
	asl	r1
	mov	dot,savdot-4(r1)
	tst	passno
	beq	1f
	mov	$txtp,-(sp)
	jsr	pc,flush
	mov	$relp,-(sp)
	jsr	pc,flush
	mov	(sp),r2
	asl	r2
	add	$txtseek-[4*25],r2
	mov	r2,tseekp
	mov	(r2),r0
	mov	2(r2),r1
	mov	$txtp,-(sp)
	jsr	pc,oset
	add	$trelseek-txtseek,r2
	mov	(r2),r0
	mov	2(r2),r1
	mov	r2,rseekp
	mov	$relp,-(sp)
	jsr	pc,oset
1:
	mov	(sp)+,r0
	mov	savdot-[2*25](r0),dot
	asr	r0
	sub	$25-2,r0
	mov	r0,dot-2		/ new . relocation
	tst	(sp)+
	rts	pc

opl32:
	cmp	r4,$200
	blo	1f
	mov	r4,-(sp)
	jsr	pc,readop
	jsr	pc,readop
	jsr	pc,expres
	mov	(sp)+,r0
	bit	$37,(r0)
	bne	1f
	bis	$40,(r0)
	mov	r2,2(r0)
1:
	tst	(sp)+
	rts	pc

addres:
	clr	-(sp)
4:
	cmp	r4,$'(
	beq	alp
	cmp	r4,$'-
	beq	amin
	cmp	r4,$'$
	beq	adoll
	cmp	r4,$'*
	bne	getx
	jmp	astar
getx:
	jsr	pc,expres
	cmp	r4,$'(
	bne	2f
	jsr	pc,readop
	mov	r2,(r5)+
	mov	r3,(r5)+
	mov	xsymbol,(r5)+
	jsr	pc,expres
	jsr	pc,checkreg
	jsr	pc,checkrp
	bis	$60,r2
	bis	(sp)+,r2
	rts	pc

2:
	cmp	r3,$24
	bne	1f
	jsr	pc,checkreg
	bis	(sp)+,r2
	rts	pc
1:
	mov	r3,-(sp)
	bic	$40,r3
	mov	(sp)+,r3
	bis	$100000,r3
	sub	dot,r2
	sub	$4,r2
	cmp	r5,$adrbuf
	beq	1f
	sub	$2,r2
1:
	mov	r2,(r5)+		/ index
	mov	r3,(r5)+		/ index reloc.
	mov	xsymbol,(r5)+		/ index global
	mov	$67,r2			/ address mode
	bis	(sp)+,r2
	rts	pc

alp:
	jsr	pc,readop
	jsr	pc,expres
	jsr	pc,checkrp
	jsr	pc,checkreg
	cmp	r4,$'+
	beq	1f
	tst	(sp)+
	beq	2f
	bis	$70,r2
	clr	(r5)+
	clr	(r5)+
	mov	xsymbol,(r5)+
	rts	pc
2:
	bis	$10,r2
	rts	pc
1:
	jsr	pc,readop
	bis	$20,r2
	bis	(sp)+,r2
	rts	pc

amin:
	jsr	pc,readop
	cmp	r4,$'(
	beq	1f
	mov	r4,savop
	mov	$'-,r4
	br	getx
1:
	jsr	pc,readop
	jsr	pc,expres
	jsr	pc,checkrp
	jsr	pc,checkreg
	bis	(sp)+,r2
	bis	$40,r2
	rts	pc

adoll:
	jsr	pc,readop
	jsr	pc,expres
	mov	r2,(r5)+
	mov	r3,(r5)+
	mov	xsymbol,(r5)+
	mov	(sp)+,r2
	bis	$27,r2
	rts	pc

astar:
	tst	(sp)
	beq	1f
	mov	$'*,-(sp)
	jsr	pc,error
1:
	mov	$10,(sp)
	jsr	pc,readop
	jmp	4b

checkreg:
	cmp	r2,$7
	bhi	1f
	cmp	r1,$1
	blos	2f
	cmp	r3,$5
	blo	1f
2:
	rts	pc
1:
	mov	$'a,-(sp)
	jsr	pc,error
	clr	r2
	clr	r3
	rts	pc

checkrp:
	cmp	r4,$')
	beq	1f
	mov	$'),-(sp)
	jsr	pc,error
	rts	pc
1:
	jsr	pc,readop
	rts	pc

setbr:
	mov	brtabp,r1
	cmp	r1,$brlen
	blt	1f
	mov	$2,r2
	rts	pc
1:
	inc	brtabp
	clr	-(sp)
	sub	dot,r0
	ble	1f
	sub	brdelt,r0
1:
	cmp	r0,$-254.
	blt	1f
	cmp	r0,$256.
	ble	2f
1:
	mov	r1,-(sp)
	bic	$!7,(sp)
	mov	$1,r0
	ash	(sp)+,r0
	ash	$-3,r1
	bisb	r0,brtab(r1)
	mov	$2,(sp)
2:
	mov	(sp)+,r2
	rts	pc

getbr:
	mov	brtabp,r1
	cmp	r1,$brlen
	blt	1f
	sec
	rts	pc
1:
	mov	r1,-(sp)
	bic	$!7,(sp)
	neg	(sp)
	inc	brtabp
	ash	$-3,r1
	movb	brtab(r1),r1
	ash	(sp)+,r1
	ror	r1			/ 0-bit into c-bit
	rts	pc

expres:
	clr	xsymbol
expres1:
	mov	r5,-(sp)
	mov	$'+,-(sp)
	clr	r2
	mov	$1,r3
	br	1f
advanc:
	jsr	pc,readop
1:
	mov	r4,r0
	blt	6f
	cmp	r0,$177
	ble	7f
6:
	movb	(r4),r0
	bne	1f
	tst	passno
	beq	1f
	mov	$'u,-(sp)
	jsr	pc,error
1:
	tst	overlaid
	beq	0f
/
/ Bill Shannon's hack to the assembler to force globl text
/ references to remain undefined so that the link editor may
/ resolve them. This is necessary if a function is used as an
/ arg in an overlay located piece of code, and the function is
/ actually in another overlay. Elsewise, the assembler fix's up
/ the reference before the link editor changes the globl refrence
/ to the thunk. -wfj 5/80
	cmp	r0,$42			/ is it globl text ?
	bne	0f			/ nope
	mov	$40,r0			/ yes, treat it as undefined external
0:
	cmp	r0,$40
	bne	1f
	mov	r4,xsymbol
	clr	r1
	br	oprand
1:
	mov	2(r4),r1
	br	oprand
7:
	cmp	r4,$141
	blo	1f
	asl	r4
	mov	curfb-[2*141](r4),r0
	mov	2(r0),r1
	movb	(r0),r0
	br	oprand
1:
	mov	$esw1,r1
1:
	cmp	(r1)+,r4
	beq	1f
	tst	(r1)+
	bne	1b
	tst	(sp)+
	mov	(sp)+,r5
	rts	pc
1:
	jmp	*(r1)
	.data
esw1:
	'+;	binop
	'-;	binop
	'*;	binop
	'/;	binop
	'&;	binop
	037;	binop
	035;	binop
	036;	binop
	'%;	binop
	'[;	brack
	'^;	binop
	1;	exnum
	2;	exnum1
	'!;	binop
	200;	0
	.text
binop:
	cmpb	(sp),$'+
	beq	1f
	mov	$'e,-(sp)
	jsr	pc,error
1:
	movb	r4,(sp)
	br	advanc

exnum1:
	mov	numval,r1
	br	1f

exnum:
	jsr	pc,getw
	mov	r4,r1
1:
	mov	$1,r0
	br	oprand

brack:
	mov	r2,-(sp)
	mov	r3,-(sp)
	jsr	pc,readop
	jsr	pc,expres1
	cmp	r4,$']
	beq	1f
	mov	$'],-(sp)
	jsr	pc,error
1:
	mov	r3,r0
	mov	r2,r1
	mov	(sp)+,r3
	mov	(sp)+,r2

oprand:
	mov	$exsw2,r5
1:
	cmp	(sp),(r5)+
	beq	1f
	tst	(r5)+
	bne	1b
	br	eoprnd
1:
	jmp	*(r5)
	.data
exsw2:
	'+; exadd
	'-; exsub
	'*; exmul
	'/; exdiv
	037; exor
	'&; exand
	035;exlsh
	036;exrsh
	'%; exmod
	'^; excmbin
	'!; exnot
	200;  0
	.text
excmbin:
	mov	r0,r3
	br	eoprnd

exrsh:
	neg	r1
	beq	exlsh
	inc	r1
	clc
	ror	r2
exlsh:
	mov	$relte2,r5
	jsr	pc,combin
	ash	r1,r2
	br	eoprnd

exmod:
	mov	$relte2,r5
	jsr	pc,combin
	mov	r3,r0
	mov	r2,r3
	clr	r2
	div	r1,r2
	mov	r3,r2
	mov	r0,r3
	br	eoprnd

exadd:
	mov	$reltp2,r5
	jsr	pc,combin
	add	r1,r2
	br	eoprnd

exsub:
	mov	$reltm2,r5
	jsr	pc,combin
	sub	r1,r2
	br	eoprnd

exand:
	mov	$relte2,r5
	jsr	pc,combin
	com	r1
	bic	r1,r2
	br	eoprnd

exor:
	mov	$relte2,r5
	jsr	pc,combin
	bis	r1,r2
	br	eoprnd

exmul:
	mov	$relte2,r5
	jsr	pc,combin
	mul	r2,r1
	mov	r1,r2
	br	eoprnd

exdiv:
	mov	$relte2,r5
	jsr	pc,combin
	mov	r3,r0
	mov	r2,r3
	clr	r2
	div	r1,r2
	mov	r0,r3
	br	eoprnd

exnot:
	mov	$relte2,r5
	jsr	pc,combin
	com	r1
	add	r1,r2
	br	eoprnd

eoprnd:
	mov	$'+,(sp)
	jmp	advanc

combin:
	tst	passno
	bne	combin1
	mov	r0,-(sp)
	bis	r3,(sp)
	bic	$!40,(sp)
	bic	$!37,r0
	bic	$!37,r3
	cmp	r0,r3
	ble	1f
	mov	r0,-(sp)
	mov	r3,r0
	mov	(sp)+,r3
1:
	tst	r0
	beq	1f
	cmp	r5,$reltm2
	bne	2f
	cmp	r0,r3
	bne	2f
	mov	$1,r3
	br	2f
1:
	clr	r3
2:
	bis	(sp)+,r3
	rts	pc
combin1:
	mov	r1,-(sp)
	clr	maxtyp
	jsr	pc,maprel
	mov	r0,r1
	mul	$6,r1
	mov	r3,r0
	jsr	pc,maprel
	add	r5,r0
	add	r1,r0
	movb	(r0),r3
	bpl	1f
	cmp	r3,$-1
	beq	2f
	mov	$'r,-(sp)
	jsr	pc,error
2:
	mov	maxtyp,r3
1:
	mov	(sp)+,r1
	rts	pc

maprel:
	cmp	r0,$40
	bne	1f
	mov	$5,r0
	rts	pc
1:
	bic	$!37,r0
	cmp	r0,maxtyp
	blos	1f
	mov	r0,maxtyp
1:
	cmp	r0,$5
	blo	1f
	mov	$1,r0
1:
	rts	pc
	.data
X = -2
M = -1
reltp2:
	.byte 0, 0, 0, 0, 0, 0
	.byte 0, M, 2, 3, 4,40
	.byte 0, 2, X, X, X, X
	.byte 0, 3, X, X, X, X
	.byte 0, 4, X, X, X, X
	.byte 0,40, X, X, X, X

reltm2:
	.byte 0, 0, 0, 0, 0, 0
	.byte 0, M, 2, 3, 4,40
	.byte 0, X, 1, X, X, X
	.byte 0, X, X, 1, X, X
	.byte 0, X, X, X, 1, X
	.byte 0, X, X, X, X, X

relte2:
	.byte 0, 0, 0, 0, 0, 0
	.byte 0, M, X, X, X, X
	.byte 0, X, X, X, X, X
	.byte 0, X, X, X, X, X
	.byte 0, X, X, X, X, X
	.byte 0, X, X, X, X, X

qnl:	<?\n>
a.out:	<a.out\0>
	.even
a.outp:	a.out

obufp:	outbuf
passno: -1
outmod:	0777

tseekp:	txtseek
rseekp:	trelseek

txtmagic:
	br	.+20
txtsiz:	.=.+2
datsiz:	.=.+2
bsssiz:	.=.+2
symsiz:	.=.+2
	.=.+6

txtseek:0; 20
datseek:.=.+4
	.=.+4
trelseek:.=.+4
drelseek:.=.+4
	.=.+4
symseek:.=.+4

	.bss
brlen	= 1024.
brtab:	.=.+[brlen\/8.]
brtabp:	.=.+2
brdelt:	.=.+2
fbbufp: .=.+2
defund:	.=.+2
datbase:.=.+2
bssbase:.=.+2
ibufc:	.=.+2
overlaid: .=.+2
adrbuf:	.=.+12.
xsymbol:.=.+2
errflg:	.=.+2
argb:	.=.+22.
numval:	.=.+2
maxtyp:	.=.+2
ibufp:	.=.+2
txtp:	.=.+8.
	.=.+1024.
relp:	.=.+8.
outbuf:
	.=.+1024.
swapf:	.=.+2
rlimit:	.=.+2
endtable:.=.+2
totalsz:			/ string table length
	.=.+4
	.text

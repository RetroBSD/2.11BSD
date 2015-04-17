	.globl	_main, _write, _close, _execl, __exit, _brk
	.globl	_read, _signal, _stat, _open, _mkstemp, _calloc, _realloc

	.globl	error, errore, errora, checkeos, pass1, aexit, argb
	.globl	overlaid, defund, a.outp, errflg, passno, filerr, outmod
	.globl	wrterr, argb, hshsiz, dot, dotdot, savdot, ch, outbuf
	.globl	line, savop, inbuf, fbptr, fbtbl, symnum, hshtab, symblk
	.globl	symleft, dotrel, symtab, fin, fout, curfb, nxtfb, ibufp
	.globl	ibufc, a.tmp1, usymtab, SYMENTSZ, SYMBLKSZ, PSTENTSZ
	.globl 	obufp, Newsym, symbol,csv

/ This assembler supports _both_ the old style object files (with
/ fixed 8 character symbols) and the new style (with a strings table).
/ The variable 'Newsym' defined below specifies which format will be
/ emitted, if the value is '0' the old style is output, if non-zero
/ then the new 'string table' format is output.
/
/ The old style on disc symbol table entries looked like this:
/   struct symbol
/	{
/	char name[8];
/	short type;
/	short value;
/	};
/
/ The new style of on disc symbol table entry looks like:
/   struct symbol
/	{
/	off_t offset_in_string_table;
/	short type;
/	short value;
/	};

	.data
Newsym:	1
	.text

PSTENTSZ = 6.
SYMENTSZ = 8.

/ User symbols and Permanent Symbol Table entries used to have the
/ same 12 byte format.  Merging the two phases of the assembler, moving
/ the symbol name characters to an externally allocated heap and
/ using a non-contiguous user symbol table meant that the symbol number
/ could no longer be calculated by subtracting the base of the symbol
/ table and dividing by the size of an entry.  What was done was to
/ expand the symbol table entry by another word and keep the symbol number
/ in that.  The new internal symbol structure is:
/
/	char	*name;
/	u_short	flags;
/	u_short value;
/	u_short number;

SYMBLKSZ = 512.
STRBLKSZ = 1024.
hshsiz = 3001.

/ PDP-11 assembler
_main:
	jsr	r5,csv
	mov	$1,-(sp)		/ signal(SIGINT, SIG_IGN)
	mov	$2,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
	ror	r0
	bcs	1f

	mov	$aexit,-(sp)		/ signal(SIGINT, aexit)
	mov	$2,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
1:
	mov	4(r5),r0		/ argc
	mov	6(r5),curarg		/ argv
9:
	dec	r0			/ argc--
	add	$2,curarg		/ argv++
1:
	mov	*curarg,r1
	cmpb	(r1)+,$'-
	bne	1f
	cmpb	(r1),$'-		/ is this "--"?
	bne	8f			/ no - br
	tstb	1(r1)			/ check for null terminator
	beq	1f			/ got it, the "--" means read 'stdin'
8:
	add	$2,curarg		/ argv++
	dec	r0			/ argc--
	cmpb	(r1),$'u
	beq	3f
	cmpb	(r1), $'V
	bne	2f
	inc	overlaid
	br	1b
2:
	tstb	(r1)
	bne	2f
3:
	mov	$40,defund
	br	1b
2:
	cmpb	(r1),$'o
	bne	1f
	mov	*curarg,a.outp
	br	9b
1:

/ The new object file format puts a ceiling of 32 characters on symbols.
/ If it is desired to raise this limit all that need be done is increase
/ the same ceiling in the C compiler and linker (ld).

	tst	Newsym
	beq	1f
	movb	$32.,Ncps
1:
	mov	r0,nargs		/ # of remaining args
	bne	8f			/ br if any left
	inc	nargs			/ fake at least one arg
	mov	$dash, curarg		/ of '-' so we read stdin
8:
	mov	$a.tmp1,-(sp)
	jsr	pc,_mkstemp		/ fout = mkstemp(a.tmp1);
	tst	(sp)+
	mov	r0,fout
	bmi	oops

/ the symbol table is a single linked list of dynamically allocated
/ 'SYMBLKSZ' byte blocks.  Allocate the first one now.
	mov	$SYMBLKSZ+2,-(sp)	/ symblk = calloc(1, SYMBLKSZ+2)
	mov	$1,-(sp)
	jsr	pc,_calloc
	cmp	(sp)+,(sp)+
	mov	r0,symblk
	mov	r0,usymtab		/ pointer to first block
	tst	(r0)+			/ skip link word
	mov	r0,symend		/ current end in block
	mov	$SYMBLKSZ,symleft	/ number of bytes left in block

/ The string portion of symbol table entries is now allocated dynamically.
/ We allocate the strings in 1kb chunks to cut down the number of times
/ the string table needs to be extended (besides, long variable names are
/ coming real soon now).
/
/ NOTE: the string blocks are linked together for debugging purposes only,
/ nothing depends on the link.

	mov	$STRBLKSZ+2,-(sp)
	mov	$1,-(sp)
	jsr	pc,_calloc		/ strblk = calloc(1, STRBLKSZ+2)
	/ check for failure???
	cmp	(sp)+,(sp)+
	mov	r0,strblk		/ save pointer to string block
	tst	(r0)+			/ skip link word
	mov	r0,strend		/ set available string pointer
	mov	$STRBLKSZ,strleft	/ set amount left in block

/ the hash table is now dynamically allocated so that it can be
/ reused in pass 2 and re-alloced if necessary.
	mov	$2,-(sp)		/ hshtab = calloc($hshsiz, sizeof(int))
	mov	$hshsiz,-(sp)
	jsr	pc,_calloc
	cmp	(sp)+,(sp)+
	mov	r0, hshtab

	mov	$symtab,r1
1:
	clr	r3
	mov	(r1),r2			/ pointer to PST symbol's string
2:
	movb	(r2)+,r4
	beq	2f
	add	r4,r3
	swab	r3
	br	2b
2:
	clr	r2
	div	$hshsiz,r2
	ashc	$1,r2
	add	hshtab,r3
4:
	sub	r2,r3
	cmp	r3,hshtab
	bhi	3f
	add	$2*hshsiz,r3
3:
	tst	-(r3)
	bne	4b
	mov	r1,(r3)
	add	$PSTENTSZ,r1
	cmp	r1,$ebsymtab
	blo	1b

/ perform pass 0 processing
	jsr	pc,pass0

/ flush the intermediate object file
	mov	$1024.,-(sp)		/ write(fout, outbuf, 1024)
	mov	$outbuf,-(sp)
	mov	fout,-(sp)
	jsr	pc,_write
	add	$6,sp

	tst	errflg			/ any errors?
	beq	1f			/ yes - br
	jsr	pc,aexit
/ not reached
1:
	inc	passno			/ go from -1 to 0
	clr	line			/ reset line number
	jmp	pass1			/ pass1 does both passes 1, 2, exit

oops:
	mov	$9f-8f,-(sp)		/ write(fileno(stderr),8f,strlen())
	mov	$8f,-(sp)
	mov	$2,-(sp)
	jsr	pc,_write
	mov	$2,(sp)
	jsr	pc,__exit
	.data
8:
	<as: can't create tmpfile\n>
9:
	.even
	.text

error:
	tst	passno			/ on pass1,2 ?
	bpl	errorp2
	inc	errflg
	mov	r0,-(sp)
	mov	r1,-(sp)
	mov	r5,r0
	tst	*curarg
	beq	1f
	mov	r0,-(sp)
	mov	*curarg,-(sp)
	clr	*curarg
	jsr	pc,filerr
	tst	(sp)+
	mov	(sp)+,r0
1:
	mov	r2,-(sp)
	mov	r3,-(sp)
	jsr	pc,errcmn
	mov	(sp)+,r3
	mov	(sp)+,r2
	mov	(sp)+,r1
	mov	(sp)+,r0
	rts	pc

errorp2:
	mov	pc,errflg
	mov	$666,outmod		/ make nonexecutable
	mov	r3,-(sp)
	mov	r2,-(sp)
	mov	r1,-(sp)
	mov	r0,-(sp)

	tst	-(sp)			/ write(1, argb, strlen(argb))
	mov	$argb,-(sp)
	mov	$1,-(sp)
	mov	$argb,r1
	clr	r0
1:
	tstb	(r1)+
	beq	2f
	inc	r0
	br	1b
2:
	mov	r0,4(sp)
	jsr	pc,_write
	add	$6,sp

	movb	12(sp),r0
	jsr	pc,errcmn
	mov	(sp)+,r0
	mov	(sp)+,r1
	mov	(sp)+,r2
	mov	(sp)+,r3
	mov	(sp)+,(sp)
	rts	pc

errcmn:
	mov	line,r3
	movb	r0,9f
	mov	$9f+6,r0
	mov	$4,r1
2:
	clr	r2
	div	$10.,r2
	add	$'0,r3
	movb	r3,-(r0)
	mov	r2,r3
	sob	r1,2b

	mov	$7,-(sp)		/ write(1, 9f, 7)
	mov	$9f,-(sp)
	mov	$1,-(sp)
	jsr	pc,_write
	add	$6,sp
	rts	pc

	.data
9:	<f xxxx\n>
	.even
	.text

p0putw:
	tst	ifflg
	beq	1f
	cmp	r4,$'\n
	bne	2f
1:
	mov	r4,*obufp
	add	$2,obufp
	cmp	obufp,$outbuf+1024.
	blo	2f
	mov	$outbuf,obufp

	mov	r1,-(sp)		/ protect r1 from library
	mov	$1024.,-(sp)		/ write(fout, outbuf, 1024)
	mov	$outbuf,-(sp)
	mov	fout,-(sp)
	jsr	pc,_write
	add	$6,sp
	mov	(sp)+,r1
	tst	r0
	bpl	2f
	jmp	wrterr
2:
	rts	pc

/ Pass 0.

pass0:
	jsr	pc,p0readop
	jsr	pc,checkeos
		br 7f
	tst	ifflg
	beq	3f
	cmp	r4,$200
	blos	pass0
	cmpb	(r4),$21		/if
	bne	2f
	inc	ifflg
2:
	cmpb	(r4),$22   		/endif
	bne	pass0
	dec	ifflg
	br	pass0
3:
	mov	r4,-(sp)
	jsr	pc,p0readop
	cmp	r4,$'=
	beq	4f
	cmp	r4,$':
	beq	1f
	mov	r4,savop
	mov	(sp)+,r4
	jsr	pc,opline
	br	ealoop
1:
	mov	(sp)+,r4
	cmp	r4,$200
	bhis	1f
	cmp	r4,$1			/ digit
	beq	3f
	mov	$'x,r5
	jsr	pc,error
	br	pass0
1:
	bitb	$37,(r4)
	beq	1f
	mov	$'m,r5
	jsr	pc,error
1:
	bisb	dot-2,(r4)
	mov	dot,2(r4)
	br	pass0
3:
	mov	numval,r0
	jsr	pc,fbcheck
	movb	dotrel,curfbr(r0)
	asl	r0
	movb	dotrel,nxtfb
	mov	dot,nxtfb+2
	movb	r0,nxtfb+1
	mov	dot,curfb(r0)

	cmp	fbfree,$4		/ room for another fb entry?
	bge	5f			/ yes - br
	jsr	pc,growfb		/ no - grow the table
5:
	sub	$4,fbfree		/ four bytes less available
	mov	nxtfb,*fbptr		/ store first word
	add	$2,fbptr		/ advance to next
	mov	nxtfb+2,*fbptr		/ store second word
	add	$2,fbptr		/ point to next entry
	br	pass0
4:
	jsr	pc,p0readop
	jsr	pc,expres
	mov	(sp)+,r1
	cmp	r1,$200
	bhis	1f
	mov	$'x,r5
	jsr	pc,error
7:
	br	ealoop
1:
	cmp	r1,$dotrel
	bne	2f
	bic	$40,r3
	cmp	r3,dotrel
	bne	1f
2:
	bicb	$37,(r1)
	bic	$!37,r3
	bne	2f
	clr	r2
2:
	bisb	r3,(r1)
	mov	r2,2(r1)
	br	ealoop
1:
	mov	$'.,r5
	jsr	pc,error
	movb	$2,dotrel
ealoop:
	cmp	r4,$';
	beq	9f
	cmp	r4,$'\n
	bne	1f
	inc	line
	br	9f
1:
	cmp	r4,$'\e
	bne	2f
	tst	ifflg
	beq	1f
	mov	$'x,r5
	jsr	pc,error
1:
	rts	pc
2:
	mov	$'x,r5
	jsr	pc,error
2:
	jsr	pc,checkeos
		br 9f
	jsr	pc,p0readop
	br	2b
9:
	jmp	pass0

fbcheck:
	cmp	r0,$9.
	bhi	1f
	rts	pc
1:
	mov	$'f,r5
	jsr	pc,error
	clr	r0
	rts	pc

/ the 'fb' table never grows to be large.  In fact all of the assemblies
/ of C compiler generated code which were processed by 'c2' never
/ produced a table larger than 0 bytes.  So we 'realloc' because
/ this is not done enough to matter.

growfb:
	mov	r1,-(sp)		/ save r1 from library
	add	$256.,fbtblsz		/ new size of fb table
	mov	$256.,fbfree		/ number available now
	mov	fbtblsz,-(sp)		/ fbtbl = realloc(fbtbl, fbtblsz);
	mov	fbtbl,-(sp)
	bne	1f			/ extending table - br
	mov	$1,(sp)			/ r0 = calloc(1, fbtblsz);
	jsr	pc,_calloc
	br	2f
1:
	jsr	pc,_realloc
2:
	cmp	(sp)+,(sp)+
	mov	r0,fbtbl
	bne	1f
	iot				/ Can never happen (I hope)
1:
	add	fbtblsz,r0		/ fbptr starts 256 bytes from
	sub	$256.,r0		/ end of new region
	mov	r0,fbptr
	mov	(sp)+,r1		/ restore register
	rts	pc

/ Symbol table lookup and hashtable maintenance.

rname:
	mov	r1,-(sp)
	mov	r2,-(sp)
	mov	r3,-(sp)
	movb	Ncps,r5			/ Max num of chars to accept
	mov	$symbol,r2
	clr	(r2)
	clr	-(sp)
	clr	-(sp)
	cmp	r0,$'~			/ symbol not for hash table?
	bne	1f			/ no - br
	inc	2(sp)
	clr	ch
1:
	jsr	pc,rch
	movb	chartab(r0),r3
	ble	1f
	add	r3,(sp)
	swab	(sp)
	dec	r5
	blt	1b
	movb	r3,(r2)+
	br	1b
1:
	clrb	(r2)+			/ null terminate string
	movb	r0,ch
	mov	(sp)+,r1
	clr	r0
	tst	(sp)+
	beq	1f
	mov	symend,r4
	br	4f			/ go insert into symtable (!hashtbl)
1:
	div	$hshsiz,r0
	ashc	$1,r0
	add	hshtab,r1
	clr	timesaround
1:
	sub	r0,r1
	cmp	r1,hshtab
	bhi	2f
	add	$2*hshsiz,r1
	tst	timesaround
	beq	3f

	mov	$8f-9f,-(sp)		/ write(fileno(stdout),9f,8f-9f);
	mov	$9f,-(sp)
	mov	$1,-(sp)
	jsr	pc,_write
	add	$6,sp
	jsr	pc,aexit
/ not reached

	.data
timesaround: 0
9:
	<as: symbol table overflow\n>
8:
	.even
	.text
3:
	inc	timesaround
2:
	mov	$symbol,r2
	mov	-(r1),r4
	beq	3f
	mov	(r4)+,r3		/ ptr to symbol's name
9:
	cmpb	(r2),(r3)+
	bne	1b			/ not the right symbol - br
	tstb	(r2)+			/ at end of symbol?
	bne	9b			/ nope - keep looking
	br	1f			/ yep - br
3:
	mov	symend,r4
	jsr	pc,isroom		/ make sure there's room in block
	mov	r4,(r1)
4:
	jsr	pc,isroom		/ check for room in current block

	mov	$symbol,r2		/ length of string (including null)
8 :
	tstb	(r2)+
	bne	8b
	sub	$symbol,r2
	jsr	pc,astring		/ allocate string space
	mov	r0,(r4)+		/ save string pointer in symtab entry
	mov	$symbol,r1
9:
	movb	(r1)+,(r0)+		/ copy symbol name to string block
	bne	9b
	sub	$SYMENTSZ,symleft

/ each new symbol is assigned a unique one up number.  This is done because
/ the user symbol table is no longer contiguous - the symbol number can
/ not be calculated by subtracting a base address and dividing by the
/ size of a symbol.

	clr	(r4)+			/ flags word
	clr	(r4)+			/ value word
	mov	symnum,(r4)+
	inc	symnum
	mov	r4,symend
	sub	$6,r4			/ point to flags word
1:
	mov	r4,-(sp)
	mov	r4,r3
	tst	-(r3)			/ back to beginning of entry
	cmp	r3,$ebsymtab		/ Permanent Symbol Table(opcode, etc)?
	blo	1f			/ yes - br
	mov	6(r3),r4		/ get symbol number
	add	$4000,r4		/ user symbol flag
	br	2f
1:

/ PST entries are PSTENTSZ bytes each because they do not have a 'symnum'
/ entry associated with them.

	sub	$symtab,r3
	clr	r2
	div	$PSTENTSZ,r2
	mov	r2,r4
	add	$1000,r4		/ builtin symbol
2:
	jsr	pc,p0putw
	mov	(sp)+,r4
	mov	(sp)+,r3
	mov	(sp)+,r2
	mov	(sp)+,r1
	tst	(sp)+
	rts	pc

isroom:
	cmp	symleft,$SYMENTSZ	/ room for another symbol?
	bge	1f			/ yes - br
	mov	r1,-(sp)		/ save from library
	mov	$SYMBLKSZ+2,-(sp)	/ size of sym block plus link word
	mov	$1,-(sp)		/ number of blocks to allocate
	jsr	pc,_calloc		/ calloc(1, SYMBLKSZ+2);
	cmp	(sp)+,(sp)+
	/ check for failure?
	mov	r0,*symblk		/ link new block to old
	mov	r0,symblk		/ this is now the current block
	tst	(r0)+			/ skip link word
	mov	$SYMBLKSZ,symleft	/ number of bytes available
	mov	r0,r4			/ put where it's expected
	mov	(sp)+,r1		/ restore saved register
1:
	rts	pc			/ return

/ allocate room for a string, the length is in R2 and includes room for
/ a terminating null byte.

astring:
	cmp	r2,strleft		/ room in current block?
	ble	1f			/ yes - go carve out a chunk
	mov	$STRBLKSZ+2,-(sp)
	mov	$1,-(sp)
	jsr	pc,_calloc		/ symblk = calloc(1,STRBLKSZ+2)
	/ check for failure?
	cmp	(sp)+,(sp)+
	mov	r0,*strblk		/ update forward link between blocks
	mov	r0,strblk		/ update current string block pointer
	tst	(r0)+			/ skip link word
	mov	r0,strend		/ current data pointer
	mov	$STRBLKSZ,strleft	/ amount of space left
1:
	mov	strend,r0		/ string address
	add	r2,strend		/ update current end point
	sub	r2,strleft		/ update amount of space left
	rts	pc

number:
	mov	r2,-(sp)
	mov	r3,-(sp)
	mov	r5,-(sp)
	clr	r1
	clr	r5
1:
	jsr	pc,rch
	cmp	r0,$'0
	blt	1f
	cmp	r0,$'9
	bgt	1f
	sub	$'0,r0
	mul	$10.,r5
	add	r0,r5
	ash	$3,r1
	add	r0,r1
	br	1b
1:
	cmp	r0,$'b
	beq	1f
	cmp	r0,$'f
	beq	1f
	cmp	r0,$'.
	bne	2f
	mov	r5,r1
	clr	r0
2:
	movb	r0,ch
	mov	r1,r0
	mov	(sp)+,r5
	mov	(sp)+,r3
	mov	(sp)+,r2
	rts	pc
1:
	mov	r0,r3
	mov	r5,r0
	jsr	pc,fbcheck
	add	$141,r0
	cmp	r3,$'b
	beq	1f
	add	$10.,r0
1:
	mov	r0,r4
	mov	(sp)+,r5
	mov	(sp)+,r3
	mov	(sp)+,r2
	add	$2,(sp)
	rts	pc

rch:
	movb	ch,r0
	beq	1f
	clrb	ch
	rts	pc
1:
	dec	ibufc
	blt	2f
	movb	*ibufp,r0
	inc	ibufp
	bic	$!177,r0
	beq	1b
	rts	pc
2:
	mov	fin,r0
	bmi	3f
	mov	r1,-(sp)		/ protect r1 from library
	mov	$1024.,-(sp)		/ read(fin, inbuf, 1024)
	mov	$inbuf,-(sp)
	mov	r0,-(sp)
	jsr	pc,_read
	add	$6,sp
	mov	(sp)+,r1
	tst	r0
	ble	2f
	mov	r0,ibufc
	mov	$inbuf,ibufp
	br	1b
2:
	mov	r1,-(sp)		/ protect r1 from library
	mov	fin,-(sp)		/ close(r0)
	jsr	pc,_close
	tst	(sp)+
	mov	$-1,fin
	mov	(sp)+,r1
3:
	dec	nargs
	bge	2f
	mov	$'\e,r0
	rts	pc
2:
	tst	ifflg
	beq	2f
	mov	$'i,r5
	jsr	pc,error
	jsr	pc,aexit
/ not reached

2:
/ check for the filename arguments of "-" or "--", these mean to read 'stdin'.
/ Additional filenames are permitted and will be processed when EOF
/ is detected on stdin.
	mov	*curarg,r0
	cmpb	(r0)+,$'-
	bne	5f			/ not the special case - br
	tstb	(r0)			/ must be '-' by itself
	beq	4f
	cmpb	(r0)+,$'-		/ check for "--"
	bne	5f			/ not a double -, must be a filename
	tstb	(r0)			/ null terminated?
	bne	5f			/ no - must be a filename
4:
	clr	fin			/ file descriptor is 0 for stdin
	br	2f
5:
	mov	r1,-(sp)		/ protect r1 from library
	clr	-(sp)			/ open((r0), O_RDONLY, 0)
	clr	-(sp)
	mov	*curarg,-(sp)
	jsr	pc,_open
	add	$6,sp
	mov	(sp)+,r1
	mov	r0,fin
	bpl	2f
	mov	*curarg,-(sp)
	jsr	pc,filerr
	tst	(sp)+
	jsr	pc,aexit
/not reached
2:
	mov	$1,line
	mov	r4,-(sp)
	mov	r1,-(sp)
	mov	$5,r4
	jsr	pc,p0putw
	mov	*curarg,r1
2:
	movb	(r1)+,r4
	beq	2f
	jsr	pc,p0putw
	br	2b
2:
	add	$2,curarg
	mov	$-1,r4
	jsr	pc,p0putw
	mov	(sp)+,r1
	mov	(sp)+,r4
	br	1b

p0readop:
	mov	savop,r4
	beq	1f
	clr	savop
	rts	pc
1:
	jsr	pc,8f
	jsr	pc,p0putw
	rts	pc

8:
	jsr	pc,rch
	mov	r0,r4
	movb	chartab(r0),r1
	bgt	rdname
	jmp	*1f-2(r1)

	.data
	fixor
	escp
	8b
	retread
	dquote
	garb
	squote
	rdname
	skip
	rdnum
	retread
	string
1:
	.text
escp:
	jsr	pc,rch
	mov	$esctab,r1
1:
	cmpb	r0,(r1)+
	beq	1f
	tstb	(r1)+
	bne	1b
	rts	pc
1:
	movb	(r1),r4
	rts	pc

	.data
esctab:
	.byte '/, '/
	.byte '\<, 035
	.byte '>, 036
	.byte '%, 037
	.byte 0, 0
	.text

fixor:
	mov	$037,r4
retread:
	rts	pc

rdname:
	movb	r0,ch
	cmp	r1,$'0
	blo	1f
	cmp	r1,$'9
	blos	rdnum
1:
	jmp	rname

rdnum:
	jsr	pc,number
		br 1f
	rts	pc

squote:
	jsr	pc,rsch
	br	1f
dquote:
	jsr	pc,rsch
	mov	r0,-(sp)
	jsr	pc,rsch
	swab	r0
	bis	(sp)+,r0
1:
	mov	r0,numval
	mov	$1,r4
	jsr	pc,p0putw
	mov	numval,r4
	jsr	pc,p0putw
	mov	$1,r4
	tst	(sp)+
	rts	pc

skip:
	jsr	pc,rch
	mov	r0,r4
	cmp	r0,$'\e
	beq	1f
	cmp	r0,$'\n
	bne	skip
1:
	rts	pc

garb:
	mov	$'g,r5
	jsr	pc,error
	br	8b

string:
	mov	$'<,r4
	jsr	pc,p0putw
	clr	numval
1:
	jsr	pc,rsch
	tst	r1
	bne	1f
	mov	r0,r4
	bis	$400,r4
	jsr	pc,p0putw
	inc	numval
	br	1b
1:
	mov	$-1,r4
	jsr	pc,p0putw
	mov	$'<,r4
	tst	(sp)+
	rts	pc

rsch:
	jsr	pc,rch
	cmp	r0,$'\e
	beq	4f
	cmp	r0,$'\n
	beq	4f
	clr	r1
	cmp	r0,$'\\
	bne	3f
	jsr	pc,rch
	mov	$schar,r2
1:
	cmpb	(r2)+,r0
	beq	2f
	tstb	(r2)+
	bpl	1b
	rts	pc
2:
	movb	(r2)+,r0
	clr	r1
	rts	pc
3:
	cmp	r0,$'>
	bne	1f
	inc	r1
1:
	rts	pc
4:
	mov	$'<,r5
	jsr	pc,error
	jsr	pc,aexit
/ not reached

	.data
schar:
	.byte 'n, 012
	.byte 's, 040
	.byte 't, 011
	.byte 'e, 004
	.byte '0, 000
	.byte 'r, 015
	.byte 'a, 006
	.byte 'p, 033
	.byte 0,  -1
	.text

opline:
	mov	r4,r0
	bmi	1f
	cmp	r0,$200
	bgt	1f
	cmp	r0,$'<
	bne	xpr
	jmp	opl17
xpr:
	jsr	pc,expres
	add	$2,dot
	rts	pc
1:
	movb	(r4),r0
	cmp	r0,$24
	beq	xpr
	cmp	r0,$5
	blt	xpr
	cmp	r0,$36
	bgt	xpr
	mov	r0,-(sp)
	jsr	pc,p0readop
	mov	(sp)+,r0
	asl	r0
	jmp	*1f-12(r0)

	.data
1:
	opl13			/ map fop freg,fdst to double
	opl6
	opl7
	opl10
	opl11
	opl13			/ map fld/fst to double
	opl13
	opl13			/ map fop fsrc,freg to double
	opl15
	opl16
	opl17
	opl20
	opl21
	opl22
	opl23
	xpr
	opl25
	opl26
	opl27
	opl13  			/ map mul s,r to double
	opl31
	opl32
	xpr
	xpr
	opl35
	opl36
	.text

opl35:					/ jbr
	mov	$4,-(sp)
	br	1f

opl36:					/ jeq, etc
	mov	$6,-(sp)
1:
	jsr	pc,expres
	cmp	r3,dotrel
	bne	1f
	sub	dot,r2
	bge	1f
	cmp	r2,$-376
	blt	1f
	mov	$2,(sp)
1:
	add	(sp)+,dot
	rts	pc

opl13:
opl7:					/double
	jsr	pc,addres
op2:
	cmp	r4,$',
	beq	1f
	jsr	pc,errora
	rts	pc
1:
	jsr	pc,p0readop
opl15:   				/ single operand
	jsr	pc,addres
	add	$2,dot
	rts	pc

opl31:					/ sob
	jsr	pc,expres
	cmp	r4,$',
	beq	1f
	jsr	pc,errora
1:
	jsr	pc,p0readop

opl6:
opl10:
opl11:					/branch
	jsr	pc,expres
	add	$2,dot
	rts	pc

opl16:					/ .byte
	jsr	pc,expres
	inc	dot
	cmp	r4,$',
	bne	1f
	jsr	pc,p0readop
	br	opl16
1:
	rts	pc

opl17:					/ < (.ascii)
	add	numval,dot
	jsr	pc,p0readop
	rts	pc

opl20:					/.even
	inc	dot
	bic	$1,dot
	rts	pc

opl21:					/.if
	jsr	pc,expres
	tst	r3
	bne	1f
	mov	$'U,r5
	jsr	pc,error
1:
	tst	r2
	bne	opl22
	inc	ifflg
opl22:					/endif
	rts	pc

opl23:					/.globl
	cmp	r4,$200
	blo	1f
	bisb	$40,(r4)
	jsr	pc,p0readop
	cmp	r4,$',
	bne	1f
	jsr	pc,p0readop
	br	opl23
1:
	rts	pc

opl25:
opl26:
opl27:
	mov	dotrel,r1
	asl	r1
	mov	dot,savdot-4(r1)
	mov	savdot-[2*25](r0),dot
	asr	r0
	sub	$25-2,r0
	mov	r0,dotrel
	rts	pc

opl32:					/ .common
	cmp	r4,$200
	blo	1f
	bis	$40,(r4)
	jsr	pc,p0readop
	cmp	r4,$',
	bne	1f
	jsr	pc,p0readop
	jsr	pc,expres
	rts	pc
1:
	mov	$'x,r5
	jsr	pc,error
	rts	pc

addres:
	cmp	r4,$'(
	beq	alp
	cmp	r4,$'-
	beq	amin
	cmp	r4,$'$
	beq	adoll
	cmp	r4,$'*
	beq	astar
getx:
	jsr	pc,expres
	cmp	r4,$'(
	bne	2f
	jsr	pc,p0readop
	jsr	pc,expres
	jsr	pc,checkreg
	jsr	pc,checkrp
1:
	add	$2,dot
	clr	r0
	rts	pc
2:
	cmp	r3,$24			/ register type
	bne	1b
	jsr	pc,checkreg
	clr	r0
	rts	pc

alp:
	jsr	pc,p0readop
	jsr	pc,expres
	jsr	pc,checkrp
	jsr	pc,checkreg
	cmp	r4,$'+
	bne	1f
	jsr	pc,p0readop
	clr	r0
	rts	pc
1:
	mov	$2,r0
	rts	pc

amin:
	jsr	pc,p0readop
	cmp	r4,$'(
	beq	1f
	mov	r4,savop
	mov	$'-,r4
	br	getx
1:
	jsr	pc,p0readop
	jsr	pc,expres
	jsr	pc,checkrp
	jsr	pc,checkreg
	clr	r0
	rts	pc

adoll:
	jsr	pc,p0readop
	jsr	pc,expres
	add	$2,dot
	clr	r0
	rts	pc

astar:
	jsr	pc,p0readop
	cmp	r4,$'*
	bne	1f
	mov	$'*,r5
	jsr	pc,error
1:
	jsr	pc,addres
	add	r0,dot
	rts	pc

errora:
	mov	$'a,r5
	jsr	pc,error
	rts	pc

checkreg:
	cmp	r2,$7
	bhi	1f
	cmp	r3,$1
	beq	2f
	cmp	r3,$4
	bhi	2f
1:
	jsr	pc,errora
2:
	rts	pc

errore:
	mov	$'e,r5
	jsr	pc,error
	rts	pc

checkrp:
	cmp	r4,$')
	beq	1f
	mov	$'),r5
	jsr	pc,error
	rts	pc
1:
	jsr	pc,p0readop
	rts	pc

expres:
	mov	r5,-(sp)
	mov	$'+,-(sp)
	clr	opfound
	clr	r2
	mov	$1,r3
	br	1f
advanc:
	jsr	pc,p0readop
1:
	mov	r4,r0
	tst	r0
	blt	6f
	cmp	r0,$177
	ble	7f
6:
	movb	(r4),r0
	mov	2(r4),r1
	br	oprand
7:
	cmp	r4,$141
	blo	1f
	cmp	r4,$141+10.
	bhis	2f
	movb	curfbr-141(r4),r0
	asl	r4
	mov	curfb-[2*141](r4),r2
	cmp	r2,$-1
	bne	oprand
	mov	$'f,r5
	jsr	pc,error
	br	oprand
2:
	clr	r3
	clr	r2
	br	oprand
1:
	mov	$esw1,r1
1:
	cmp	(r1)+,r4
	beq	1f
	tst	(r1)+
	bne	1b
	tst	opfound
	bne	2f
	jsr	pc,errore
2:
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
	'!;	binop
	0;	0
	.text

binop:
	cmpb	(sp),$'+
	beq	1f
	jsr	pc,errore
1:
	movb	r4,(sp)
	br	advanc

exnum:
	mov	numval,r1
	mov	$1,r0
	br	oprand

brack:
	mov	r2,-(sp)
	mov	r3,-(sp)
	jsr	pc,p0readop
	jsr	pc,expres
	cmp	r4,$']
	beq	1f
	mov	$'],r5
	jsr	pc,error
1:
	mov	r3,r0
	mov	r2,r1
	mov	(sp)+,r3
	mov	(sp)+,r2

oprand:
	inc	opfound
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
	'!; exnot
	'^; excmbin
	0;  0
	.text

excmbin:
	mov	r0,r3			/ give left flag of right
	br	eoprnd

exrsh:
	neg	r1
	beq	exlsh
	inc	r1
	clc
	ror	r2
exlsh:
	clr	r5
	jsr	pc,combin
	ash	r1,r2
	br	eoprnd

exmod:
	clr	r5
	jsr	pc,combin
	mov	r1,-(sp)
	mov	r2,r1
	clr	r0
	div	(sp)+,r0
	mov	r1,r2
	br	eoprnd

exadd:
	clr	r5
	jsr	pc,combin
	add	r1,r2
	br	eoprnd

exsub:
	mov	$1,r5
	jsr	pc,combin
	sub	r1,r2
	br	eoprnd

exand:
	clr	r5
	jsr	pc,combin
	com	r1
	bic	r1,r2
	br	eoprnd

exor:
	clr	r5
	jsr	pc,combin
	bis	r1,r2
	br	eoprnd

exmul:
	clr	r5
	jsr	pc,combin
	mul	r2,r1
	mov	r1,r2
	br	eoprnd

exdiv:
	clr	r5
	jsr	pc,combin
	mov	r1,-(sp)
	mov	r2,r1
	clr	r0
	div	(sp)+,r0
	mov	r0,r2
	br	eoprnd

exnot:
	clr	r5
	jsr	pc,combin
	com	r1
	add	r1,r2
	br	eoprnd

eoprnd:
	mov	$'+,(sp)
	jmp	advanc

combin:
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
	tst	r5
	beq	2f
	cmp	r0,r3
	bne	2f
	mov	$1,r3
	br	2f
1:
	clr	r3
2:
	bis	(sp)+,r3
	rts	pc

	.data
chartab:
	.byte -14,-14,-14,-14,-02,-14,-14,-14
	.byte -14,-22, -2,-14,-14,-22,-14,-14
	.byte -14,-14,-14,-14,-14,-14,-14,-14
	.byte -14,-14,-14,-14,-14,-14,-14,-14
	.byte -22,-20,-16,-14,-20,-20,-20,-12
	.byte -20,-20,-20,-20,-20,-20,056,-06
	.byte 060,061,062,063,064,065,066,067
	.byte 070,071,-20,-02,-00,-20,-14,-14
	.byte -14,101,102,103,104,105,106,107
	.byte 110,111,112,113,114,115,116,117
	.byte 120,121,122,123,124,125,126,127
	.byte 130,131,132,-20,-24,-20,-20,137
	.byte -14,141,142,143,144,145,146,147
	.byte 150,151,152,153,154,155,156,157
	.byte 160,161,162,163,164,165,166,167
	.byte 170,171,172,-14,-26,-14,176,-14

a.tmp1:	</tmp/atm1XX\0>
Ncps:	.byte 8.
1:	<-\0>
	.even
dash:	1b
fin:	-1
fout:	-1
/ The next two _must_ be adjacent!  Not sure why, but then this whole
/ assembler is weird beyond belief.
curfb:	-1;-1;-1;-1;-1;-1;-1;-1;-1;-1
nxtfb:	.=.+20.			/ first 4 used by pass0, all 20. by pass1+2
	.bss
curfbr:	.=.+10.
savdot:	.=.+6
hshtab:	.=.+2			/ dynamically allocated
ch:	.=.+2
symnum:	.=.+2			/ symbol number
symbol:	.=.+32.			/ XXX
	.=.+2			/ paranoia to make sure a null is present
inbuf:  .=.+1024.
line:	.=.+2
ifflg:	.=.+2
nargs:	.=.+2
curarg:	.=.+2
opfound:.=.+2
savop:	.=.+2
numval:	.=.+2
fbtblsz:.=.+2
fbfree: .=.+2
fbptr:  .=.+2
fbtbl:  .=.+2
usymtab:.=.+2			/ ptr to first block of symbols
symleft:.=.+2			/ bytes left in current symbol block
symend: .=.+2			/ ptr to next symbol space in block
symblk: .=.+2			/ ptr to beginning of current sym block
strleft:.=.+2			/ amount left in string block
strend:	.=.+2			/ ptr to next available string byte
strblk:	.=.+2			/ ptr to current string block link word

/ key to types

/	0	undefined
/	1	absolute (nop, reset, bpt, ...)
/	2	text
/	3	data
/	4	bss
/	5	flop freg,dst (movfo, = stcfd)
/	6	branch
/	7	jsr
/	10	rts
/	11	sys, trap
/	12	movf (=ldf,stf)
/	13	double operand (mov)
/	14	flop fsrc,freg (addf)
/	15	single operand (clr)
/	16	.byte
/	17	string (.ascii, "<")
/	20	.even
/	21	.if
/	22	.endif
/	23	.globl
/	24	register
/	25	.text
/	26	.data
/	27	.bss
/	30	mul,div, etc
/	31	sob
/	32	.comm
/	33	estimated text
/	34	estimated data
/	35	jbr
/	36	jeq, jne, etc

	.data
/ the format of PST entries was changed.  rather than fixed 8 byte strings
/ (often with many trailing nulls) a pointer to a null terminated string
/ is now used.  This saves quite a bit of space since most PST entries are
/ only 3 or 4 characters long.  we had to do this the hard way since there
/ is no macro capability in the assembler and i'd chuck out the SDI [Span
/ Dependent Instruction] stuff and use my own assembler before trying to
/ add macros to this one.  Symbols beginning with 'L' are used since the
/ linker can be told to discard those.

symtab:
/ special symbols

L1; dotrel: 02;    dot: 0000000
L2;	    01; dotdot: 0000000

/ register

L3;	24;	000000
L4;	24;	000001
L5;	24;	000002
L6;	24;	000003
L7;	24;	000004
L8;	24;	000005
L9;	24;	000006
L10;	24;	000007

/ double operand

L11;	13;	0010000
L12;	13;	0110000
L13;	13;	0020000
L14;	13;	0120000
L15;	13;	0030000
L16;	13;	0130000
L17;	13;	0040000
L18;	13;	0140000
L19;	13;	0050000
L20;	13;	0150000
L21;	13;	0060000
L22;	13;	0160000

/ branch

L23;	06;	0000400
L24;	06;	0001000
L25;	06;	0001400
L26;	06;	0002000
L27;	06;	0002400
L28;	06;	0003000
L29;	06;	0003400
L30;	06;	0100000
L31;	06;	0100400
L32;	06;	0101000
L33;	06;	0101400
L34;	06;	0102000
L35;	06;	0102400
L36;	06;	0103000
L37;	06;	0103000
L38;	06;	0103000
L39;	06;	0103400
L40;	06;	0103400
L41;	06;	0103400

/ jump/branch type

L42;	35;	0000400
L43;	36;	0001000
L44;	36;	0001400
L45;	36;	0002000
L46;	36;	0002400
L47;	36;	0003000
L48;	36;	0003400
L49;	36;	0100000
L50;	36;	0100400
L51;	36;	0101000
L52;	36;	0101400
L53;	36;	0102000
L54;	36;	0102400
L55;	36;	0103000
L56;	36;	0103000
L57;	36;	0103000
L58;	36;	0103400
L59;	36;	0103400
L60;	36;	0103400

/ single operand

L61;	15;	0005000
L62;	15;	0105000
L63;	15;	0005100
L64;	15;	0105100
L65;	15;	0005200
L66;	15;	0105200
L67;	15;	0005300
L68;	15;	0105300
L69;	15;	0005400
L70;	15;	0105400
L71;	15;	0005500
L72;	15;	0105500
L73;	15;	0005600
L74;	15;	0105600
L75;	15;	0005700
L76;	15;	0105700
L77;	15;	0006000
L78;	15;	0106000
L79;	15;	0006100
L80;	15;	0106100
L81;	15;	0006200
L82;	15;	0106200
L83;	15;	0006300
L84;	15;	0106300
L85;	15;	0000100
L86;	15;	0000300
L87;	15;	0006500
L88;	15;	0006600
L89;	15;	0106500
L90;	15;	0106600
L91;	15;	0170300
L92;	15;	0106700
L93;	15;	0106400
L94;	15;	0007000
L95;	15;	0007200
L96;	15;	0007300

/ jsr

L97;	07;	0004000

/ rts

L98;	010;	000200

/ simple operand

L99;	011;	104400
L102;	011;	000230

/ flag-setting

L103;	01;	0000240
L104;	01;	0000241
L105;	01;	0000242
L106;	01;	0000244
L107;	01;	0000250
L108;	01;	0000257
L109;	01;	0000261
L110;	01;	0000262
L111;	01;	0000264
L112;	01;	0000270
L113;	01;	0000277
L114;	01;	0000000
L115;	01;	0000001
L116;	01;	0000002
L117;	01;	0000003
L118;	01;	0000004
L119;	01;	0000005
L120;	01;	0000006
L121;	01;	0000007

/ floating point ops

L122;	01;	170000
L123;	01;	170001
L124;	01;	170011
L125;	01;	170002
L126;	01;	170012
L127;	15;	170400
L128;	15;	170700
L129;	15;	170600
L130;	15;	170500
L131;	12;	172400
L132;	14;	177000
L133;	05;	175400
L134;	14;	177400
L135;	05;	176000
L136;	14;	172000
L137;	14;	173000
L138;	14;	171000
L139;	14;	174400
L140;	14;	173400
L141;	14;	171400
L142;	14;	176400
L143;	05;	175000
L144;	15;	170100
L145;	15;	170200
L146;	24;	000000
L147;	24;	000001
L148;	24;	000002
L149;	24;	000003
L150;	24;	000004
L151;	24;	000005

L152;	30;	070000
L153;	30;	071000
L154;	30;	072000
L155;	30;	073000
L156;	07;	074000
L157;	15;	006700
L158;	11;	006400
L159;	31;	077000

/ pseudo ops

L160;	16;	000000
L161;	20;	000000
L162;	21;	000000
L163;	22;	000000
L164;	23;	000000
L165;	25;	000000
L166;	26;	000000
L167;	27;	000000
L168;	32;	000000

ebsymtab:

L1:	<.\0>
L2:	<..\0>
L3:	<r0\0>
L4:	<r1\0>
L5:	<r2\0>
L6:	<r3\0>
L7:	<r4\0>
L8:	<r5\0>
L9:	<sp\0>
L10:	<pc\0>
L11:	<mov\0>
L12:	<movb\0>
L13:	<cmp\0>
L14:	<cmpb\0>
L15:	<bit\0>
L16:	<bitb\0>
L17:	<bic\0>
L18:	<bicb\0>
L19:	<bis\0>
L20:	<bisb\0>
L21:	<add\0>
L22:	<sub\0>
L23:	<br\0>
L24:	<bne\0>
L25:	<beq\0>
L26:	<bge\0>
L27:	<blt\0>
L28:	<bgt\0>
L29:	<ble\0>
L30:	<bpl\0>
L31:	<bmi\0>
L32:	<bhi\0>
L33:	<blos\0>
L34:	<bvc\0>
L35:	<bvs\0>
L36:	<bhis\0>
L37:	<bec\0>
L38:	<bcc\0>
L39:	<blo\0>
L40:	<bcs\0>
L41:	<bes\0>
L42:	<jbr\0>
L43:	<jne\0>
L44:	<jeq\0>
L45:	<jge\0>
L46:	<jlt\0>
L47:	<jgt\0>
L48:	<jle\0>
L49:	<jpl\0>
L50:	<jmi\0>
L51:	<jhi\0>
L52:	<jlos\0>
L53:	<jvc\0>
L54:	<jvs\0>
L55:	<jhis\0>
L56:	<jec\0>
L57:	<jcc\0>
L58:	<jlo\0>
L59:	<jcs\0>
L60:	<jes\0>
L61:	<clr\0>
L62:	<clrb\0>
L63:	<com\0>
L64:	<comb\0>
L65:	<inc\0>
L66:	<incb\0>
L67:	<dec\0>
L68:	<decb\0>
L69:	<neg\0>
L70:	<negb\0>
L71:	<adc\0>
L72:	<adcb\0>
L73:	<sbc\0>
L74:	<sbcb\0>
L75:	<tst\0>
L76:	<tstb\0>
L77:	<ror\0>
L78:	<rorb\0>
L79:	<rol\0>
L80:	<rolb\0>
L81:	<asr\0>
L82:	<asrb\0>
L83:	<asl\0>
L84:	<aslb\0>
L85:	<jmp\0>
L86:	<swab\0>
L87:	<mfpi\0>
L88:	<mtpi\0>
L89:	<mfpd\0>
L90:	<mtpd\0>
L91:	<stst\0>
L92:	<mfps\0>
L93:	<mtps\0>
L94:	<csm\0>
L95:	<tstset\0>
L96:	<wrtlck\0>
L97:	<jsr\0>
L98:	<rts\0>
L99:	<sys\0>
L102:	<spl\0>
L103:	<nop\0>
L104:	<clc\0>
L105:	<clv\0>
L106:	<clz\0>
L107:	<cln\0>
L108:	<ccc\0>
L109:	<sec\0>
L110:	<sev\0>
L111:	<sez\0>
L112:	<sen\0>
L113:	<scc\0>
L114:	<halt\0>
L115:	<wait\0>
L116:	<rti\0>
L117:	<bpt\0>
L118:	<iot\0>
L119:	<reset\0>
L120:	<rtt\0>
L121:	<mfpt\0>
L122:	<cfcc\0>
L123:	<setf\0>
L124:	<setd\0>
L125:	<seti\0>
L126:	<setl\0>
L127:	<clrf\0>
L128:	<negf\0>
L129:	<absf\0>
L130:	<tstf\0>
L131:	<movf\0>
L132:	<movif\0>
L133:	<movfi\0>
L134:	<movof\0>
L135:	<movfo\0>
L136:	<addf\0>
L137:	<subf\0>
L138:	<mulf\0>
L139:	<divf\0>
L140:	<cmpf\0>
L141:	<modf\0>
L142:	<movie\0>
L143:	<movei\0>
L144:	<ldfps\0>
L145:	<stfps\0>
L146:	<fr0\0>
L147:	<fr1\0>
L148:	<fr2\0>
L149:	<fr3\0>
L150:	<fr4\0>
L151:	<fr5\0>
L152:	<mul\0>
L153:	<div\0>
L154:	<ash\0>
L155:	<ashc\0>
L156:	<xor\0>
L157:	<sxt\0>
L158:	<mark\0>
L159:	<sob\0>
L160:	<.byte\0>
L161:	<.even\0>
L162:	<.if\0>
L163:	<.endif\0>
L164:	<.globl\0>
L165:	<.text\0>
L166:	<.data\0>
L167:	<.bss\0>
L168:	<.comm\0>
	.text

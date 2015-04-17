	.title	at.sml	-   assembler/translator system macros
	; @(#)at.sml	1.3 11/3/82

	.ident	/10may4/

	.macro	always		;all files of macro

	.macro	.data
	entsec	.data
	.endm	.data

	.macro	.text
	entsec	.text
	.endm

	.macro	.bss
	entsec	.bss
	.endm

mk.symbol=1			;one to make symbols, 0 otherwise
x40=	0
pdpv45=	0			; host machine has 'mul', 'div', sob' instrucs.
				; if not you will have to write macros for them
$timdf=	7			; California Time Zone
				; should really use ftime(2) for this and for
				; DST.
;xfltg=	0		;define to assmbl out floating hardware
rsx11d	=	0	; rsx11d features 
debug	=	0	; <<< REEDS if non zero includes debug junk

ft.id=	1			;have set i & d.  set =0 if not

ft.unx = 1			; this macro-11 is for UNIX.  =0 if not.

	.nlist	bex

tab=	11
lf=	12
vt=	13
ff=	14
cr=	15
space=	40

bpmb	=	20		;bytes per macro block





	.psect	.text	con, shr, gbl,ins
	.psect	.data	con, dat, prv, gbl
	.psect	.bss	con, bss, gbl

	.psect	dpure	con, dat, prv, gbl
	.psect	mixed	con, prv, gbl
	.psect	errmes	con, dat, prv, gbl
	.psect	impure	con, bss, gbl
	.psect	imppas	con, bss, gbl
	.psect	implin	con, bss, gbl
	.psect	swtsec	con, dat, prv, gbl	; unix command line flags
	.psect	cndsec 	con, dat, prv, gbl	; gt, le, equ, etc.  for '.if'
	.psect	crfsec 	con, dat, prv, gbl	; args for -cr flag
	.psect	edtsec 	con, dat, prv, gbl	; args for .enabl
	.psect	lctsec 	con, dat, prv, gbl	; args for .list
	.psect	psasec 	con, dat, prv, gbl
	.psect	pstsec 	con, dat, prv, gbl
	.psect	rolbas 	con, dat, prv, gbl	; core allocation: starts of tables
	.psect	rolsiz 	con, dat, prv, gbl	; sizes of table entries
	.psect	roltop 	con, dat, prv, gbl	; tops of tables
	.psect	xpcor	con,bss	, gbl	; this one MUST come last in core


	.macro	entsec	name 	;init a section
	.psect	name	con
	.endm	entsec



	.macro jeq	x,?fred
	bne	fred
	jmp	x
fred:
	.endm
	.macro	jne	x,?fred
	beq	fred
	jmp	x
fred:
	.endm
	.macro	xitsec
	entsec	.text
	.endm	xitsec


	.macro	call	address
	jsr	pc,address
	.endm

	.macro	return
	rts	pc
	.endm


	.macro	always
	.nlist	bex
	.endm	always
	.endm	always


	.if ne debug
	
	.macro	ndebug n
	.globl	ndebug,..z
	mov	n,..z
	call	ndebug
	.endm

	.macro	sdebug	string
	.globl	sdebug,..z,..zbuf
	x = 0
	.irpc	t,<string>
	movb	#''t,..zbuf+x
	x = x+1
	.endm
	movb	#0,..zbuf+x
	mov	#..zbuf,..z
	call	sdebug
	.endm

	.iff
	
	.macro	ndebug n
	.endm

	.macro	sdebug	string
	.endm
	
	.endc
	
	
	.macro	param	mne,	value	;define default parameters
	.iif ndf mne,	mne=	value
	.list
mne=	mne
	.nlist
	.endm
	.macro	putkb	addr	;list to kb
	.globl	putkb
	mov	addr,r0
	call	putkb
	.endm

	.macro	putlp	addr	;list to lp
	.globl	putlp
	mov	addr,r0
	call	putlp
	.endm

	.macro	putkbl	addr	;list to kb and lp
	.globl	putkbl
	mov	addr,r0
	call	putkbl
	.endm


	.macro	xmit	wrdcnt	;move small # of words
	.globl	xmit0
	call	xmit0-<wrdcnt*2>
	.endm	xmit


;the macro "genswt" is used to specify  a command
;string switch (1st argument) and the address of
;the routine to be called when encountered (2nd arg).
; the switch is made upper-case.

	.macro	genswt	mne,addr,?label
	entsec	swtsec
label:	.irpc	x,mne
	.if ge ''x-141
		.if le ''x-172
			.byte ''x-40
		.iff
			.byte ''x
		.endc
	.iff
	.byte ''x
	.endc
	.endm
	.iif ne <.-label&1>,	.byte	0
	.word	addr
	xitsec
	.endm
	.macro	zread	chan
	.globl	zread
	mov	#chan'chn,r0
	call	zread
	.endm	zread

	.macro	zwrite	chan
	.globl	zwrite
	mov	#chan'chn,r0
	call	zwrite
	.endm	zwrite
	.macro	genedt	mne,subr	;gen enable/disable table
	entsec	edtsec
	.rad50	/mne/
	.if nb	subr
	.word	subr
	.iff
	.word	cpopj
	.endc
	.word	ed.'mne
	xitsec
	.endm	genedt


;the macro "gencnd" is used to specify conditional
;arguments.  it takes two or three arguments:

;	1-	mnemonic
;	2-	subroutine to be called
;	3-	if non-blank, complement condition

	.macro	gencnd	mne,subr,toggle	;generate conditional
	entsec	cndsec
	.rad50	/mne/
	.if b	<toggle>
	.word	subr
	.iff
	.word	subr+1
	.endc
	xitsec
	.endm
	.macro	ch.mne

ch.ior=	'!
ch.qtm=	'"
ch.hsh=	'#
ch.dol=	'$
ch.pct=	'%
ch.and=	'&
ch.xcl=	''

ch.lp=	'(
ch.rp=	')
ch.mul=	'*
ch.add=	'+
ch.com=	',
ch.sub=	'-
ch.dot=	'.
ch.div=	'/

ch.col=	':
ch.smc=	';
ch.lab=	'<
ch.equ=	'=
ch.rab=	'>
ch.qm=	'?

ch.ind=	'@
ch.bsl=	'\
ch.uar=	'^

let.a=	'a&^c40
let.b=	'b&^c40
let.c=	'c&^c40
let.d=	'd&^c40
let.e=	'e&^c40
let.f=	'f&^c40
let.g=	'g&^c40
let.o=	'o&^c40
let.p=	'p&^c40
let.r=	'r&^c40
let.z=	'z&^c40

dig.0=	'0
dig.9=	'9
	.macro	ch.mne
	.endm	ch.mne
	.endm	ch.mne

	.macro error num,arg, mess ,?x
	sdebug	<num>
	.globl	err.'arg,ern'num, errbts,errref
	.if	b	<mess>
	deliberate error mistake
	.endc
	.if	dif	0,num
	.globl	err.xx
	tst	err.xx
	bne	x
	mov	#ern'num,err.xx
x:
	.endc
	bis	#err.'arg,errbts
	.endm



	.macro	setnz	addr	;set addr to non-zero for t/f flags
	mov	sp,addr
	.endm


	.macro	bisbic	arg	; used by .list/.nlist, .enabl/.dsabl
	.globl	bisbic
	mov	#arg,-(sp)
	call	bisbic
	tst	(sp)+
	.endm
				;roll handler calls

	.macro	search	rolnum	;binary search
	mov	#rolnum,r0
	.globl	search
	call	search
	.endm

	.macro	scan	rolnum	;linear scan
	mov	#rolnum,r0
	.globl	scan
	call	scan
	.endm

	.macro	scanw	rolnum	;linear scan, one word
	mov	#rolnum,r0
	.globl	scanw
	call	scanw
	.endm

	.macro	next	rolnum	;fetch next entry
	mov	#rolnum,r0
	.globl	next
	call	next
	.endm

	.macro	append	rolnum	;append to end of roll
	mov	#rolnum,r0
	.globl	append
	call	append
	.endm

	.macro	zap	rolnum	;clear roll
	mov	#rolnum,r0
	.globl	zap
	call	zap
	.endm

;	call	insert		;insert (must be preceded by one 
				;of the above to set pointers)
;	call	setrol		;save and set regs for above
;flags used in symbol table mode

	.macro	st.flg

.if le ft.unx

ovrflg=	000004		;overlay (psect only)
defflg=	000010		;defined
relflg=	000040		;relocatable
glbflg=	000100		;global
dfgflg= 000200		; default global <rsx11d>... reeds's guess


.endc

.if gt ft.unx

			; ****** these should not be changed!! ******
shrflg=	000001		;shareable (psect only)
.if gt ft.id
insflg=	shrflg*2	;instruction space (psect only)
bssflg=	insflg*2	;blank section (psect only)
m.idf=	shrflg!insflg!bssflg	;mask to turn them off
.iff
bssflg=	shrflg*2
m.idf=	shrflg!bssflg
.endc
b.idf=	1		;shift count to make above bits word offset
			; ***********************************
defflg=	000010		;defined
ovrflg=	000020		;overlay (psect only)
relflg=	000040		;relocatable
glbflg=	000100		;global
dfgflg= 000200		; default global <rsx11d>... reeds's guess

.endc

;
; default psect attribs.
; can be changed, but make sure all customers know about
; it, including all the linkers.
;
pattrs=relflg!defflg		; For .psects and blank .csects
aattrs=glbflg!defflg!ovrflg		; For .asect
cattrs=glbflg!relflg!defflg!ovrflg	; For named .csects

regflg=	000001		;register
lblflg=	000002		;label
mdfflg=	000004		;multilpy defined
	.macro	st.flg
	.endm
	.endm	st.flg



	.macro	ct.mne
	.globl	cttbl
ct.eol	=	000		; eol
ct.com	=	001		; comma
ct.tab	=	002		; tab
ct.sp	=	004		; space
ct.pcx	=	010		; printing character
ct.num	=	020		; numeric
ct.alp	=	040		; alpha, dot, dollar
ct.lc	=	100		; lower case alpha
ct.smc	=	200		; semi-colon (sign bit)

ct.pc	=	ct.com!ct.smc!ct.pcx!ct.num!ct.alp
	.macro	ct.mne
	.endm	ct.mne
	.endm	ct.mne


	.end

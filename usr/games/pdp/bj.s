/ bj -- black jack

.globl _signal
	mov	$prtot, -(sp)
	mov	$2,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
	jsr	r5,mesg; <Black Jack\n\0>; .even
	mov	sp,gsp
	clr	r0
1:
	movb	r0,deck(r0)
	inc	r0
	cmp	r0,$52.
	blo	1b

game:
	mov	gsp,sp
	jsr	r5,mesg; <new game\n\0>; .even
	clr	dbjf
	add	taction,action
	clr	taction
	jsr	pc,shuffle
	jsr	r5,quest5
	mov	total,stotal
	jsr	pc,card
	mov	cval,upval
	mov	aceflg,upace
	jsr	pc,peek
	mov	r0,dhole
	mov	cval,r0
	add	upval,r0
	cmp	r0,$21.
	bne	1f
	inc	dbjf
1:
	jsr	r5,mesg; < up\n\0>; .even
	jsr	pc,card
	mov	cval,ival
	mov	aceflg,iace
	mov	$'+,r0
	jsr	pc,putc
	mov	$-1,-(sp)
	mov	$-1,-(sp) / safety
	clr	dflag
	br 	1f

doubl:
	cmp	dflag,$1
	beq	2f
	jmp	dmove
2:
	inc	dflag
1:
	mov	ival,value
	mov	iace,naces
	mov	$1,fcard
	mov	bet,-(sp)

hit:
	jsr	pc,card
	jsr	pc,nline
	jsr	pc,nline
	add	cval,value
	add	aceflg,naces
	dec	fcard
	bge	2f
	jmp	1f
2:
	add	bet,taction
	tst	dflag
	bne	2f
	tst	upace
	beq	2f
	jsr	r5,mesg; <Insurance? \0>; .even
	jsr	r5,quest1
		br 2f
	mov	bet,r0
	add	r0,total
	asr	r0
	add	r0,taction
	tst	dbjf
	bne	2f
	sub	r0,total
	asl	r0
	sub	r0,total
2:
	tst	dflag
	beq	2f
	cmp	ival,$11.
	beq	3f
2:
	cmp	value,$21.
	bne	2f
	tst	dflag
	bne	2f
	jsr	r5,mesg; <You have black jack!\n\0>; .even
	tst	(sp)+
	tst	dbjf
	bne	doubl
	mov	bet,r0
	add	r0,total
	asr	r0
	add	r0,total
	br	doubl
2:
	tst	dbjf
	bne	3f
	cmp	cval,ival
	bne	2f
	tst	dflag
	bne	2f
	jsr	r5,mesg; <Split pair? \0>; .even
	jsr	r5,quest2
		br 2f
	inc	dflag
	tst	(sp)+
	br	1b
2:
	cmp	value,$10.
	blo	1f
	cmp	value,$11.
	bhi	1f
	jsr	r5,mesg; <Double down? \0>; .even
	jsr	r5,quest3
		br 1f
	add	(sp),(sp) / double bet
	add	bet,taction
	jsr	pc,card
	jsr	pc,nline
	add	cval,value
3:
	cmp	value,$21.
	blos	stick
	sub	$10.,value
	br	stick
1:
	cmp	value,$21.
	blos	1f
	dec	naces
	bge	2f
	jsr	r5,mesg; <You bust\n\0>
	sub	(sp)+,total
	jmp	doubl
2:
	sub	$10.,value
	br	1b
1:
	jsr	r5,mesg; <Hit? \0>; .even
	jsr	r5,quest4
		br stick
	jmp	hit

stick:
	tst	dbjf
	bne	1f
	jsr	r5,mesg; <You have \0>; .even
	mov	value,r0
	jsr	pc,decml
	jsr	pc,nline
1:
	mov	value,-(sp)
	jmp	doubl

dmove:
	tst	(sp)
	blt	addin
	jsr	r5,mesg; <Dealer has \0>; .even
	mov	dhole,peekf

dhit:
	jsr	pc,card
	add	cval,upval
	add	aceflg,upace
	tst	dbjf
	beq	1f
	jsr	r5,mesg; < = blackjack\n\0>; .even
	mov	$22.,upval
	br	addin
1:
	cmp	upval,$21.
	blos	1f
	dec	upace
	bge	2f
	jsr	r5,mesg; < = bust\n\0>; .even
	clr	upval
	br	addin
2:
	sub	$10.,upval
	br	1b
1:
	cmp	upval,$17.
	bhis	1f
	mov	$'+,r0
	jsr	pc,putc
	br	dhit
1:
	jsr	r5,mesg;< = >; .even
	mov	upval,r0
	jsr	pc,decml
	jsr	pc,nline

addin:
	mov	(sp)+,r0
	blt	2f
	cmp	upval,r0
	bne	1f
	tst	(sp)+
	br	addin
1:
	blt	1f
	sub	(sp)+,total
	br	addin
1:
	add	(sp)+,total
	br	addin
2:
	mov	total,r0
	sub	stotal,r0
	jsr	pc,dollars
	jmp	game

shuffle:
	cmp	mod,$15.
	bhis	1f
	jsr	r5,mesg; <Shuffle\n\0>; .even
	jsr	pc,pract
	mov	$52.,mod
	mov	$16.,tens
	mov	$36.,others
1:
	rts	pc

card:
	jsr	pc,peek
	mov	cden,r0
	jsr	pc,putc
	mov	csuit,r0
	jsr	pc,putc
	cmp	cval,$10.
	beq	1f
	dec	others
	rts	pc
1:
	dec	tens
	rts	pc

peek:
	mov	peekf,-(sp)
	bne	1f
	cmp	mod,$1
	bgt	2f
	jsr	pc,shuffle
2:
	jsr	r5,rand; mod: 0
	dec	mod
	movb	deck(r0),(sp)
	mov	mod,r1
	movb	deck(r1),deck(r0)
	movb	(sp),deck(r1)
	inc	(sp)
1:
	clr	peekf
	mov	(sp),r1
	dec	r1
	clr	r0
	div	$4.,r0
	movb	denom(r0),cden
	movb	suit(r1),csuit
	clr	aceflg
	tst	r0
	bne	1f
	inc	aceflg
	mov	$11.,r0
	br	2f
1:
	inc	r0
	cmp	r0,$10.
	blos	2f
	mov	$10.,r0
2:
	mov	r0,cval
	mov	(sp)+,r0
	rts	pc

prtot:
	jsr	pc,nline
	jsr	pc,pract
	jmp	done

pract:
	tst	action
	beq	1f
	jsr	r5,mesg; <Action $\0>; .even
	mov	action,r0
	jsr	pc,decml
	mov	total,r0
	jsr	pc,dollars
1:
	rts	pc

dollars:
	mov	r0,-(sp)
	blt	1f
	bne	2f
	jsr	r5,mesg; <\nYou break even\0>; .even
	tst	(sp)+
	br	3f
2:
	jsr	r5,mesg; <\nYou win $\0>; .even
	br	2f
1:
	jsr	r5,mesg; <\nYou lose $\0>; .even
	neg	(sp)
2:
	mov	(sp)+,r0
	jsr	pc,decml
3:
	jsr	pc,nline
	rts	pc

denom:	<A23456789TJQK>
suit:	<HSDC>
deck:	.=.+52.
	.even

cval:	.=.+2
aceflg:	.=.+2
cden:	.=.+2
csuit:	.=.+2
peekf:	.=.+2
dhole:	.=.+2
dbjf:	.=.+2
upval:	.=.+2
upace:	.=.+2
value:	.=.+2
taction:	.=.+2
action:	.=.+2
naces:	.=.+2
ival:	.=.+2
iace:	.=.+2
gsp:	.=.+2
dflag:	.=.+2
fcard:	.=.+2
stotal:	.=.+2
total:	.=.+2


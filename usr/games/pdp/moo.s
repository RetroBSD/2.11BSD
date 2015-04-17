/ moo

	jsr	r5,mesg; <MOO\n\0>; .even

game:
	jsr	r5,mesg; <new game\n\0>; .even
	mov	$act,r1
1:
	movb	$-1,(r1)+
	cmp	r1,$act+4.
	bne	1b
	mov	$act,r1
1:
	jsr	r5,rand; 10.
	mov	$act,r2
2:
	cmpb	r0,(r2)+
	beq	1b
	cmp	r2,$act+4
	bne	2b
	movb	r0,(r1)+
	cmp	r1,$act+4
	bne	1b
	clr	nguess
	jmp	loop

error:
	jsr	r5,mesg; <bad guess\n\0>; .even
	mov	$ibuf,r1
1:
	cmpb	(r1)+,$'\n
	beq	loop
	cmp	r1,$ibuf+5
	bne	1b
1:
	jsr	pc,flush

loop:
	clr	ncows
	clr	nbulls
	jsr	r5,mesg; <? \0>; .even
	clr	r0
	mov	$ibuf,r1
1:
	movb	$-1,(r1)+
	cmp	r1,$ibuf+5
	bne	1b
	mov	$ibuf,r1
1:
	jsr	pc,getc
	movb	r0,(r1)
	sub	$'0,r0
	cmp	r0,$10.
	bhis	error
	mov	$ibuf,r2
2:
	cmpb	r0,(r2)+
	beq	error
	cmp	r2,$ibuf+4
	bne	2b
	movb	r0,(r1)+
	mov	$act,r2
2:
	cmpb	r0,(r2)+
	bne	3f
	inc	ncows
3:
	cmp	r2,$act+4
	bne	2b
	cmp	r1,$ibuf+4
	bne	1b
	jsr	pc,getc
	cmp	r0,$'\n
	bne	error
	mov	$ibuf,r1
	mov	$act,r2
1:
	cmpb	(r1)+,(r2)+
	bne	2f
	inc	nbulls
	dec	ncows
2:
	cmp	r1,$ibuf+4
	bne	1b
	mov	nbulls,r0
	jsr	pc,decml
	jsr	r5,mesg; < bulls; \0>; .even
	mov	ncows,r0
	jsr	pc,decml
	jsr	r5,mesg; < cows\n\0>; .even
	cmp	nbulls,$4
	beq	1f
	inc	nguess
	jmp	loop
1:
	mov	nguess,r0
	jsr	pc,decml
	jsr	r5,mesg; < guesses\n\n\0>; .even
	jmp	game

act:	.=.+4
ibuf:	.=.+6
ncows:	.=.+2
nbulls:	.=.+2
nguess:	.=.+2


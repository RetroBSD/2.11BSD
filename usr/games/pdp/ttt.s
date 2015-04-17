/ ttt -- learning tic-tac-toe

	jsr	r5,mesg; <Tic-Tac-Toe\n\0>; .even
	jsr	r5,mesg; <Accumulated knowledge? \0>; .even
	jsr	r5,quest
		br 1f
.globl	_open
.globl	_read
.globl	_close
	mov	$0,-(sp)
	mov	$ttt.k,-(sp)
	jsr	pc,_open
	cmp	(sp)+,(sp)+
	bes	1f
	mov	r0,r1
	mov	$1000.,-(sp)
	mov	$badbuf,-(sp)
	mov	r0,-(sp)
	jsr	pc,_read
	add	$6,sp
	mov	r1,-(sp)
	jsr	pc,_close
	tst	(sp)+
1:
	mov	$badbuf,r0
1:
.globl	_signal
	tst	(r0)+
	bne	1b
	tst	-(r0)
	mov	r0,badp
	sub	$badbuf,r0
	asr	r0
	jsr	pc,decml
	jsr	r5,mesg; < 'bits' of knowledge\n\0>; .even
	mov	$addno,-(sp)
	mov	$2,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+

game:
	jsr	r5,mesg; <new game\n\0>; .even

/ initialize board

	mov	$singbuf,singp
	mov	$board,r0
1:
	clrb	(r0)+
	cmp	r0,$board+9
	blo	1b

loop:

/ print board

	mov	$board,r1
1:
	movb	(r1)+,r0
	beq	3f
	dec	r0
	beq	2f
	mov	$'X,r0
	br	4f
2:
	mov	$'O,r0
	br	4f
3:
	mov	r1,r0
	sub	$board-'0,r0
4:
	jsr	pc,putc
	cmp	r1,$board+3
	beq	2f
	cmp	r1,$board+6
	beq	2f
	cmp	r1,$board+9
	bne	1b
2:
	jsr	pc,nline
	cmp	r1,$board+9
	blo	1b
	jsr	pc,nline

/ get bad move

	jsr	r5,mesg; <? \0>; .even
	jsr	pc,getc
	cmp	r0,$'\n
	beq	1f
	sub	$'1,r0
	cmp	r0,$8
	bhi	badmov
	tstb	board(r0)
	bne	badmov
	mov	r0,r1
	jsr	pc,getc
	cmp	r0,$'\n
	bne	badmov
	movb	$2,board(r1)
1:

/ check if he won

	jsr	r5,check; 6.
		br loose

/ check if cat game

	jsr	pc,catgam

/ select good move

	clr	ntrial
	clr	trial
	mov	$board,r4
1:
	tstb	(r4)
	bne	2f
	movb	$1,(r4)
	jsr	r5,check; 3
		br win
	jsr	pc,getsit
	clrb	(r4)
	mov	$badbuf,r1
3:
	cmp	r1,badp
	bhis	3f
	cmp	r0,(r1)+
	bne	3b
	br	2f
3:
	cmp	r0,trial
	beq	2f
	blo	3f
	mov	r0,trial
	mov	r4,move
3:
	inc	ntrial
2:
	inc	r4
	cmp	r4,$board+9
	blo	1b

/ install move

	tst	ntrial
	beq	conced
1:
	cmp	ntrial,$1
	beq	1f
	mov	$singbuf,singp
1:
	mov	singp,r0
	mov	trial,(r0)+
	mov	r0,singp
	mov	move,r0
	movb	$1,(r0)

	jmp	loop

badmov:
	jsr	r5,mesg; <Illegal move\n\0>; .even
	jsr	pc,flush
	jmp	loop

loose:
	jsr	r5,mesg; <You win\n\0>; .even
	br	1f

conced:
	jsr	r5,mesg; <I concede\n\0>; .even
1:
	mov	badp,r1
	mov	$singbuf,r2
1:
	cmp	r2,singp
	bhis	1f
	mov	(r2)+,(r1)+
	br	1b
1:
	mov	r1,badp
	jmp	game

win:
	jsr	r5,mesg; <I win\n\0>; .even
	jmp	game

getsit:
	mov	$maps,r2
	clr	r3
1:
	mov	$9,r1
	mov	r5,-(sp)
	clr	r5
2:
	mul	$3,r5
	movb	(r2)+,r0
	movb	board(r0),r0
	add	r0,r5
	sob	r1,2b
	mov	r5,r0
	mov	(sp)+,r5
	cmp	r0,r3
	blo	2f
	mov	r0,r3
2:
	cmp	r2,$maps+[9*8]
	blo	1b
	mov	r3,r0
	rts	pc

catgam:
	mov	$board,r0
1:
	tstb	(r0)+
	beq	1f
	cmp	r0,$board+9
	blo	1b
	jsr	r5,mesg; <Draw\n\0>; .even
	jmp	game
1:
	rts	pc

check:
	mov	$wins,r1
1:
	jsr	pc,1f
	mov	r0,r2
	jsr	pc,1f
	add	r0,r2
	jsr	pc,1f
	add	r0,r2
	cmp	r2,(r5)
	beq	2f
	cmp	r1,$wins+[8*3]
	blo	1b
	tst	(r5)+
2:
	tst	(r5)+
	rts	r5
1:
	movb	(r1)+,r0
	movb	board(r0),r0
	bne	1f
	mov	$10.,r0
1:
	rts	pc

addno:
.globl	_open
.globl	_read
.globl	_close
	mov	$0,-(sp)
	mov	$ttt.k,-(sp)
	jsr	pc,_open
	cmp	(sp)+,(sp)+
	bes	1f
	mov	r0,r1
	mov	$1000.,-(sp)
	mov	badp,-(sp)
	mov	r0,-(sp)
	jsr	pc,_read
	add	$6,sp
	mov	r1,-(sp)
	jsr	pc,_close
	tst	(sp)+
1:
	mov	$badbuf,r1
	mov	r1,r2
1:
	mov	(r2)+,r0
	beq	1f
	blt	1b
	mov	$badbuf,r3
2:
	tst	(r3)
	beq	2f
	cmp	r0,(r3)+
	bne	2b
	mov	$-1,-2(r3)
	br	2b
2:
	mov	r0,(r1)+
	br	1b
1:
.globl	_creat
.globl	_write
	sub	$badbuf,r1
	mov	r1,0f
	jsr	pc,nline
	mov	r1,r0
	asr	r0
	jsr	pc,decml
	jsr	r5,mesg; < 'bits' returned\n\0>; .even
	mov	$644,-(sp)
	mov	$ttt.k,-(sp)
	jsr	pc,_creat
	cmp	(sp)+,(sp)+
	mov	0f,-(sp)
	mov	$badbuf,-(sp)
	mov	r0,-(sp)
	jsr	pc,_write
	add	$6,sp
	jmp	done

0:	..

maps:
	.byte	0,1,2,3,4,5,6,7,8
	.byte	6,3,0,7,4,1,8,5,2
	.byte	8,7,6,5,4,3,2,1,0
	.byte	2,5,8,1,4,7,0,3,6
	.byte	2,1,0,5,4,3,8,7,6
	.byte	8,5,2,7,4,1,6,3,0
	.byte	6,7,8,3,4,5,0,1,2
	.byte	0,3,6,1,4,7,2,5,8

wins:
	.byte	0,1,2
	.byte	3,4,5
	.byte	6,7,8
	.byte	0,3,6
	.byte	1,4,7
	.byte	2,5,8
	.byte	0,4,8
	.byte	2,4,6

ttt.k:	<ttt.k\0>

board:	.=.+9
	.even
singp:	.=.+2
singbuf:.=.+18.
badp:	.=.+2
badbuf:	.=.+1000.
ntrial:	.=.+2
trial:	.=.+2
move:	.=.+2


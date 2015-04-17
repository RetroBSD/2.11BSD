/ etc.s -- common code

.globl	_signal
	mov	$done,-(sp)
	mov	$2,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
	jmp	around

done:
.globl	_exit
	jsr	pc,nline
	jsr	pc,_exit

mesg:
	movb	(r5)+,r0
	beq	1f
	jsr	pc,putc
	br	mesg
1:
	inc	r5
	bic	$1,r5
	rts	r5

quest:
	jsr	pc,getc
	cmp	r0,$'y
	bne	1f
	tst	(r5)+
1:
	cmp	r0,$'\n
	beq	1f
	jsr	pc,flush
1:
	rts	r5

getc:
.globl	_read
	mov	$1,-(sp)
	mov	$ch,-(sp)
	clr	-(sp)
	jsr	pc,_read
	add	$6,sp
	bes	done
	tst	r0
	beq	done
	mov	ch,r0
	rts	pc

0:	0
print:
.globl	_open
	mov	r5,0b
	mov	$0b,-(sp)
	mov	$0,-(sp)
	jsr	pc,_open;
	cmp	(sp)+,(sp)+
	bes	1f
	mov	r0,r1
2:
.globl	_read
	mov	$1,-(sp)
	mov	$ch,-(sp)
	mov	r1,-(sp)
	jsr	pc,_read
	add	$6,sp
	tst	r0
	beq	2f
	mov	ch,r0
	jsr	pc,putc
	br	2b
2:
.globl	_close
	mov	r1,-(sp)
	jsr	pc,_close
	tst	(sp)+
1:
	tstb	(r5)+
	bne	1b
	inc	r5
	bic	$1,r5
	rts	r5

nline:
	mov	$'\n,r0

putc:
.globl	_write
	mov	r0,ch
	mov	$1,-(sp)
	mov	$ch,-(sp)
	mov	$1,-(sp)
	jsr	pc,_write
	add	$6,sp
	rts	pc

decml:
	mov	r1,-(sp)
	jsr	pc,1f
	mov	(sp)+,r1
	rts	pc

1:
	mov	r0,r1
	clr	r0
	div	$10.,r0
	mov	r1,-(sp)
	tst	r0
	beq	1f
	jsr	pc,1b
1:
	mov	(sp)+,r0
	add	$'0,r0
	jsr	pc,putc
	rts	pc

flush:
	jsr	pc,getc
	cmp	r0,$'\n
	bne	flush
	rts	pc

rand:
.globl	_time
	mov	r1,-(sp)
	tst	randx
	bne	1f
	clr	-(sp)
	jsr	pc,_time
	tst	(sp)+
	mov	r1,randx
	bis	$1,randx
1:
	mov	randx,r0
	mul	$15625.,r0
	mov	r1,randx
	ashc	$-2,r0
	clr	r0
	div	(r5)+,r0
	mov	r1,r0
	mov	(sp)+,r1
	rts	r5

ch:	0
randx:	0
around:

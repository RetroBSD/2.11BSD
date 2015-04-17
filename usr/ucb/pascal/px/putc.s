/ putw/putc -- write words/characters on output file

	.globl	_putc, _putw, _fflush, _fcreat
	.globl	_werflg
	.comm	_errno,2

_fcreat:
	mov	r5,-(sp)
	mov	sp,r5
	mov	6(r5),r1
	mov	pc,(r1)		/ a putatively illegal file desc.
	mov	$0644,-(sp)
	mov	4(r5),-(sp)
	jsr	pc,_creat
	cmp	(sp)+,(sp)+
	tst	r0
	bmi	badret
	mov	r0,(r1)+
	clr	(r1)+
	clr	(r1)+
	br	goodret

_putw:
	mov	r5,-(sp)
	mov	sp,r5
	mov	6(r5),r1
	dec	2(r1)
	bge	1f
	jsr	pc,fl
	dec	2(r1)
1:
	movb	4(r5),*4(r1)
	inc	4(r1)
	dec	2(r1)
	bge	1f
	jsr	pc,fl
	dec	2(r1)
1:
	movb	5(r5),*4(r1)
	inc	4(r1)
	mov	4(r5),r0
	br	goodret

_putc:
	mov	r5,-(sp)
	mov	sp,r5
	mov	6(r5),r1
	dec	2(r1)
	bge	1f
	jsr	pc,fl
	dec	2(r1)
1:
	mov	4(r5),r0
	movb	r0,*4(r1)
	inc	4(r1)
	br	goodret

_fflush:
	mov	r5,-(sp)
	mov	sp,r5
	mov	4(r5),r1
	jsr	pc,fl
	br	goodret

fl:
	mov	r1,r0
	add	$6,r0
	mov	r0,-(sp)
	tst	4(r1)
	beq	1f
	mov	4(r1),-(sp)
	sub	r0,(sp)
	mov	r0,-(sp)
	mov	(r1),-(sp)
	jsr	pc,_write
	add	$6,sp
	tst	r0
	bpl	1f
	mov	r0,_werflg
1:
	mov	(sp)+,4(r1)
	mov	$512.,2(r1)
	rts	pc

badret:
	mov	r5,sp
	mov	(sp)+,r5
	rts	pc

goodret:
	clr	_errno
	mov	(sp)+,r5
	rts	pc
.bss
_werflg:.=.+2

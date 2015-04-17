/ qsort interfact to c

/	qsort(from, to)

.globl	_qsort
_qsort:
	mov	2(sp),r1
	mov	4(sp),r2
	jsr	pc,qsort
	rts	pc


qsort:
	mov	r2,r3
	sub	r1,r3
	cmp	r3,$4
	ble	done
	asr	r3
	bic	$3,r3
	add	r1,r3
	mov	r1,-(sp)
	mov	r2,-(sp)

loop:
	cmp	r1,r3
	bhis	loop1
	cmp	(r1),(r3)
	bgt	loop1
	add	$4,r1
	br	loop

loop1:
	cmp	r2,r3
	blos	1f
	sub	$4,r2
	mov	r2,r0
	cmp	(r0),(r3)
	bge	loop1

	mov	(r1),r0
	mov	(r2),(r1)+
	mov	r0,(r2)+
	mov	(r1),r0
	mov	(r2),(r1)
	mov	r0,(r2)
	cmp	-(r1),-(r2)
	cmp	r1,r3
	bne	loop
	mov	r2,r3
	br	loop

1:
	cmp	r1,r3
	beq	1f
	mov	(r1),r0
	mov	(r2),(r1)+
	mov	r0,(r2)+
	mov	(r1),r0
	mov	(r2),(r1)
	mov	r0,(r2)
	cmp	-(r1),-(r2)
	mov	r1,r3
	br	loop1

1:
	mov	(sp)+,r2
	mov	r3,-(sp)
	mov	r3,r1
	add	$4,r1
	jsr	pc,qsort
	mov	(sp)+,r2
	mov	(sp)+,r1
	br	qsort

done:
	rts	pc

	rti = 2

/	itinit()

.globl	_itinit
.globl	_intrp, _term
.globl	_signal

_itinit:
	mov	$1,-(sp)
	mov	$1,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
	bit	$1,r0
	bne	1f
	mov	$_onhup,-(sp)
	mov	$1,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
1:

	mov	$1,-(sp)
	mov	$2,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
	bit	$1,r0
	bne	1f
	mov	$onint,-(sp)
	mov	$2,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
1:
	mov	$1,-(sp)
	mov	$3,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
	rts	pc

.globl	_onhup
_onhup:
	mov	$1,-(sp)
	mov	$1,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
	mov	$1,-(sp)
	mov	$2,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
	mov	$1,-(sp)
	mov	$3,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
	jmp	_term

onint:
	mov	r0,-(sp)
	mov	$onint,-(sp)
	mov	$2,-(sp)
	jsr	pc,_signal
	cmp	(sp)+,(sp)+
	inc	_intrp
	mov	(sp)+,r0
	rts	pc

/	t = clock()

.globl	_clock
.globl	_time
_clock:
	clr	-(sp)
	jsr	pc,_time
	tst	(sp)+
	mov	r0,-(sp)
	mov	r1,-(sp)
	sub	t+2,r1
	sbc	r0
	sub	t,r0
	mov	r1,r0
	mov	(sp)+,t+2
	mov	(sp)+,t
	rts	pc

.bss
t:	.=.+4

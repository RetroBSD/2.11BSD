#if	NRF > 0				/* RF-11 */
.globl	_rfintr
rfio:	jsr	r0,call; jmp _rfintr
#endif

#if	NRF > 0				/* RF-11 */
. = ZERO+204
	rfio; br5
#endif
=================================================================
#if	NRP > 0				/* RP03 */
.globl	_rpintr
rpio:	jsr	r0,call; jmp _rpintr
#endif

#if	NRP > 0				/* RP03 */
. = ZERO+254
	rpio; br5
#endif
=================================================================
#if	NDVHP > 0			/* Diva Comp V */
.globl	_dvhpintr
dvhpio:	jsr	r0,call; jmp _dvhpintr
#endif

#if	NDVHP > 0			/* Diva Comp V */
. = ZERO+254
	dvhpio; br 5
#endif
=================================================================
#if	NCAT > 0			/* GP DR11C driver used for C/A/T */
.globl	_ctintr
ctio:	jsr	r0,call; jmp _ctintr
#endif
=================================================================
#if	NDN > 0				/* DN-11 ACU */
.globl	_dnint
dnou:	jsr	r0,call; jmp _dnint
#endif
=================================================================
#if	NHS > 0				/* RS03/04 */
.globl	_hsintr
hsio:	jsr	r0,call; jmp _hsintr
#endif
=================================================================
#if	NDM > 0				/* DM */
.globl	_dmintr
dmin:	jsr	r0,call; jmp _dmintr
#endif
=================================================================
#if	NRF > 0				/* RF-11 */
.globl	_rfintr
rfio:	jsr	r0,call; jmp _rfintr
#endif

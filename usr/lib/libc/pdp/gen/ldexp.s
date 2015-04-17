/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)ldexp.s	2.3 (Berkeley) 6/12/88\0>
	.even
#endif LIBC_SCCS

/*
 * double
 * ldexp (value, exp)
 *	double value;
 *	int exp;
 *
 * Ldexp returns value*2**exp, if that result is in range.
 * If underflow occurs, it returns zero.  If overflow occurs,
 * it returns a value of appropriate sign and largest
 * possible magnitude.  In case of either overflow or underflow,
 * errno is set to ERANGE.  Note that errno is not modified if
 * no error occurs.  Note also that the code assumes that both
 * FIV and FIU are disabled in the floating point processor:
 * FIV disabled prevents a trap from ocurring on floating overflow;
 * FIU disabled cause a floating zero to be generated on floating underflow.
 */

#include "DEFS.h"

ERANGE	= 34.			/ can't include <errno.h> because of the
.comm	_errno,2		/   comments on each line ... (sigh)

.data
huge:	077777; 0177777; 0177777; 0177777
.text

ENTRY(ldexp)
	movf	2(sp),fr0	/ fr0 = value
	cfcc
	beq	2f		/ done if zero
	movei	fr0,r0		/ r0 = log2(value)
	add	10.(sp),r0	/ add exp to log2(value) and stuff result
	movie	r0,fr0		/   back as new exponent for fr0
	cfcc			/ Overflow?
	bvc	2f
	movf	huge,fr0	/ yes, fr0 = (value < 0.0) ? -huge : huge;
	bpl	1f
	negf	fr0
1:
	mov	$ERANGE,_errno	/   and errno = ERANGE
2:
	rts	pc

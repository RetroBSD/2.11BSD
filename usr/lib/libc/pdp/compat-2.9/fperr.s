/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef SYSLIBC_SCCS
_sccsid: <@(#)fperr.s	2.5 (Berkeley) 1/29/87\0>
	.even
#endif SYSLIBC_SCCS

/*
 * fperr(fpe)
 *	struct fperr *fpe;
 *
 * Get floating point error status from user structure and pass back to
 * caller.
 */
#include "SYS.h"

ENTRY(fperr)
	SYS(fperr)		/ grab floating point exception registers
	mov	r2,-(sp)
	mov	4(sp), r2	/  and pass them back to the caller
	mov	r0, (r2)+
	mov	r1, (r2)
	mov	(sp)+, r2
	rts	pc

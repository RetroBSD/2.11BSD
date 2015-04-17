/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)remque.s	1.2 (Berkeley) 1/20/87\0>
	.even
#endif LIBC_SCCS

/*
 * struct qelem
 * {
 *	struct qelem	*q_forw;
 *	struct qelem	*q_back;
 *	char		q_data[];
 * }
 *
 * remque (elem)
 *	struct qelem  *elem
 *
 * Remove elem from queue.
 */
#include "DEFS.h"

ENTRY(remque)
	mov	2(sp),r0	/ r0 = elem
	mov	(r0),r1		/ r1 = elem->q_forw

	mov	2(r0),r0	/ r0 = elem->q_back
	mov	r1,(r0)		/ elem->q_back->q_forw = elem->forw
	mov	r0,2(r1)	/ elem->q_forw->q_back = elem->back

	rts	pc

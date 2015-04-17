/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef LIBC_SCCS
	<@(#)insque.s	1.2 (Berkeley) 1/20/87\0>
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
 * insque (elem, pred)
 *   struct qelem  *elem, *pred
 *
 * Insert elem into queue with predecessor pred.
 */
#include "DEFS.h"

ENTRY(insque)
	mov	2(sp),r0	/ r0 = elem
	mov	4(sp),r1	/ r1 = pred
	mov	r1, 2(r0)	/ elem->q_back = pred
	mov	(r1),(r0)	/ elem->q_forw = pred->q_forw
	mov	r0,(r1)		/ pred->q_forw = elem
	mov	(r0),r1		/ r1 = elem->q_forw  /* was pred->q_forw */
	mov	r0,2(r1)	/ elem->q_forw->q_back = elem
	rts	pc

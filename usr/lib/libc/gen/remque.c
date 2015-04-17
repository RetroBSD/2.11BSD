/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)remque.c	1.1 (Berkeley) 1/19/87";
#endif LIBC_SCCS and not lint

/*
 * remque -- vax remque instruction
 *
 * NOTE: this implementation is non-atomic!!
 */

struct vaxque {		/* queue format expected by VAX queue instructions */
	struct vaxque	*vq_next;
	struct vaxque	*vq_prev;
};

remque(e)
	register struct vaxque *e;
{
	e->vq_prev->vq_next = e->vq_next;
	e->vq_next->vq_prev = e->vq_prev;
}

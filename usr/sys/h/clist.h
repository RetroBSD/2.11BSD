/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)clist.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * Raw structures for the character list routines.
 */
struct cblock {
	struct cblock *c_next;
	char	c_info[CBSIZE];
};
#if defined(KERNEL) && !defined(SUPERVISOR)
#ifdef UCB_CLIST
extern struct cblock *cfree;
extern memaddr clststrt;
extern u_int clstdesc;		/* PDR for clist segment when mapped */
				/* virt. addr. of clists (0120000 - 0140000) */
#else
extern struct cblock cfree[];
#endif
int	nclist;
struct	cblock *cfreelist;
int	cfreecount;
#endif

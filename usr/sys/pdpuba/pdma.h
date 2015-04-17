/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)pdma.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * Structure to describe pseudo-DMA buffer used by DZ-11 pseudo-DMA
 * routines.  The offsets in the structure are well-known in dzdma,
 * an assembly routine.  We use pd_addr, not p_addr, because proc.h
 * got there first.
 */
struct pdma {
	struct	dzdevice *pd_addr;	/* address of controlling device */
	char	*p_mem;			/* start of buffer */
	char	*p_end;			/* end of buffer */
	struct	tty *p_arg;		/* tty structure for this line */
};

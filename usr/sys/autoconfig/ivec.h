/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ivec.h	1.2 (2.11BSD Berkeley) 12/30/92
 */

struct vec_s {
	int	v_func;
	int	v_br;
} ivec;

#define	IVSIZE	(sizeof ivec)

/*
 * increased from 100 to 128 to allow for vectors to be allocated
 * backwards from 01000 (512) via calls to 'nextiv'
*/
#define	NVECTOR	128		/* max. number of vectors we can set */

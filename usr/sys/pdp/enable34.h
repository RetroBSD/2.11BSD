/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)enable34.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 *	Addresses and bits for Able's ENABLE/34.
 *
 *	All information relevant to the ENABLE/34 is supplied with
 *	the permission of ABLE Computer and may not be disclosed in
 *	any manner to sites not licensed by the University of California
 *	for the Second Berkeley Software Distribution.
 *
 */

#define	ENABLE_SSR3		((u_short *) 0163676)

/* bits in the ENABLE's SSR3 */
#define	ENABLE_SSR3_PARS_ON	0000010
#define	ENABLE_SSR3_UBMAP_ON	0000020

#define	ENABLE_SSR4		((u_short *) 0163674)

/* bits in the ENABLE's SSR4 */
#define	ENABLE_SSR4_MAP_ON	0000001

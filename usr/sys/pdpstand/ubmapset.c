/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ubmapset.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

#include "../h/types.h"

#define UBMAP	((physadr)0170200)

extern char ubmap;

ubmapset()
{
	register unsigned int i;

	if (ubmap)
		for (i = 0; i < 62; i += 2) {
			UBMAP->r[i] = i << 12;
			UBMAP->r[i+1] = i >> 4;
		}
}

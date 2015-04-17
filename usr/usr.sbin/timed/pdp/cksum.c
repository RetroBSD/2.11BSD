
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)cksum.c	2.1 (Berkeley) 6/6/86";
#endif not lint

#include "../globals.h"
#include <protocols/timed.h>

/* computes the checksum for ip packets for a pdp-11 base computer */

in_cksum(w, mlen)
	register u_char *w;		/* known to be r4 */
	register u_int mlen;		/* known to be r3 */
{
	register u_int sum = 0;         /* known to be r2 */

	if (mlen > 0) {
		if (mlen & 01) {
			/*
			 * The segment is an odd length, add the high
			 * order byte into the checksum.
			 */
			sum = in_ckadd(sum,(*w++ << 8));
			mlen--;
		}
		if (mlen > 0)
			in_ckbuf();     /* arguments already in registers */
	}
	return (~sum);
}

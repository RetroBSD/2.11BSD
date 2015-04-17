/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)clock.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

#include "param.h"
#include "clock.h"

clkstart()
{
	register u_short	*lks;

	lks = LKS;
	if (fioword((caddr_t)lks) == -1) {
		lks = KW11P_CCSR;
		if (fioword((caddr_t)lks) == -1) {
			printf("no clock!!");
			return;
		}
		*KW11P_CCNT = 0;
		*KW11P_CCSB = 1;
		*lks = K_IENB | K_LFRATE | K_MODE | K_RUN;
	}
	else
		*lks = K_IENB;
}

#if defined(PROFILE) && !defined(ENABLE34)
/*
 * Profiling expects to have a KW11-P in addition to the
 * line-frequency clock, and it should enter at BR7.
 */
isprof()
{
	if (fioword((caddr_t)KW11P_CCSR) == -1)
		panic("no profile clock");
	*KW11P_CCSB = 100;		/* count set = 100 */
	*KW11P_CCSR = K_IENB | K_10KRATE | K_MODE | K_RUN;
}
#endif

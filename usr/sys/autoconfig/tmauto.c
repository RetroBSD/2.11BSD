/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tmauto.c	1.2 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

#include "tmreg.h"

tmprobe(addr,vector)
	struct tmdevice *addr;
{
	extern int errno;

	stuff(TM_IE, &(addr->tmcs));
	errno = 0;			/* check distant register, make */
	grab(&(addr->tmba));		/* sure this isn't a ts-11 */
	return(errno ? ACP_NXDEV : ACP_IFINTR);
}

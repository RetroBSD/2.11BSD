/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)raauto.c	1.2 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

/*
 * This is one of the few devices which could use the 'vector' parameter.
 * Forcing an interrupt from a MSCP device is a large enough undertaking
 * that it hasn't been done (yet).
*/
raprobe(addr,vector)
	u_int *addr;
	int vector;
{
	extern int	errno;

	errno = 0;
	grab(addr);
	return(errno ? ACP_NXDEV : ACP_EXISTS);
}

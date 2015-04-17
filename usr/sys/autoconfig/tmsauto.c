
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tmsauto.c	1.1 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

/*
 * The TMSCP controller is another one which has programmable vectors.
 * The kernel and autoconfigure program have been modified to handle this,
 * which is why the vector is being passed to all of the probe routines.
 * For now just continue testing for the presence of a device until a
 * real probe routine can be written.
*/
tmsprobe(addr,vector)
	u_int *addr;
	{
	extern int	errno;

	errno = 0;
	grab(addr);
	return(errno ? ACP_NXDEV : ACP_EXISTS);
	}

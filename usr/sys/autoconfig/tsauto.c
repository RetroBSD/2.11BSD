/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tsauto.c	2.1 (2.11BSD) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

#include "tsreg.h"

tsprobe(addr,vector)
	register struct tsdevice *addr;
	int vector;
{
	extern int errno;

	/*
	 * Unfortunately the TS and TM CSRs overlap.  So simply testing for
	 * presence of a TS register isn't good enough.  We borrow from
	 * the "universal" tape boot block by poking the controller and
	 * looking for the "need buffer address" bit from a TS.  If that
	 * bit fails to come on the device is not a TS.
	 */
	errno = 0;
	stuff(0, &addr->tssr);		/* poke the controller */
	if (errno)			/* paranoia */
		return(ACP_NXDEV);
	DELAY(100L);			/* give TS time for diagnostics */
	if (grab(&addr->tssr) & TS_NBA)	/* need buffer address bit on? */
		return(ACP_EXISTS);	/* yes = return existence */
	return(ACP_NXDEV);		/* not a TS */
}

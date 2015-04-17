/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rxauto.c	1.2 (2.11BSD GTE) 1995/11/21
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

#include "rxreg.h"

/*
 * rxprobe - check for rx
 */
rxprobe(addr)
	struct rxdevice *addr;
{
	stuff(RX_INIT, (&(addr)->rxcs));
	stuff(RX_IE, (&(addr)->rxcs));
	DELAY(200000L);
	stuff(RX_IE, (&(addr)->rxcs));
	DELAY(1000L);
	stuff(0, (&(addr)->rxcs));
	return(ACP_IFINTR);
}

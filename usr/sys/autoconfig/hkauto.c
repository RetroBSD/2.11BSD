/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)hkauto.c	1.2 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

#include "hkreg.h"

hkprobe(addr,vector)
	struct hkdevice *addr;
	int vector;
{
	stuff(HK_CDT | HK_IE | HK_CRDY, (&(addr->hkcs1)));
	DELAY(10L);
	stuff(HK_CDT, (&(addr->hkcs1)));
	return(ACP_IFINTR);
}

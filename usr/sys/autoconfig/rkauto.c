/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rkauto.c	1.2 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

#include "rkreg.h"

rkprobe(addr,vector)
	struct rkdevice *addr;
	int vector;
{
	stuff(RKCS_IDE | RKCS_DRESET | RKCS_GO, (&(addr->rkcs)));
	DELAY(10L);
	stuff(0, (&(addr->rkcs)));
	return(ACP_IFINTR);
}

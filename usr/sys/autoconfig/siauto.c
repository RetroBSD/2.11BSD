/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)siauto.c	1.2 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

#include "sireg.h"

siprobe(addr,vector)
	struct sidevice *addr;
	int vector;
{
	stuff(SI_IE | SI_READ, &(addr->sicnr));
	stuff(SI_IE | SI_READ | SI_DONE, &(addr->sicnr));
	DELAY(10L);
	stuff(0, &(addr->sicnr));
	return(0);
}

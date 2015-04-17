/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dhauto.c	1.2 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

#include "ioctl.h"
#include "dhreg.h"
#include "dmreg.h"

dhprobe(addr,vector)
	struct dhdevice *addr;
	int vector;
{
	stuff(DH_TIE, &(addr->un.dhcsr));
	DELAY(5L);
	stuff((B9600 << 10) | (B9600 << 6) | BITS7|PENABLE, &(addr->dhlpr));
	stuff(-1, &(addr->dhbcr));
	stuff(0, &(addr->dhcar));
	stuff(1, &(addr->dhbar));
	DELAY(35000L);		/* wait 1/10'th of a sec for interrupt */
	DELAY(35000L);
	stuff(0, &(addr->un.dhcsr));
	return(ACP_IFINTR);
}

dmprobe(addr,vector)
	struct dmdevice *addr;
	int vector;
{
	stuff(grab(&(addr->dmcsr)) | DM_DONE | DM_IE, &(addr->dmcsr));
	DELAY(20L);
	stuff(0, &(addr->dmcsr));
	return(ACP_IFINTR);
}

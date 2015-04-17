/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dzauto.c	1.2 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

#include "dzreg.h"

dzprobe(addr,vector)
	struct dzdevice *addr;
	int vector;
{
	stuff(grab(&(addr->dzcsr)) | DZ_TIE | DZ_MSE, &(addr->dzcsr));
	stuff(1, &(addr->dztcr));
	DELAY(35000L);
	DELAY(35000L);
	stuff(DZ_CLR, &(addr->dzcsr));
	return(ACP_IFINTR);
}

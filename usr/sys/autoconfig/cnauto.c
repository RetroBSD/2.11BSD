/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)cauto.c	1.2 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "cons.h"

cnprobe(addr, vector)
struct dldevice *addr;
int vector;
{
	stuff(grab(&(addr->dlxcsr)) | DLXCSR_TIE, &(addr->dlxcsr));
	DELAY(35000L);
	DELAY(35000L);
	/*
	 *  Leave TIE enabled; cons.c never turns it off
	 *  (and this could be the console).
	 */
	return(ACP_IFINTR);
}

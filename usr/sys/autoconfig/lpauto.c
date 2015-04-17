/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)lpauto.c	1.2 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

/*
 * LP_IE and lpdevice copied from lp.c!!!
 */
#define	LP_IE		0100		/* interrupt enable */

struct lpdevice {
	short	lpcs;
	short	lpdb;
};

lpprobe(addr,vector)
	struct lpdevice	*addr;
	int vector;
{
	stuff(grab(&(addr->lpcs)) | LP_IE, &(addr->lpcs));
	DELAY(10L);
	stuff(0, &(addr->lpcs));
	return(ACP_IFINTR);
}

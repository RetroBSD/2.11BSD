/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)htauto.c	1.2 (2.11BSD GTE) 12/30/92
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

#include "htreg.h"

htprobe(addr,vector)
	struct htdevice *addr;
	int vector;
{
	/*
	 * I can't get the hardware to interrupt when a transport is selected
	 * without doing i/o, so I select transport 0 on driver 7 (that's tape
	 * drive 56).  If you happen to have 56 tape drives on your system and
	 * you boot with it on line, tough.
	 */
	stuff(07, (&(addr->htcs2)));
	stuff(HT_SENSE | HT_IE | HT_GO, (&(addr->htcs1)));
	DELAY(10L);
	stuff(0, (&(addr->htcs1)));
	/*
	 * clear error condition, or driver will report an error first
	 * time you open it after the boot.
	 */
	stuff(HTCS2_CLR, (&(addr->htcs2)));
	return(ACP_IFINTR);
}

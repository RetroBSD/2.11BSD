/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vpauto.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

#include "param.h"
#include "../machine/autoconfig.h"
#include "../machine/machparam.h"

#include "vpreg.h"

vpprobe(addr)
	struct vpdevice *addr;
{
	extern int errno;

	errno = 0;
	/*
	 *  Use the plot csr now, to distinguish from a line printer.
	 */
	stuff(VP_IENABLE | VP_CLRCOM, (&(addr->plcsr)));
	DELAY(10000);
	/*
	 *  Make sure that the DMA registers are there.
	 */
	grab(&(addr->plbcr));
	/*
	 * Write the print csr this time, to leave it in print mode.
	 */
	stuff(0, (&(addr->prcsr)));
	/*
	 * Possibly an LP csr, but no plot regs
	 */
	return(errno ? ACP_NXDEV : ACP_IFINTR);
}

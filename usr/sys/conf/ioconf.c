/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ioconf.c	2.0 (2.11BSD GTE) 12/24/92
 */

#include "param.h"
#include "systm.h"

dev_t	rootdev = %ROOTDEV%,
	swapdev = %SWAPDEV%,
	pipedev = %PIPEDEV%;

dev_t	dumpdev = %DUMPDEV%;
daddr_t	dumplo = (daddr_t)%DUMPLO%;
int	%DUMPROUTINE%();
int	(*dump)() = %DUMPROUTINE%;

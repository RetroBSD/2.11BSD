/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ioconf.c	2.0 (2.11BSD GTE) 12/24/92
 */

#include "param.h"
#include "systm.h"

dev_t	rootdev = makedev(10,0),
	swapdev = makedev(10,1),
	pipedev = makedev(10,0);

dev_t	dumpdev = NODEV;
daddr_t	dumplo = (daddr_t)512;
int	nulldev();
int	(*dump)() = nulldev;

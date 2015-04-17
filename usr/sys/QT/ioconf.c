/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ioconf.c	2.0 (2.11BSD GTE) 12/24/92
 */

#include "param.h"
#include "systm.h"

dev_t	rootdev = makedev(5,0),
	swapdev = makedev(5,1),
	pipedev = makedev(5,0);

dev_t	dumpdev = makedev(5,1);
daddr_t	dumplo = (daddr_t)512;
int	radump();
int	(*dump)() = radump;

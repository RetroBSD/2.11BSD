/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)subr_xxx.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

#include "param.h"
#include "user.h"

/*
 * Routine placed in illegal entries in the bdevsw and cdevsw tables.
 */
nodev()
{

	return (ENODEV);
}

/*
 * Null routine; placed in insignificant entries
 * in the bdevsw and cdevsw tables.
 */
nulldev()
{

	return (0);
}

/*
 * socket(2) and socketpair(2) if networking not available.
 */
#ifndef INET
nonet()
{
	u.u_error = EPROTONOSUPPORT;
	return(EPROTONOSUPPORT);
}
#endif

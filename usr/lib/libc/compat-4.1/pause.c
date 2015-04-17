/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)pause.c	5.2.1 (2.11BSD) 1997/9/9";
#endif LIBC_SCCS and not lint

#include <signal.h>

/*
 * Backwards compatible pause.
 */
pause()
{
	sigset_t set;

	sigemptyset(&set);
	sigsuspend(&set);
}

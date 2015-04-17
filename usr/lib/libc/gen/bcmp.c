/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)bcmp.c	1.1 (Berkeley) 1/19/87";
#endif LIBC_SCCS and not lint

/*
 * bcmp -- vax cmpc3 instruction
 */
bcmp(b1, b2, length)
	register char *b1, *b2;
	register unsigned int length;
{
	if (length)
		do
			if (*b1++ != *b2++)
				break;
		while (--length);
	return(length);
}

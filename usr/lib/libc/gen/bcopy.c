/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)bcopy.c	1.1 (Berkeley) 1/19/87";
#endif LIBC_SCCS and not lint

/*
 * bcopy -- vax movc3 instruction
 */
bcopy(src, dst, length)
	register char *src, *dst;
	register unsigned int length;
{
	if (length && src != dst)
		if (dst < src)
			do
				*dst++ = *src++;
			while (--length);
		else {			/* copy backwards */
			src += length;
			dst += length;
			do
				*--dst = *--src;
			while (--length);
		}
	return(0);
}

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)fputc.c	5.3 (Berkeley) 3/4/87";
#endif LIBC_SCCS and not lint

#include <stdio.h>

fputc(c, fp)
register c;
register FILE *fp;
{
	return(putc(c, fp));
}

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)fgetc.c	5.3 (Berkeley) 3/4/87";
#endif LIBC_SCCS and not lint

#include <stdio.h>

fgetc(fp)
register FILE *fp;
{
	return(getc(fp));
}

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)putchar.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

/*
 * A subroutine version of the macro putchar
 */
#define	USE_STDIO_MACROS
#include <stdio.h>

#undef putchar

putchar(c)
register c;
{
	putc(c, stdout);
}

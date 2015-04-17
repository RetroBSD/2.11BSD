#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)putw.c	5.2.1 (2.11BSD GTE) 1/1/94";
#endif

#include <stdio.h>

putw(w, iop)
register FILE *iop;
{
	register char *p;
	register i;

	p = (char *)&w;
	for (i=sizeof(int); --i>=0;)
		putc(*p++, iop);
	return(ferror(iop));
}

#ifdef pdp11
putlw(w, iop)
long w;
register FILE *iop;
{
	register char *p;
	register i;

	p = (char *)&w;
	for (i=sizeof(long); --i>=0;)
		putc(*p++, iop);
	return(ferror(iop));
}
#endif

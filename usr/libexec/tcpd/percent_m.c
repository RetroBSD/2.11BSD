 /*
  * Replace %m by system error message.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#if	!defined(lint) && defined(DOSCCS)
static char sccsid[] = "@(#) percent_m.c 1.1.1 96/3/23 17:42:37";
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "mystdarg.h"

char   *percent_m(obuf, ibuf)
char   *obuf;
char   *ibuf;
{
    char   *bp = obuf;
    char   *cp = ibuf;

    while (*bp = *cp)
	if (*cp == '%' && cp[1] == 'm') {
	    strcpy(bp, strerror(errno));
	    bp += strlen(bp);
	    cp += 2;
	} else {
	    bp++, cp++;
	}
    return (obuf);
}

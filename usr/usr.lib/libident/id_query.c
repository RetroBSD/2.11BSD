/*
** id_query.c                             Transmit a query to an IDENT server
**
** Author: Peter Eriksson <pen@lysator.liu.se>
*/

#ifdef NeXT3
#  include <libc.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <signal.h>

#ifdef HAVE_ANSIHEADERS
#  include <stdlib.h>
#  include <string.h>
#  include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#ifdef _AIX
#  include <sys/select.h>
#endif

#ifdef _AIX
#  include <sys/select.h>
#endif
#ifdef VMS
#  include <sys/socket.h>     /* for fd_set */
#endif
#define IN_LIBIDENT_SRC
#include "ident.h"


int id_query __P4(ident_t *, id,
		  int, lport,
		  int, fport,
		  struct timeval *, timeout)
{
#ifdef SIGRETURNTYPE
    SIGRETURNTYPE (*old_sig)();
#else
    void (*old_sig) __P((int));
#endif
    int res;
    char buf[80];
    fd_set ws;
    
    sprintf(buf, "%d , %d\r\n", lport, fport);
    
    if (timeout)
    {
	FD_ZERO(&ws);
	FD_SET(id->fd, &ws);

#ifdef __hpux
	if ((res = select(FD_SETSIZE, (int *)0, (int *)&ws, (int *)0, timeout)) < 0)
#else
	if ((res = select(FD_SETSIZE, (fd_set *)0, &ws, (fd_set *)0, timeout)) < 0)
#endif
	    return -1;
	
	if (res == 0)
	{
	    errno = ETIMEDOUT;
	    return -1;
	}
    }

    old_sig = signal(SIGPIPE, SIG_IGN);
    
    res = write(id->fd, buf, strlen(buf));
    
    signal(SIGPIPE, old_sig);
    
    return res;
}

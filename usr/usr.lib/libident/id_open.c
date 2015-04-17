/*
** id_open.c                 Establish/initiate a connection to an IDENT server
**
** Author: Peter Eriksson <pen@lysator.liu.se>
** Fixes: Pdr Emanuelsson <pell@lysator.liu.se>
*/

#ifdef NeXT3
#  include <libc.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#ifdef HAVE_ANSIHEADERS
#  include <stdlib.h>
#  include <string.h>
#  include <unistd.h>
#  if !defined(__sgi) && !defined(VMS)
#    define bzero(p,l)     memset(p, 0, l)
#  endif
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/file.h>

#define IN_LIBIDENT_SRC
#include "ident.h"

#include <arpa/inet.h>

#ifdef _AIX
#  include <sys/select.h>
#endif

ident_t *id_open __P3(struct in_addr *, laddr,
		      struct in_addr *, faddr,
		      struct timeval *, timeout)
{
    ident_t *id;
    int res, tmperrno;
    struct sockaddr_in sin_laddr, sin_faddr;
    fd_set rs, ws, es;
#ifndef OLD_SETSOCKOPT
    int on = 1;
    struct linger linger;
#endif
    
    if ((id = (ident_t *) malloc(sizeof(*id))) == 0)
	return 0;
    
    if ((id->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	free(id);
	return 0;
    }
    
    if (timeout)
    {
	if ((res = fcntl(id->fd, F_GETFL, 0)) < 0)
	    goto ERROR;

#ifndef VMS
	if (fcntl(id->fd, F_SETFL, res | FNDELAY) < 0)
	    goto ERROR;
#endif
    }

    /* We silently ignore errors if we can't change LINGER */
#ifdef OLD_SETSOCKOPT
    /* Old style setsockopt() */
    (void) setsockopt(id->fd, SOL_SOCKET, SO_DONTLINGER);
    (void) setsockopt(id->fd, SOL_SOCKET, SO_REUSEADDR);
#else
    /* New style setsockopt() */
    linger.l_onoff = 0;
    linger.l_linger = 0;
    
    (void) setsockopt(id->fd, SOL_SOCKET, SO_LINGER, (void *) &linger, sizeof(linger));
    (void) setsockopt(id->fd, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on));
#endif
    
    id->buf[0] = '\0';
    
    bzero((char *)&sin_laddr, sizeof(sin_laddr));
    sin_laddr.sin_family = AF_INET;
    sin_laddr.sin_addr = *laddr;
    sin_laddr.sin_port = 0;
    
    if (bind(id->fd, (struct sockaddr *) &sin_laddr, sizeof(sin_laddr)) < 0)
    {
#ifdef DEBUG
	perror("libident: bind");
#endif
	goto ERROR;
    }
    
    bzero((char *)&sin_faddr, sizeof(sin_faddr));
    sin_faddr.sin_family = AF_INET;
    sin_faddr.sin_addr = *faddr;
    sin_faddr.sin_port = htons(IDPORT);

    errno = 0;
    res = connect(id->fd, (struct sockaddr *) &sin_faddr, sizeof(sin_faddr));
    if (res < 0 && errno != EINPROGRESS)
    {
#ifdef DEBUG
	perror("libident: connect");
#endif
	goto ERROR;
    }

    if (timeout)
    {
	FD_ZERO(&rs);
	FD_ZERO(&ws);
	FD_ZERO(&es);
	
	FD_SET(id->fd, &rs);
	FD_SET(id->fd, &ws);
	FD_SET(id->fd, &es);

#ifdef __hpux
	if ((res = select(FD_SETSIZE, (int *) &rs, (int *) &ws, (int *) &es, timeout)) < 0)
#else
	if ((res = select(FD_SETSIZE, &rs, &ws, &es, timeout)) < 0)
#endif
	{
#ifdef DEBUG
	    perror("libident: select");
#endif
	    goto ERROR;
	}
	
	if (res == 0)
	{
	    errno = ETIMEDOUT;
	    goto ERROR;
	}
	
	if (FD_ISSET(id->fd, &es))
	    goto ERROR;
	
	if (!FD_ISSET(id->fd, &rs) && !FD_ISSET(id->fd, &ws))
	    goto ERROR;
    }
    
    return id;
    
  ERROR:
    tmperrno = errno;		/* Save, so close() won't erase it */
    close(id->fd);
    free(id);
    errno = tmperrno;
    return 0;
}

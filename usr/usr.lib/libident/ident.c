/*
** ident.c	High-level calls to the ident lib
**
** Author: Pdr Emanuelsson <pell@lysator.liu.se>
** Hacked by: Peter Eriksson <pen@lysator.liu.se>
*/

#ifdef NeXT3
#  include <libc.h>
#endif

#include <stdio.h>

#ifdef HAVE_ANSIHEADERS
#  include <stdlib.h>
#  include <string.h>
#endif

#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

#define IN_LIBIDENT_SRC
#include "ident.h"

#include <arpa/inet.h>


/* Do a complete ident query and return result */

IDENT *ident_lookup __P2(int, fd,
			 int, timeout)
{
    struct sockaddr_in localaddr, remoteaddr;
    int len;
    
    len = sizeof(remoteaddr);
    if (getpeername(fd, (struct sockaddr*) &remoteaddr, &len) < 0)
	return 0;
    
    len = sizeof(localaddr);
    if (getsockname(fd, (struct sockaddr *) &localaddr, &len) < 0)
	return 0;

    return ident_query( &localaddr.sin_addr, &remoteaddr.sin_addr,
		       ntohs(localaddr.sin_port), ntohs(remoteaddr.sin_port),
		       timeout);
}


IDENT *ident_query __P5(struct in_addr *, laddr,
			struct in_addr *, raddr,
			int, lport,
			int, rport,
			int, timeout)
{
    int res;
    ident_t *id;
    struct timeval timout;
    IDENT *ident=0;

    
    timout.tv_sec = timeout;
    timout.tv_usec = 0;
    
    if (timeout)
	id = id_open( laddr, raddr, &timout);
    else
	id = id_open( laddr, raddr, (struct timeval *)0);
    
    if (!id)
    {
	errno = EINVAL;
	return 0;
    }
  
    if (timeout)
	res = id_query(id, rport, lport, &timout);
    else
	res = id_query(id, rport, lport, (struct timeval *) 0);
    
    if (res < 0)
    {
	id_close(id);
	return 0;
    }
    
    ident = (IDENT *) malloc(sizeof(IDENT));
    if (!ident) {
	id_close(id);
	return 0;
    }
    
    if (timeout)
	res = id_parse(id, &timout,
		       &ident->lport,
		       &ident->fport,
		       &ident->identifier,
		       &ident->opsys,
		       &ident->charset);
    else
	res = id_parse(id, (struct timeval *) 0,
		       &ident->lport,
		       &ident->fport,
		       &ident->identifier,
		       &ident->opsys,
		       &ident->charset);
    
    if (res != 1)
    {
	free(ident);
	id_close(id);
	return 0;
    }
    
    id_close(id);
    return ident;			/* At last! */
}


char *ident_id __P2(int, fd,
		    int, timeout)
{
    IDENT *ident;
    char *id=0;
    
    ident = ident_lookup(fd, timeout);
    if (ident && ident->identifier && *ident->identifier)
    {
	id = id_strdup(ident->identifier);
	if (id == NULL)
	    return NULL;
    }

    ident_free(ident);
    return id;
}


void ident_free __P1(IDENT *, id)
{
    if (!id)
	return;
    if (id->identifier)
	free(id->identifier);
    if (id->opsys)
	free(id->opsys);
    if (id->charset)
	free(id->charset);
    free(id);
}

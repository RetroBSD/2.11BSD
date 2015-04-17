/*
** id_parse.c                    Receive and parse a reply from an IDENT server
**
** Author: Peter Eriksson <pen@lysator.liu.se>
** Fiddling: Pär Emanuelsson <pell@lysator.liu.se>
*/

#ifdef NeXT3
#  include <libc.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

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


int id_parse __P7(ident_t *, id,
		  struct timeval *, timeout,
		  int *, lport,
		  int *, fport,
		  char **, identifier,
		  char **, opsys,
		  char **, charset)
{
    char c, *cp, *tmp_charset;
    fd_set rs;
    int pos, res=0, lp, fp;
    
    errno = 0;
    
    tmp_charset = 0;
    
    if (!id)
	return -1;
    if (lport)
	*lport = 0;
    if (fport)
	*fport = 0;
    if (identifier)
	*identifier = 0;
    if (opsys)
	*opsys = 0;
    if (charset)
	*charset = 0;
    
    pos = strlen(id->buf);
    
    if (timeout)
    {
	FD_ZERO(&rs);
	FD_SET(id->fd, &rs);

#ifdef __hpux
	if ((res = select(FD_SETSIZE, (int *) &rs, (int *)0, (int *)0, timeout)) < 0)
#else
	if ((res = select(FD_SETSIZE, &rs, (fd_set *)0, (fd_set *)0, timeout)) < 0)
#endif
	    return -1;
	
	if (res == 0)
	{
	    errno = ETIMEDOUT;
	    return -1;
	}
    }
    
    /* Every octal value is allowed except 0, \n and \r */
    while (pos < sizeof(id->buf) &&
	   (res = read(id->fd, id->buf + pos, 1)) == 1 &&
	   id->buf[pos] != '\n' && id->buf[pos] != '\r')
	pos++;
    
    if (res < 0)
	return -1;
    
    if (res == 0)
    {
	errno = ENOTCONN;
	return -1;
    }
    
    if (id->buf[pos] != '\n' && id->buf[pos] != '\r')
	return 0;		/* Not properly terminated string */
    
    id->buf[pos++] = '\0';
    
    /*
    ** Get first field (<lport> , <fport>)
    */
    cp = id_strtok(id->buf, ":", &c);
    if (!cp)
	return -2;
    
    if (sscanf(cp, " %d , %d", &lp, &fp) != 2)
    {
	if (identifier)
	{
	    *identifier = id_strdup(cp);
	    if (*identifier == NULL)
	        return -4;
	}
	return -2;
    }
    
    if (lport)
	*lport = lp;
    if (fport)
	*fport = fp;
    
    /*
    ** Get second field (USERID or ERROR)
    */
    cp = id_strtok((char *)0, ":", &c);
    if (!cp)
	return -2;
    
    if (strcmp(cp, "ERROR") == 0)
    {
	cp = id_strtok((char *)0, "\n\r", &c);
	if (!cp)
	    return -2;
	
	if (identifier)
	{
	    *identifier = id_strdup(cp);
	    if (*identifier == NULL)
	        return -4;
	}
	
	return 2;
    }
    else if (strcmp(cp, "USERID") == 0)
    {
	/*
	** Get first subfield of third field <opsys>
	*/
	cp = id_strtok((char *) 0, ",:", &c);
	if (!cp)
	    return -2;
	
	if (opsys)
	{
	    *opsys = id_strdup(cp);
	    if (*opsys == NULL)
	        return -4;
	}
	
	/*
	** We have a second subfield (<charset>)
	*/
	if (c == ',')
	{
	    cp = id_strtok((char *)0, ":", &c);
	    if (!cp)
		return -2;
	    
	    tmp_charset = cp;
	    if (charset)
	    {
		*charset = id_strdup(cp);
		if (*charset == NULL)
		    return -4;
	    }
	    
	    /*
	    ** We have even more subfields - ignore them
	    */
	    if (c == ',')
		id_strtok((char *)0, ":", &c);
	}
	
	if (tmp_charset && strcmp(tmp_charset, "OCTET") == 0)
	    cp = id_strtok((char *)0, (char *)0, &c);
	else
	    cp = id_strtok((char *)0, "\n\r", &c);
	
	if (identifier)
	{
	    *identifier = id_strdup(cp);
	    if (*identifier == NULL)
	        return -4;
	}
	return 1;
    }
    else
    {
	if (identifier)
	{
	    *identifier = id_strdup(cp);
	    if (*identifier == NULL)
	        return -4;
	}
	return -3;
    }
}

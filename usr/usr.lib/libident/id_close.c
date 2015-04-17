/*
** id_close.c                            Close a connection to an IDENT server
**
** Author: Peter Eriksson <pen@lysator.liu.se>
*/

#ifdef NeXT3
#  include <libc.h>
#endif

#ifdef HAVE_ANSIHEADERS
#  include <stdlib.h>
#  include <unistd.h>
#endif

#define IN_LIBIDENT_SRC
#include "ident.h"

int id_close __P1(ident_t *, id)
{
    int res;
  
    res = close(id->fd);
    free(id);
    
    return res;
}



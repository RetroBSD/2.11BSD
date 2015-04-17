/*
** support.c
**
** Author: Pr Emanuelsson <pell@lysator.liu.se>
** Hacked by: Peter Eriksson <pen@lysator.liu.se>
*/
#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_ANSIHEADERS
#  include <stdlib.h>
#  include <string.h>
#else
#  define strchr(str, c) index(str, c)
#endif

#define IN_LIBIDENT_SRC
#include "ident.h"


char *id_strdup __P1(char *, str)
{
    char *cp;

    cp = (char *) malloc(strlen(str)+1);
    if (cp == NULL)
    {
#ifdef DEBUG
	perror("libident: malloc");
#endif
        return NULL;
    }

    strcpy(cp, str);

    return cp;
}


char *id_strtok __P3(char *, cp,
		      char *, cs,
		      char *, dc)
{
    static char *bp = 0;
    
    if (cp)
	bp = cp;
    
    /*
    ** No delimitor cs - return whole buffer and point at end
    */
    if (!cs)
    {
	while (*bp)
	    bp++;
	return cs;
    }
    
    /*
    ** Skip leading spaces
    */
    while (isspace(*bp))
	bp++;
    
    /*
    ** No token found?
    */
    if (!*bp)
	return 0;
    
    cp = bp;
    while (*bp && !strchr(cs, *bp))
	bp++;
    
    /*
    ** Remove trailing spaces
    */
    *dc = *bp;
    for (dc = bp-1; dc > cp && isspace(*dc); dc--)
	;
    *++dc = '\0';
    
    bp++;
    
    return cp;
}

#include "parms.h"
#include "structs.h"
#include <pwd.h>

#ifdef	RCSIDENT
static char rcsid[] = "$Header: gname.c,v 1.7.0.1 85/03/17 20:55:37 notes Rel $";
#endif	RCSIDENT


/* 
 * 	get the user id ( and his name from th password file )
 *	the easy way - with system calls.
 */
getname (who_me, anon)					/* anon=true for anonymous */
struct auth_f  *who_me;
{
    register    count;
    register char  *s,
                   *d;
    static int  gotname = 0;				/* whether we have done a getpw */
    static struct passwd  *gotstat = 0;

    if (gotname == 0 && anon == 0)			/* grab name if we will require it */
    {
	gotstat = getpwuid(globuid);			/* grab it */
	gotname = 1;					/* set flag saying we have it */
    }
    if (!gotstat || anon)
    {
	s = "Anonymous:";
	who_me -> aid = Anonuid;
    }
    else
    {
	s = gotstat->pw_name;
	who_me -> aid = globuid;
    }
    d = who_me -> aname;				/* copy his name */
    count = NAMESZ;
    while (((*d++ = *s++) != '\0') && --count);
    *--d = '\0';
    s = Authsystem;					/* copy his system */
    d = who_me -> asystem;
    count = HOMESYSSZ;
    while (((*d++ = *s++)) != '\0' && --count);		/* move system */
    *--d = '\0';
}

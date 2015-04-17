#include "parms.h"
#include "structs.h"

#ifdef	RCSIDENT
static char rcsid[] = "$Header: help.c,v 1.7.0.1 86/01/25 22:55:47 notes Rel $";
#endif	RCSIDENT

/*
 *	help(file) char *file;
 *	prints the specified help file using more and then
 *	does a pause with 'hit any key to continue.
 *	
 *	Original idea:	Rob Kolstad January 1982
 */

help (file)
char   *file;
{
    char    cmdline[CMDLEN];				/* line buffer */
    char   *command;

    if ((command = getenv ("PAGER")) == NULL)		/* see if overridden */
	command = PAGER;				/* assign default */
#ifndef	FASTFORK
    sprintf (cmdline, "%s < %s/%s/%s", command, Mstdir, UTILITY, file);
    dounix (cmdline, 1, 1);
#else
    {
	if (index (command, ' ') != (char *) NULL)	/* need shell */
	{
	    sprintf (cmdline, "%s %s/%s/%s",
		    command, Mstdir, UTILITY, file);
	    dounix (1, 1, DFLTSH, "-c", cmdline, 0, 0);
	}
	else
	{
	    sprintf (cmdline, "%s/%s/%s", Mstdir, UTILITY, file);
	    dounix (1, 1, command, cmdline, 0, 0, 0);
	}
    }
#endif
    printf ("  --Hit any key to continue--");
    gchar ();
}

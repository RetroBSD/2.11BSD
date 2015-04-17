#include "parms.h"
#include "structs.h"
#include "net.h"

#ifdef	RCSIDENT
static char rcsid[] = "$Header: getnet.c,v 1.7.0.2 86/01/17 23:49:54 notes Rel $";
#endif	RCSIDENT

/*
 *	getnet(sys,nf, howp)
 *	char *sys;
 *	char *nf;
 *	struct nethow_f *howp;
 *
 *	This routine will scan the net.how file, looking for alternate
 *	routes to the system specified. The xmit and reply pointers are
 *	set appropriately.
 *	(null if no entry found).
 *	The routine returns -1 if the file is not there, otherwise
 *	it returns the count of lines matched (0,1,2)
 *
 *	Original Coding:	Ray Essick	April 1982
 */



int     GetNetDebug = 0;

getnet (sys, nf, howp)
char   *sys;
char   *nf;
struct nethow_f *howp;					/* fill it in */
{

    FILE * nethow;
    char    pathname[256];				/* probably ok */
    char    oneline[512];
    char   *p;
    int     i,
            count;
#define	MAXFIELDS	5
    char   *field[MAXFIELDS];				/* field pointers */

    count = 0;						/* lines we have */
    if (howp != (struct nethow_f *) NULL)		/* init the sucker */
    {
	howp -> nh_proto = 0;
	howp -> nh_system = howp -> nh_nf = (char *) NULL;
	howp -> nh_xmit = howp -> nh_rcv = (char *) NULL;
    }

    sprintf (pathname, "%s/%s/%s", Mstdir, UTILITY, NETHOW);
    if ((nethow = fopen (pathname, "r")) == NULL)
	return (-1);

    while (fgets (oneline, sizeof oneline, nethow) != NULL && count < 2)
    {
	if (GetNetDebug)
	    fprintf (stderr, "getnet reads: %s", oneline);
	i = strlen (oneline);				/* get end */
	if (oneline[i - 1] != '\n')			/* not entire line */
	{
	    fprintf (stderr, "%s: net.how line longer than %d characters",
		    Invokedas, pathname, sizeof oneline);
	    return (-2);				/* line too long */
	}
	else
	{
	    oneline[i - 1] = '\0';			/* strip newline */
	}
	if (oneline[0] == '#' || oneline[0] == '\0')
	    continue;					/* comment or empty */

	for (i = 1; i < MAXFIELDS; i++)
	    field[i] = NULL;				/* zero them */
	for (i = 1, p = field[0] = oneline; i < MAXFIELDS && *p != '\0';)
	{						/* extract the field */
	    if (*p == '\0')				/* end of string */
		break;
	    if (*p != ':')				/* not end of a field */
	    {
		p++;
		continue;
	    }
	    *p = '\0';					/* end this field */
	    field[i++] = ++p;				/* start next field */
	}
	if (GetNetDebug > 3)
	{
	    for (i = 0; i < MAXFIELDS; i++)
		fprintf (stderr, "field[%d] = %s\n", i, field[i]);
	}

/*
 *	see if it is what we want. if so, fill in the appropriate
 *	structure elements and go away.
 */

	if (strcmp (field[0], sys) != 0)		/* match? */
	    continue;					/* no */

	/* 
	 * see if the nf matches.
	 * BEEF THIS UP. MAKE A SUBROUTINE TO MATCH
	 */

	if (*field[3] != '\0')
	{
	    char   *lead = field[3],
	           *trail = field[3];
	    int     match = 0;

	    extern int  nfmatch ();

	    while (1)
	    {
		if (*lead == '\0' || *lead == ',')	/* new pattern */
		{
		    if (*lead == ',')
		    {
			*lead++ = '\0';
		    }					/* terminate */
		    if (nfmatch (nf, trail) == 0)	/* same? */
		    {
			match++;
			break;				/* yes, get out */
		    }
		    trail = lead;			/* bring up the pointer */
		    if (*lead != '\0')			/* get more */
			continue;
		    else
			break;				/* get out */
		}
		lead++;					/* skip onward */
	    }
	    if (match == 0)				/* nothing matched */
		continue;				/* so get another line */
	}

	if (howp -> nh_nf == (char *) NULL)
	{
	    howp -> nh_nf = strsave (nf);		/* nf name saved */
	}


	/* 
	 * fill in the appropriate field
	 */
	if (*field[1] == 'x')				/* transmit */
	{
	    if (howp -> nh_xmit == (char *) NULL)	/* not yet */
	    {
		howp -> nh_xmit = strsave (field[4]);	/* save it */
		if (sscanf (field[2], "%d", &i) != 1)
		{
		    if (*field[2] != '\0')		/* message if non-empty */
			fprintf (stderr, "protocol field '%s' should be an integer\n",
				field[2]);
		    i = 0;
		}
		howp -> nh_proto = i;			/* save protocol */
		count++;				/* filled another field */
		if (GetNetDebug > 1)
		    fprintf (stderr, "getnet: proto %d, how: %s\n",
			    howp -> nh_proto, howp -> nh_xmit);
	    }
	}
	else
	{						/* force reply */
	    if (howp -> nh_rcv == (char *) NULL)
	    {
		howp -> nh_rcv = strsave (field[4]);	/* save it */
		count++;				/* filled a field */
		if (GetNetDebug > 1)
		    fprintf (stderr, "getnet: rcv: %s\n", howp -> nh_rcv);
	    }
	}
    }
    /* 
     * have both lknes (or exhausted input). close the file
     * and go back.
     */
    fclose (nethow);
    return (count);
}

/*
 *	quicky pattern matching routine.
 *
 *	someday, make it do real pattern matching
 */

static int  nfmatch (nf, pattern)
char   *nf;
char   *pattern;
{
    char   *p;
    int     result;

#ifdef	HAS_REGEXP
    extern char *re_comp ();
    extern int  re_exec ();
#endif	HAS_REGEXP

    if (GetNetDebug)
	fprintf (stderr, "nf = '%s', pattern = '%s'\n", nf, pattern);

#ifdef	HAS_REGEXP
    /* 
     * regular expression routines from BSD -- don't know
     * if anyone else picked them up.
     */
    p = re_comp (pattern);				/* compile pattern */
    if (p != NULL)					/* error in pattern */
    {
	fprintf (stderr, "%s: %s: %s\n", Invokedas, pattern, p);
	return (-1);					/* no match */
    }
    result = re_exec (nf);
    switch (result)					/* fix return code */
    {
	case 1: 
	    if (GetNetDebug > 1)
		fprintf (stderr, "Matched\n");
	    return 0;
	case 0: 
	    if (GetNetDebug > 1)
		fprintf (stderr, "NO Match\n");
	    return 1;
	default: 
	    if (GetNetDebug > 1)
		fprintf (stderr, "Regular Expresion Error\n");
	    return result;
    }
#endif	HAS_REGEXP
    /* 
     * real cheap -- check exact matches
     */
    result = strcmp (nf, pattern);
    switch (result)
    {
	case 0: 
	    if (GetNetDebug)
		fprintf (stderr, "strcmp match\n");
	    return (0);
	default: 
	    if (GetNetDebug)
		fprintf (stderr, "strcmp NO match\n");
	    return (result);
    }
}

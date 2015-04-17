#define		MAINLINE
#include	"parms.h"
#include	"structs.h"

#ifdef	RCSIDENT
static char rcsid[] = "$Header: nftimestamp.c,v 1.7.0.1 86/02/12 11:27:13 notes Rel $";
#endif	RCSIDENT

/*
 *	quickie that sets the specified timestamps to the specified
 *	time.  no time means now.
 *
 *	Ray Essick		February 1986
 */

static int  Verbose = 0;


main (argc, argv)
int     argc;
char  **argv;
{
    struct io_f io;
    int     argn;
    struct nflist_f *nfptr;
    struct when_f   ztime;
    char    buf[128];

    if (argc == 1)
	usage ();					/* tell him how */
    startup (argc, argv);				/* common init code */
    gettime (&ztime);					/* if no -t, use this */


    for (argn = 1; argn < argc; argn++)
    {
	if (strcmp (argv[argn], "-v") == 0)		/* be noisy */
	{
	    Verbose++;
	    continue;
	}
	if (strcmp (argv[argn], "-t") == 0)		/* set time */
	{
	    if (++argn >= argc)
		usage ();
	    switch (parsetime (argv[argn], &ztime))
	    {
		case -1: 				/* invalid date */
		    fprintf (stderr, "Unable to parse '%s'\n", argv[argn]);
		    exit (1);
		case 2: 				/* future */
		    sprdate (&ztime, buf);
		    fprintf (stderr, "Time '%s' parses to the future as '%s'\n",
			    argv[argn], buf);
		    exit (1);
		case 0: 				/* just right */
		    sprdate (&ztime, buf);
		    fprintf (stderr, "Time parsed as %s\n", buf);
		    break;
	    }
	    continue;
	}
	if (strcmp (argv[argn], "-f") == 0)
	{
	    argn++;
	    if (argn >= argc)
	    {
		usage ();
	    }
	    readrc (argv[argn]);
	    continue;
	}
	if (strcmp (argv[argn], "-u") == 0)		/* specify user */
	{
	    if (globuid != Notesuid)
	    {
		fprintf (stderr, "Only %s can specify -u user\n", NOTES);
		exit (1);
	    }
	    argn++;
	    if (argn >= argc)
	    {
		usage ();
	    }
	    strcpy (Seqname, argv[argn]);
	    continue;
	}
	/* 
	 * must be a notesfile
	 */
	expand (argv[argn]);				/* load it */
    }

    while ((nfptr = nextgroup ()) != (struct nflist_f *) NULL)
    {
	/* 
	 * open it to make sure it exists.
	 */
	if (init (&io, nfptr -> nf_name) < 0)		/* open */
	{
	    printf ("%s:	couldn't open\n", nfptr -> nf_name);
	    continue;
	}
	if (Verbose)
	{
	    fprintf (stdout, "%s\n", nfptr -> nf_name);
	    fflush (stdout);
	}
	/* 
	 * go get the timestamp, update, and replace
	 * only update it if he has read permission in the nf.
	 * exception -- when notes uses -u, give him whatever he
	 * wants. (he will have read permission in such a case).
	 */
	if (allow (&io, READOK))
	{
	    fixlast (&ztime, io.nf, NORMSEQ, Seqname);
	}

	closenf (&io);
    }
    exit (0);						/* all done */
}

usage (name)						/* how to invoke */
char   *name;
{
    fprintf (stderr,
	    "Usage: %s [-v] [-u sequencer] [-t timestamp] <nfspec> [<notesfile>...]\n",
	    name);
    fprintf (stderr, "<nfspec> ::= <notesfile> | -f <filename>\n");
    exit (1);
}

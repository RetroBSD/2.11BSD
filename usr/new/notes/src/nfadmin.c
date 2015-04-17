#define		MAINLINE
#include	"parms.h"
#include	"structs.h"

#ifdef	RCSIDENT
static char rcsid[] = "$Header: nfadmin.c,v 1.8 87/07/01 13:47:49 paul Exp $";
#endif	RCSIDENT

/*
 *	Any normal user can run this program. It only allows changes
 *	to notesfiles the user is a director in.
 *
 */


main (argc, argv)
int     argc;
char  **argv;
{
    struct io_f io;
    int     argn;
    struct nflist_f *nfptr;
    int	    change, newexp;
    int	    newanon, newopen, newnet, newarch, newlocal, newmod;
    int	    newwset, newdir, newact, newlen;

    startup (argc, argv);				/* common init code */

    newdir = newact = newwset = newlen = newexp = -2;
    newanon = newopen = newnet = newarch = newlocal = newmod = change = 0;
    for ( argc--, argv++; argc > 0 && argv[0][0] == '-'; argc--, argv++)
    {
	switch(argv[0][1])
	{
	case 'f':	
		readrc(argv[1]);
		argv++, argc--;
		break;
	case 'a':
		change = 1;
		newanon = newval(argv[0][2]);
		break;
	case 'n':
		change = 1;
		newnet = newval(argv[0][2]);
		break;
	case 'o':
		change = 1;
		newopen = newval(argv[0][2]);
		break;
	case 'L':
		change = 1;
		newlocal = newval(argv[0][2]);
		break;
	case 'M':
		change = 1;
		newmod = newval(argv[0][2]);
		break;
	case 'A':
		change = 1;
		newarch = newval(argv[0][2]);
		break;
	case 'e':
		change = 1;
		if (argv[0][2] == '=')
		{
			newexp = atoi(&argv[0][3]);
		} else usage();
		break;
	case 'W':
		change = 1;
		if (argv[0][2] == '=')
		{
			newwset = atoi(&argv[0][3]);
		} else usage();
		break;
	case 'l':
		change = 1;
		if (argv[0][2] == '=')
		{
			newlen = atoi(&argv[0][3]);
		} else usage();
		break;
	case 'E':
		change = 1;
		if (argv[0][2] == '=')
		{
			if (strncmp(&argv[0][3], "arch", 4) == 0)
				newact = KEEPYES;
			else if (strncmp(&argv[0][3], "del", 3) == 0)
				newact = KEEPNO;
			else if (strncmp(&argv[0][3], "def", 3) == 0)
				newact = KEEPDFLT;
			else if (strncmp(&argv[0][3], "dfl", 3) == 0)
				newact = KEEPDFLT;
			else usage();
		} else usage();
		break;
	case 'D':
		change = 1;
		if (argv[0][2] == '=')
		{
			if (strncmp(&argv[0][3], "on", 2) == 0)
				newdir = DIRON;
			else if (strncmp(&argv[0][3], "off", 3) == 0)
				newdir = DIROFF;
			else if (strncmp(&argv[0][3], "no", 2) == 0)
				newdir = DIRNOCARE;
			else if (strncmp(&argv[0][3], "anyon", 5) == 0)
				newdir = DIRANYON;
			else if (strncmp(&argv[0][3], "def", 3) == 0)
				newdir = DIRDFLT;
			else if (strncmp(&argv[0][3], "dfl", 3) == 0)
				newdir = DIRDFLT;
			else usage();
		} else usage();
		break;
	default:
		printf("Bad flag %c\n", argv[0][1]);
		usage();
		break;
	}
    }

    for (argn = 0; argn < argc; argn++)
    {
	expand (argv[argn]);				/* load it */
    }

    while ((nfptr = nextgroup ()) != (struct nflist_f *) NULL)
    {
	if (init (&io, nfptr -> nf_name) < 0)		/* open */
	{
	    printf ("%s:	couldn't open\n", nfptr -> nf_name);
	    continue;
	}
	if (change)
	{
	    if (!allow (&io, DRCTOK))			/* a director? */
	    {
		printf ("%s:	Not a director\n", nfptr -> nf_name);
		closenf (&io);
		continue;
	    }
	    locknf(&io, DSCRLOCK);
	    getdscr(&io, &io.descr);
	    if (newnet)
		if (newnet > 0)
			io.descr.d_stat |= NETWRKD;
		else
			io.descr.d_stat &= ~NETWRKD;
	    if (newanon)
		if (newanon > 0)
			io.descr.d_stat |= ANONOK;
		else
			io.descr.d_stat &= ~ANONOK;
	    if (newopen)
		if (newopen > 0)
			io.descr.d_stat |= OPEN;
		else
			io.descr.d_stat &= ~OPEN;
	    if (newlocal)
		if (newlocal > 0)
			io.descr.d_stat |= LOCAL;
		else
			io.descr.d_stat &= ~LOCAL;
	    if (newmod)
		if (newmod > 0)
			io.descr.d_stat |= MODERATED;
		else
			io.descr.d_stat &= ~MODERATED;
	    if (newarch)
		if (newarch > 0)
			io.descr.d_stat |= ISARCH;
		else
			io.descr.d_stat &= ~ISARCH;
	    if (newexp != -2)
		io.descr.d_archtime = newexp;
	    if (newwset != -2)
		io.descr.d_workset = newwset;
	    if (newlen != -2)
		io.descr.d_longnote = newlen;
	    if (newact != -2)
		io.descr.d_archkeep = newact;
	    if (newdir != -2)
		io.descr.d_dmesgstat = newdir;

	    putdscr(&io, &io.descr);
	    unlocknf(&io, DSCRLOCK);

	}
	notesum(&io.descr);
	closenf (&io);
    }
    exit (0);						/* all done */
}

usage (name)						/* how to invoke */
char   *name;
{
    fprintf (stderr,
	    "Usage: %s <flags> <notesfile> [<notesfile>...]\n",
	    "nfadmin");
    fprintf(stderr,
	    "-a+\tanonymous on\n-a-\tanonymous off\n-n+\tnetworked\n-n-\tnot networked\n-o+\topen\n-o-\tclosed\n-A+\tis archive\n-A-\tis not archive\n-e=n\texpiration time\n-e=-1\tnever expire\n");
    exit (1);
}

char *yes(b)
int	b;
{
	if (b)
		return "YES ";
	else	return " NO ";
}

int	flag = 0;

notesum(d)
struct descr_f *d;
{
/*
notesfile name  NetW Open Anon Arch   Wset archt kp  msgst    #note
xxxxxxxxxxxxxx: Y/N  Y/N  Y/N  Y/N   xxxxx xxxxx ARCH xxxx    xxxxx
*/

	char archtime[10], archkeep[10], msgstat[10];

	if (!flag)
	{
		printf("    notesfile title     NetW Open Anon Arch Locl Mod    WSet Arch. Keep DirM     #         Max\n");
		printf("                        ------------status-----------   Size Time  Actn Stat   Notes       Size\n");
	}
	flag = 1;
	switch((int) d->d_dmesgstat)
	{
	case DIRDFLT:	strcpy(msgstat, "DFLT");
			break;
	case DIRON:	strcpy(msgstat, " ON ");
			break;
	case DIROFF:	strcpy(msgstat, "OFF ");
			break;
	case DIRANYON:	strcpy(msgstat, "ANY ");
			break;
	case DIRNOCARE:	strcpy(msgstat, "IGNR");
			break;
	default:	strcpy(msgstat, "????");
			break;
	}
	switch((int) d->d_archkeep)
	{
	case KEEPDFLT:	strcpy(archkeep, "DFLT");
			break;
	case KEEPNO:	strcpy(archkeep, "DEL ");
			break;
	case KEEPYES:	strcpy(archkeep, "ARCH");
			break;
	default:	strcpy(archkeep, "????");
			break;
	}
	if (d->d_archtime == -1)
		strcpy(archtime, "NEVER");
	else if (d->d_archtime == 0)
		strcpy(archtime, "DFLT ");
	else	sprintf(archtime, "%4d ", d->d_archtime);


	printf("%22.22s: %s %s %s %s %s %s  %5d %s %s %s   %5d%12d\n",
		d->d_title, yes(d->d_stat & NETWRKD), yes(d->d_stat & OPEN),
		yes(d->d_stat & ANONOK), yes(d->d_stat & ISARCH), 
		yes(d->d_stat & LOCAL), yes(d->d_stat & MODERATED), 
		d->d_workset, archtime, archkeep, msgstat, d->d_nnote, 
		d->d_longnote);

}
int newval(c)
char	c;
{
	switch(c)
	{
	case '\0':
	case '+':
		return 1;
	case '-':
		return -1;
	}
	usage();
}

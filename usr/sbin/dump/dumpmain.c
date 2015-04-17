#if	!defined(lint) && defined(DOSCCS)
static	char *sccsid = "@(#)dumpmain.c	1.4 (2.11BSD GTE) 1996/2/7";
#endif

#include "dump.h"

long	blocksperfile;
int	cartridge = 0;
static	long	numarg();
static	void	missingarg();

char	*host = NULL;

main(argc, argv)
	int	argc;
	char	*argv[];
{
	register char	*cp;
	int	i, Tflag = 0, honorlevel = 1, mapsize;
	register struct	fstab	*dt;

	density = 160;	/* density in 0.1" units */
	time(&(spcl.c_date));

	tsize = 0;	/* Default later, based on 'c' option for cart tapes */
	tape = TAPE;
	increm = NINCREM;
	incno = '0';

	if (argc == 1) {
		(void) fprintf(stderr, "Must specify a key.\n");
		Exit(X_ABORT);
	}
	argv++;
	argc -= 2;

	for (cp = *argv++; *cp; cp++) {
		switch (*cp) {
		case '-':
			break;

		case 'w':
			lastdump('w');	/* tell us only what has to be done */
			exit(0);
			break;

		case 'W':		/* what to do */
			lastdump('W');	/* tell us state of what is done */
			exit(0);	/* do nothing else */

		case 'f':		/* output file */
			if (argc < 1)
				missingarg('f', "output file");
			tape = *argv++;
			argc--;
			break;

		case 'd':		/* density, in bits per inch */
			density = numarg('d', "density",
				10L, 327670L, &argc, &argv) / 10;
			break;

		case 's':			/* tape size, feet */
			tsize = numarg('s', "size",
				1L, 0L, &argc, &argv) * 12 * 10;
			break;

		case 'T':		/* time of last dump */
			if (argc < 1)
				missingarg('T', "time of last dump");
			spcl.c_ddate = unctime(*argv);
			if (spcl.c_ddate < 0) {
				(void)fprintf(stderr, "bad time \"%s\"\n",
					*argv);
				exit(X_ABORT);
			}
			Tflag = 1;
			lastlevel = '?';
			argc--;
			argv++;
			break;
#ifdef	notnow
		case 'b':		/* blocks per tape write */
			ntrec = numarg('b', "number of blocks per write",
				    1L, 1000L, &argc, &argv);
			break;
#endif

		case 'B':		/* blocks per output file */
			blocksperfile = numarg('B', "number of blocks per file",
				    1L, 0L, &argc, &argv);
			break;

		case 'c':		/* Tape is cart. not 9-track */
			cartridge = 1;
			break;

		/* dump level */
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			incno = *cp;
			break;

		case 'u':			/* update /etc/dumpdates */
			uflag++;
			break;

		case 'n':			/* notify operators */
			notify++;
			break;

		case 'h':
			honorlevel = numarg('h', "honor level",
				0L, 10L, &argc, &argv);
			break;

		default:
			fprintf(stderr, "bad key '%c%'\n", *cp);
			exit(X_ABORT);
		}
	}
	if (argc < 1) {
		(void)fprintf(stderr, "Must specify disk or filesystem\n");
		exit(X_ABORT);
	}
	disk = *argv++;
	argc--;
	if (argc >= 1) {
		(void)fprintf(stderr, "Unknown arguments to dump:");
		while (argc--)
			(void)fprintf(stderr, " %s", *argv++);
		(void)fprintf(stderr, "\n");
		exit(X_ABORT);
	}
	if (Tflag && uflag) {
	        (void)fprintf(stderr,
		    "You cannot use the T and u flags together.\n");
		exit(X_ABORT);
	}
	if (strcmp(tape, "-") == 0) {
		pipeout++;
		tape = "standard output";
	}

	if (blocksperfile)
		blocksperfile = blocksperfile / NTREC * NTREC; /* round down */
	else {
		/*
		 * Determine how to default tape size and density
		 *
		 *         	density				tape size
		 * 9-track	1600 bpi (160 bytes/.1")	2300 ft.
		 * 9-track	6250 bpi (625 bytes/.1")	2300 ft.
		 * cartridge	8000 bpi (100 bytes/.1")	1700 ft.
		 *						(450*4 - slop)
		 */
		if (density == 0)
			density = cartridge ? 100 : 160;
		if (tsize == 0)
			tsize = cartridge ? 1700L*120L : 2300L*120L;
	}

	if (index(tape, ':')) {
		host = tape;
		tape = index(host, ':');
		*tape++ = '\0';
#ifdef RDUMP
		if (rmthost(host) == 0)
			exit(X_ABORT);
#else
		(void)fprintf(stderr, "remote dump not enabled\n");
		exit(X_ABORT);
#endif
	}
	(void)setuid(getuid()); /* rmthost() is the only reason to be setuid */

	if (signal(SIGHUP, sighup) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGTRAP, sigtrap) == SIG_IGN)
		signal(SIGTRAP, SIG_IGN);
	if (signal(SIGFPE, sigfpe) == SIG_IGN)
		signal(SIGFPE, SIG_IGN);
	if (signal(SIGBUS, sigbus) == SIG_IGN)
		signal(SIGBUS, SIG_IGN);
	if (signal(SIGSEGV, sigsegv) == SIG_IGN)
		signal(SIGSEGV, SIG_IGN);
	if (signal(SIGTERM, sigterm) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
	if (signal(SIGINT, interrupt) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	set_operators();	/* /etc/group snarfed */
	/*
	 *	disk can be either the character or block special file name,
	 *	or the full file system path name.
	 */
	if (dt = getfsfile(disk))
		disk = rawname(dt->fs_spec);
	else
		if (dt = getfsspec(disk))
			disk = rawname(disk);
		else
			dt = getfsspec(deraw(disk));

	if (!Tflag)
		getitime();	/* /etc/dumpdates snarfed */

	msg("Date of this level %c dump: %s\n", incno,
		prdate(spcl.c_date));
	msg("Date of last level %c dump: %s\n", lastlevel,
		prdate(spcl.c_ddate));
	msg("Dumping %s ", disk);
	if (dt != 0)
		msgtail("(%s) ", dt->fs_file);
	if (host)
		msgtail("to %s on host %s\n", tape, host);
	else
		msgtail("to %s\n", tape);

	fi = open(disk, 0);
	if (fi < 0) {
		msg("Cannot open %s\n", disk);
		Exit(X_ABORT);
	}
	bzero(clrmap, sizeof (clrmap));
	bzero(dirmap, sizeof (dirmap));
	bzero(nodmap, sizeof (nodmap));
	mapsize = roundup(howmany(sizeof(dirmap),NBBY), DEV_BSIZE);

	esize = 0;

	nonodump = (incno - '0') < honorlevel;

	msg("mapping (Pass I) [regular files]\n");
	pass(mark, (short *)NULL);		/* mark updates esize */

	do {
		msg("mapping (Pass II) [directories]\n");
		nadded = 0;
		pass(add, dirmap);
	} while(nadded);

	bmapest(clrmap);
	bmapest(nodmap);

	if (pipeout) {
		esize += 10;	/* 10 trailer blocks */
		msg("estimated %ld tape blocks.\n", esize);
	} else {
		double fetapes;

		if (blocksperfile)
			fetapes = (double) esize / blocksperfile;
		else if (cartridge) {
			/* Estimate number of tapes, assuming streaming stops at
			   the end of each block written, and not in mid-block.
			   Assume no erroneous blocks; this can be compensated
			   for with an artificially low tape size. */
			fetapes = 
			(	  esize		/* blocks */
				* DEV_BSIZE	/* bytes/block */
				* (1.0/density)	/* 0.1" / byte */
			  +
				  esize		/* blocks */
				* (1.0/NTREC)	/* streaming-stops per block */
				* 15.48		/* 0.1" / streaming-stop */
			) * (1.0 / tsize );	/* tape / 0.1" */
		} else {
			/* Estimate number of tapes, for old fashioned 9-track
			   tape */
			int tenthsperirg = (density == 625) ? 3 : 7;
			fetapes =
			(	  esize		/* blocks */
				* DEV_BSIZE	/* bytes / block */
				* (1.0/density)	/* 0.1" / byte */
			  +
				  esize		/* blocks */
				* (1.0/NTREC)	/* IRG's / block */
				* tenthsperirg	/* 0.1" / IRG */
			) * (1.0 / tsize );	/* tape / 0.1" */
		}
		etapes = fetapes;		/* truncating assignment */
		etapes++;
		/* count the dumped inodes map on each additional tape */
		esize += (etapes - 1) *
			(howmany(mapsize * sizeof(char), DEV_BSIZE) + 1);
		esize += etapes + 10;	/* headers + 10 trailer blks */
		msg("estimated %ld tape blocks on %3.2f tape(s).\n",
		    esize, fetapes);
	}

	alloctape();			/* Allocate tape buffer */

	otape();			/* bitmap is the first to tape write */
	time(&(tstart_writing));
	bitmap(clrmap, TS_CLRI);

	msg("dumping (Pass III) [directories]\n");
	pass(dump, dirmap);

	msg("dumping (Pass IV) [regular files]\n");
	pass(dump, nodmap);

	spcl.c_type = TS_END;
	for(i=0; i<NTREC; i++)
		spclrec();
	msg("DUMP: %ld tape blocks on %d tape(s)\n",spcl.c_tapea,spcl.c_volume);
	msg("DUMP IS DONE\n");

	putitime();
	tape_rewind();
	broadcast("DUMP IS DONE!\7\7\n");
	Exit(X_FINOK);
}

/*
 * Pick up a numeric argument.  It must be nonnegative and in the given
 * range (except that a vmax of 0 means unlimited).
 */
static long
numarg(letter, meaning, vmin, vmax, pargc, pargv)
	int letter;
	char *meaning;
	long vmin, vmax;
	int *pargc;
	char ***pargv;
{
	register char *p;
	long val;
	char *str;

	if (--*pargc < 0)
		missingarg(letter, meaning);
	str = *(*pargv)++;
	for (p = str; *p; p++)
		if (!isdigit(*p))
			goto bad;
	val = atol(str);
	if (val < vmin || (vmax && val > vmax))
		goto bad;
	return (val);

bad:
	(void)fprintf(stderr, "bad '%c' (%s) value \"%s\"\n",
	    letter, meaning, str);
	exit(X_ABORT);
/* NOTREACHED */
}

static void
missingarg(letter, meaning)
	int letter;
	char *meaning;
{

	(void)fprintf(stderr, "The '%c' flag (%s) requires an argument\n",
	    letter, meaning);
	exit(X_ABORT);
}

int	sighup(){	msg("SIGHUP()  try rewriting\n"); sigAbort();}
int	sigtrap(){	msg("SIGTRAP()  try rewriting\n"); sigAbort();}
int	sigfpe(){	msg("SIGFPE()  try rewriting\n"); sigAbort();}
int	sigbus(){	msg("SIGBUS()  try rewriting\n"); sigAbort();}
int	sigsegv(){	msg("SIGSEGV()  ABORTING!\n"); abort();}
int	sigalrm(){	msg("SIGALRM()  try rewriting\n"); sigAbort();}
int	sigterm(){	msg("SIGTERM()  try rewriting\n"); sigAbort();}

sigAbort()
{
	if (pipeout) {
		msg("Unknown signal, cannot recover\n");
		dumpabort();
	}
	msg("Rewriting attempted as response to unknown signal.\n");
	fflush(stderr);
	fflush(stdout);
	close_rewind();
	exit(X_REWRITE);
}

char *rawname(cp)
	char *cp;
{
	static char rawbuf[32];
	char *dp = rindex(cp, '/');

	if (dp == 0)
		return ((char *) 0);
	*dp = '\0';
	strcpy(rawbuf, cp);
	*dp++ = '/';
	strcat(rawbuf, "/r");
	strcat(rawbuf, dp);
	return (rawbuf);
}

char *deraw(cp)
	char *cp;
{
	static char rawbuf[32];
	register char *dp;
	register char *tp;

	strcpy(rawbuf, cp);

	dp = rindex(rawbuf, '/');
	if (dp++ == 0)
		return((char *) 0);

	tp = dp++;
	while (*tp++ = *dp++);
	return(rawbuf);
}

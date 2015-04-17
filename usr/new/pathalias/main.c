/* pathalias -- by steve bellovin, as told to peter honeyman */
#ifndef lint
static char	*sccsid = "@(#)main.c	9.1 87/10/04";
#endif

#define MAIN	/* for sccsid in header files */

#include "def.h"

/* exports */
extern void die();
char *Cfile;	/* current input file */
char *Graphout;	/* file for dumping edges (-g option) */
char *Linkout;	/* file for dumping shortest path tree */
char **Argv;	/* external copy of argv (for input files) */
p_node Home;	/* node for local host */
int Cflag;	/* print costs (-c option) */
int Dflag;	/* penalize routes beyond domains (-D option) */
int Iflag;	/* ignore case (-i option) */
int Tflag;	/* trace links (-t option) */
int Vflag;	/* verbose (-v option) */
int Fflag;	/* print cost of first hop */
int Argc;	/* external copy of argc (for input files) */
long Lineno = 1;/* line number within current input file */

/* imports */
extern long allocation();
extern void wasted(), mapit(), penalize(), hashanalyze(), deadlink();
extern void printit();
extern char *local();
extern p_node addnode();
#ifndef TMPFILES
#define getnode(x) x
#define getlink(y) y
#else  /*TMPFILES*/
void initstruct();
extern node *getnode();
extern link *getlink();
extern char *nfile, *lfile, *sfile;
extern long nhits, lhits, nmisses, lmisses;
#endif /*TMPFILES*/
extern char *optarg;
extern int optind;
extern long Lcount, Ncount;

#define USAGE "usage: %s [-vciDf] [-l localname] [-d deadlink] [-t tracelink] [-g edgeout] [-s treeout] [-a avoid] [files ...]\n"

main(argc, argv) 
	register int argc; 
	register char **argv;
{	char *locname = 0, buf[32], *bang;
	register int c;
	int errflg = 0;

	setbuf(stderr, (char *) 0);
	(void) allocation();	/* initialize data space monitoring */
	Cfile = "[deadlinks]";	/* for tracing dead links */
	Argv = argv;
	Argc = argc;

#ifdef TMPFILES
	initstruct();		/* initialize the node cache, etc. */
#endif /*TMPFILES*/

	while ((c = getopt(argc, argv, "a:cd:Dfg:il:s:t:v")) != EOF)
		switch(c) {
		case 'a':	/* adjust cost out of arg */
			penalize(optarg, DEFPENALTY);
			break;
		case 'c':	/* print cost info */
			Cflag++;
			break;
		case 'd':	/* dead host or link */
			if ((bang = index(optarg, '!')) != 0) {
				*bang++ = 0;
				deadlink(addnode(optarg), addnode(bang));
			} else
				deadlink(addnode(optarg), (p_node) 0);
			break;
		case 'D':	/* penalize routes beyond domains */
			Dflag++;
			break;
		case 'f':	/* print cost of first hop */
			Cflag++;
			Fflag++;
			break;
		case 'g':	/* graph output file */
			Graphout = optarg;
			break;
		case 'i':	/* ignore case */
			Iflag++;
			break;
		case 'l':	/* local name */
			locname = optarg;
			break;
		case 's':	/* show shortest path tree */
			Linkout = optarg;
			break;
		case 't':	/* trace this link */
			if (tracelink(optarg) < 0) {
				fprintf(stderr, "%s: can trace only %d links\n", Argv[0], NTRACE);
				exit(1);
			}
			Tflag = 1;
			break;
		case 'v':	/* verbose stderr, mixed blessing */
			Vflag++;
#ifndef TMPFILES	/* This could have unexpected side effects. */
			if (Vflag == 1) {
				/* v8 pi snarf, benign EBADF elsewhere */
				sprintf(buf, "/proc/%05d\n", getpid());
				write(3, buf, strlen(buf));
			}
#endif /*TMPFILES*/
			break;
		default:
			errflg++;
		}

	if (errflg) {
		fprintf(stderr, USAGE, Argv[0]);
		exit(1);
	}
	argv += optind;		/* kludge for yywrap() */

	if (*argv)
		freopen("/dev/null", "r", stdin);
	else
		Cfile = "[stdin]";

	if (!locname) 
		locname = local();
	if (*locname == 0) {
		locname = "lostinspace";
		fprintf(stderr, "%s: using \"%s\" for local name\n",
				Argv[0], locname);
	}

	Home = addnode(locname);	/* add home node */
	getnode(Home)->n_cost = 0;	/* doesn't cost to get here */

	yyparse();			/* read in link info */

	if (Vflag > 1)
		hashanalyze();
	vprintf(stderr, "%ld vertices, %ld edges\n", Ncount, Lcount);
	vprintf(stderr, "allocation is %ldk after parsing\n", allocation());

	Cfile = "[backlinks]";	/* for tracing back links */
	Lineno = 0;

	/* compute shortest path tree */
	mapit();
	vprintf(stderr, "allocation is %ldk after mapping\n", allocation());

	/* traverse tree and print paths */
	printit();
	vprintf(stderr, "allocation is %ldk after printing\n", allocation());

	wasted();	/* how much was wasted in memory allocation? */

#ifdef TMPFILES		/* print out handy statistics, and clean up. */
	vprintf(stderr, "node cache %ld hits, %ld misses\n", nhits, nmisses);
	vprintf(stderr, "link cache %ld hits, %ld misses\n", lhits, lmisses);
#ifndef DEBUG
	(void) unlink(nfile);
	(void) unlink(lfile);
	(void) unlink(sfile);
#endif /*DEBUG*/
#endif /*TMPFILES*/

	return 0;
}

void
die(s)
	char *s;
{
	fprintf(stderr, "%s: %s; notify the authorities\n", Argv[0], s);
#ifdef DEBUG
		fflush(stdout);
		fflush(stderr);
		abort();
#else
#ifdef TMPFILES
		(void) unlink(nfile);
		(void) unlink(lfile);
		(void) unlink(sfile);
#endif /*TMPFILES*/
		exit(-1);
#endif
}

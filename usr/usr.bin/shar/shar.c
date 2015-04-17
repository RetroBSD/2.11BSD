/*
	Shar puts readable text files together in a package
	from which they are extracted with /bin/sh and friends.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#ifndef lint
static	char	sccsid[] = "@(#)shar.c 2.0 (Usenet) 3/11/86";
#endif

typedef	int 	Boole;
#define	TRUE	((Boole) 1)
#define	FALSE	((Boole) 0)
typedef	int 	Status;
#define	SUCCESS	0
#define	FAILURE	1

/* GLOBALS */
int 	Lastchar;   /* the last character printed */
int 	Ctrlcount;  /* how many bad control characters are in file */

/* COMMANDS */
#define	EXTRACT "#! /bin/sh"          /* magic exec string at shar file start */
#define	PATH    "/bin:/usr/bin:$PATH" /* search path for programs */
#define	CAT     "cat";                /* /bin/cat */
#define	SED     "sed 's/^%s//'"       /* /bin/sed removes Prefix from lines */
#define	MKDIR   "mkdir"               /* make a new dirctory */
#define	CHMOD   "chmod"               /* change file mode */
#define	CHDIR   "cd"                  /* change current directory */
#define	TEST    "test"                /* /bin/test files */
#define	WC_C    "wc -c <"             /* counts chars in file */
#define	ECHO    "echo shar:"          /* echo a message to extractor */
#define	DECODE  "uudecode"            /* used to decode uuencoded files */

/*FUNCTION main: traverse files to make archive to standard output */
main (argc, argv) char **argv;	
	{
	int 	shar ();
	int 	optind;

	if ((optind = initial (argc, argv)) < 0)
		exit (FAILURE);

	if (header (argc, argv, optind))
		exit (FAILURE);

	while (optind < argc)
		traverse (argv[optind++], shar);

	footer ();

	exit (SUCCESS);
	}

/* OPTIONS */
Boole	Verbose = FALSE;       /* provide append/extract feedback */
Boole	Basename = FALSE;      /* extract into basenames */
Boole	Count = FALSE;         /* count characters to check transfer */
Boole	Overcheck = TRUE;      /* check overwriting */
Boole	Uucode = FALSE;        /* uuencode the file */
char	*Delim = "SHAR_EOF";   /* put after each file */
char	Filter[100] = CAT;     /* used to extract archived files */
char	*Prefix = NULL;        /* line prefix to avoid funny chars */
Boole	Modeset = FALSE;       /* set exact modes on extraction */

/*FUNCTION: initial: do option parsing and any setup */
int /* returns the index of the first operand file */
initial (argc, argv) char **argv;
	{
	int 	errflg = 0;
	extern	int 	optind;
	extern	char	*optarg;
	int 	C;
	char	*optstring = "abcmsuvp:d:";
	char	*usage = "[-abcmsuv] [-p prefix] [-d delim] files > archive";

	while ((C = getopt (argc, argv, optstring)) != EOF)
		switch (C)
			{
			case 'u': Uucode = TRUE; break;
			case 'b': Basename = TRUE; break;
			case 'c': Count = TRUE; break;
			case 'd': Delim = optarg; break;
			case 'm': Modeset = TRUE; break;
			case 's': /* silent running */
				Overcheck = FALSE;
				Verbose = FALSE;
				Count = FALSE;
				Prefix = NULL;
				break;
			case 'a': /* all the options */
				Verbose = TRUE;
				Count = TRUE;
				Basename = TRUE;
				/* fall through to set prefix */
				optarg = "	X";
				/* FALLTHROUGH */
			case 'p': (void) sprintf (Filter, SED, Prefix = optarg); break;
			case 'v': Verbose = TRUE; break;
			default: errflg++;
			}

	if (errflg || optind == argc)
		{
		if (optind == argc)
			fprintf (stderr, "shar: No input files\n");
		fprintf (stderr, "Usage: shar %s\n", usage);
		return (-1);
		}

	return (optind);
	}

/*FUNCTION header: print header for archive */
header (argc, argv, optind)
char	**argv;
	{
	int 	i;
	Boole	problems = FALSE;
	long	clock;
	char	*ctime ();
	char	*getenv ();
	char	*NAME = getenv ("NAME");
	char	*ORG = getenv ("ORGANIZATION");

	for (i = optind; i < argc; i++)
		if (access (argv[i], 4)) /* check read permission */
			{
			fprintf (stderr, "shar: Can't read '%s'\n", argv[i]);
			problems++;
			}

	if (problems)
		return (FAILURE);

	printf ("%s\n", EXTRACT);
	printf ("# This is a shell archive, meaning:\n");
	printf ("# 1. Remove everything above the %s line.\n", EXTRACT);
	printf ("# 2. Save the resulting text in a file.\n");
	printf ("# 3. Execute the file with /bin/sh (not csh) to create:\n");
	for (i = optind; i < argc; i++)
		printf ("#\t%s\n", argv[i]);
	(void) time (&clock);
	printf ("# This archive created: %s", ctime (&clock));
	if (NAME)
		printf ("# By:\t%s (%s)\n", NAME, ORG ? ORG : "");
	printf ("export PATH; PATH=%s\n", PATH);
	return (SUCCESS);
	}

/*FUNCTION footer: print end of shell archive */
footer ()
	{
	printf ("exit 0\n");
	printf ("#\tEnd of shell archive\n");
	}

/* uuencode options available to send cntrl and non-ascii chars */
/* really, this is getting to be too much like cpio or tar */

/* ENC is the basic 1 character encoding function to make a char printing */
#define ENC(c) (((c) & 077) + ' ')

/*FUNCTION uuarchive: simulate uuencode to send files */
Status
uuarchive (input, protection, output)
char	*input;
int 	protection;
char	*output;
	{
	FILE	*in;
	if (in = fopen (input, "r"))
		{
		printf ("%s << \\%s\n", DECODE, Delim);
		printf ("begin %o %s\n", protection, output);
		uuencode (in, stdout);
		printf ("end\n");
		fclose (in);
		return (SUCCESS);
		}
	return (FAILURE);
	}

uuencode (in, out)
FILE *in, *out;
	{
	char	buf[80];
	int 	i, n;
	for (;;)
		{
		n = fread (buf, 1, 45, in);
		putc (ENC(n), out);
		for (i = 0; i < n; i += 3)
			outdec (&buf[i], out);
		putc ('\n', out);
		if (n <= 0)
			break;
		}
	}

/* output one group of 3 bytes, pointed at by p, on file ioptr */
outdec (p, ioptr)
char	*p;
FILE	*ioptr;
	{
	int c1, c2, c3, c4;
	c1 = *p >> 2;
	c2 = (*p << 4) & 060 | (p[1] >> 4) & 017;
	c3 = (p[1] << 2) & 074 | (p[2] >> 6) & 03;
	c4 = p[2] & 077;
	putc (ENC(c1), ioptr);
	putc (ENC(c2), ioptr);
	putc (ENC(c3), ioptr);
	putc (ENC(c4), ioptr);
	}

/*FUNCTION archive: make archive of input file to be extracted to output */
archive (input, output)
char	*input, *output;
	{
	char	buf[BUFSIZ];
	FILE	*ioptr;

	if (ioptr = fopen (input, "r"))
		{
		Ctrlcount = 0;    /* no bad control characters so far */
		Lastchar = '\n';  /* simulate line start */

		printf ("%s << \\%s > '%s'\n", Filter, Delim, output);

		if (Prefix)
			{
			while (fgets (buf, BUFSIZ, ioptr))
				{
				outline (Prefix);
				outline (buf);
				}
			}
		else copyout (ioptr);

		if (Lastchar != '\n') /* incomplete last line */
			putchar ('\n');   /* Delim MUST begin new line! */

		if (Count == TRUE && Lastchar != '\n')
			printf ("%s \"a missing newline was added to '%s'\"\n", ECHO, input);
		if (Count == TRUE && Ctrlcount)
			printf ("%s \"%d control character%s may be missing from '%s'\"\n",
				ECHO, Ctrlcount, Ctrlcount > 1 ? "s" : "", input);

		(void) fclose (ioptr);
		return (SUCCESS);
		}
	else
		{
		fprintf (stderr, "shar: Can't open '%s'\n", input);
		return (FAILURE);
		}
	}

/*FUNCTION copyout: copy ioptr to stdout */
/*
	Copyout copies its ioptr almost as fast as possible
	except that it has to keep track of the last character
	printed.  If the last character is not a newline, then
	shar has to add one so that the end of file delimiter
	is recognized by the shell.  This checking costs about
	a 10% difference in user time.  Otherwise, it is about
	as fast as cat.  It also might count control characters.
*/
#define	badctrl(c) (iscntrl (c) && !isspace (c))
copyout (ioptr)
register	FILE	*ioptr;
	{
	register	int 	C;
	register	int 	L;
	register	count;

	count = Count;

	while ((C = getc (ioptr)) != EOF)
		{
		if (count == TRUE && badctrl (C))
			Ctrlcount++;
		L = putchar (C);
		}

	Lastchar = L;
	}

/*FUNCTION outline: output a line, recoring last character */
outline (s)
register	char	*s;
	{
	if (*s)
		{
		while (*s)
			{
			if (Count == TRUE && badctrl (*s)) Ctrlcount++;
			putchar (*s++);
			}
		Lastchar = *(s-1);
		}
	}

/*FUNCTION shar: main archiving routine passed to directory traverser */
shar (file, type, pos)
char	*file;     /* file or directory to be processed */
int 	type;      /* either 'f' for file or 'd' for directory */
int 	pos;       /* 0 going in to a file or dir, 1 going out */
	{
	struct	stat	statbuf;
	char	*basefile = file;
	int 	protection;

	if (!strcmp (file, "."))
		return;

	if (stat (file, &statbuf))
		statbuf.st_size = 0;
	else
		protection = statbuf.st_mode & 07777;

	if (Basename == TRUE)
		{
		while (*basefile) basefile++; /* go to end of name */
		while (basefile > file && *(basefile-1) != '/')
			basefile--;
		}

	if (pos == 0) /* before the file starts */
		{
		beginfile (basefile, type, statbuf.st_size, protection);
		if (type == 'f')
			{
			if (Uucode)
				{
				if (uuarchive (file, protection, basefile) != SUCCESS)
					exit (FAILURE);
				}
			else
				if (archive (file, basefile) != SUCCESS)
					exit (FAILURE);
			}
		}
	else /* pos == 1, after the file is archived */
		endfile (basefile, type, statbuf.st_size, protection);
	}

/*FUNCTION beginfile: do activities before packing up a file */
beginfile (basefile, type, size, protection)
char	*basefile;  /* name of the target file */
int 	type;       /* either 'd' for directory, or 'f' for file */
long 	size;       /* size of the file */
int 	protection; /* chmod protection bits */
	{
	if (type == 'd')
		{
		printf ("if %s ! -d '%s'\n", TEST, basefile);
		printf ("then\n");
		if (Verbose == TRUE)
			printf ("	%s \"creating directory '%s'\"\n", ECHO, basefile);
		printf ("	%s '%s'\n", MKDIR, basefile);
		printf ("fi\n");
		if (Verbose == TRUE)
			printf ("%s \"entering directory '%s'\"\n", ECHO, basefile);
		printf ("%s '%s'\n", CHDIR, basefile);
		}
	else /* type == 'f' */
		{
		if (Verbose == TRUE)
			printf ("%s \"extracting '%s'\" '(%ld character%s)'\n",
				ECHO, basefile, size, size == 1 ? "" : "s");

		if (Overcheck == TRUE)
			{
			printf ("if %s -f '%s'\n", TEST, basefile);
			printf ("then\n");
			printf ("	%s \"will not over-write existing file '%s'\"\n",
				ECHO, basefile);
			printf ("else\n");
			}

		}
	}

/*FUNCTION endfile: do activities after packing up a file */
endfile (basefile, type, size, protection)
char	*basefile;  /* name of the target file */
int 	type;       /* either 'd' for directory, or 'f' for file */
long 	size;       /* size of the file */
int 	protection; /* chmod protection bits */
	{
	if (type == 'd')
		{
		if (Modeset == TRUE)
			printf ("%s %o .\n", CHMOD, protection);
		if (Verbose == TRUE)
			printf ("%s \"done with directory '%s'\"\n", ECHO, basefile);
		printf ("%s ..\n", CHDIR);
		}
	else /* type == 'f' (plain file) */
		{
		printf ("%s\n", Delim);
		if (Count == TRUE)
			{
			printf ("if %s %ld -ne \"`%s '%s'`\"\n", TEST, size, WC_C, basefile);
			printf ("then\n");
			printf ("	%s \"error transmitting '%s'\" ", ECHO, basefile);
			printf ("'(should have been %ld character%s)'\n",
				size, size == 1 ? "" : "s");
			printf ("fi\n");
			}
		if (Uucode == FALSE) /* might have to chmod by hand */
			{
			if (Modeset == TRUE) /* set all protection bits (W McKeeman) */
				printf ("%s %o '%s'\n",
						CHMOD, protection, basefile);
			else if (protection & 0111) /* executable -> chmod +x */
				printf ("%s +x '%s'\n", CHMOD, basefile);
			}
		if (Overcheck == TRUE)
			printf ("fi\n");
		}
	}

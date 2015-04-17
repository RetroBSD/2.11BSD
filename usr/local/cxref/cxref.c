/*
** cxref.c
**
** C driver for Cxref program.
** does argument handling, then builds the right
** shell commands for passing to the system() routine.
**
** Set up the argument vectors ourselves, the i/o with a pipe()
** call, and do all the forking and execing ourselves.
**
** Arnold Robbins, Information and Computer Science, Georgia Tech
**	gatech!arnold
** Copyright (c) 1984 by Arnold Robbins
** All rights reserved
** This program may not be sold, but may be distributed
** provided this header is included.
*/

#include <stdio.h>
#include <ctype.h>
#include <signal.h>

#define TRUE	1
#define FALSE	0

char name[BUFSIZ];	/* save command name */

int xargc;		/* make argc and argv available globally */
char **xargv;

int width = 0;		/* output width */

int sepflag = FALSE;	/* do each one separately */

int iflag = TRUE;	/* print out ints */
int fflag = TRUE;	/* print out floats */
int cflag = TRUE;	/* print out chars */
int sflag = TRUE;	/* print out strings */
int Fflag = FALSE;	/* fold case in indentifiers */

int ancestor;		/* id of this process, used by children */

char *filename();	/* turns "-" into "stdin" */

#define do_pipe(x)	if (pipe(x) < 0) { fprintf(stderr, "x: pipe failed\n");\
				fflush(stderr); exit (1); }

main(argc, argv)
int argc;
char **argv;
{
	int i;
	int catchem();

	signal (SIGQUIT, catchem);
	signal (SIGINT, catchem);

	strcpy (name, filename(argv[0]));

	ancestor = getpid();

	for(argv++, argc--; argc > 0; argv++, argc--)
		if (argv[0][0] != '-')
			break;
		else if(argv[0][1] == '\0')	/* filename of "-" */
			break;
		else
			for(i = 1; argv[0][i] != '\0'; i++)
			{
				switch(argv[0][i]) {
				case 'F':
					Fflag = TRUE;
					break;

				case 'S':
					sepflag = TRUE;
					break;
				
				case 'C':
					/* leave out all constants */
					cflag =
					iflag =
					fflag =
					sflag = FALSE;
					break;

				case 'c':
					cflag = FALSE;
					break;
				
				case 'i':
					iflag = FALSE;
					break;
				
				case 'f':
					fflag = FALSE;
					break;
				
				case 's':
					sflag = FALSE;
					break;
				
				case 'w':
					if (isdigit(argv[0][i+1]))
					{
						width = 0;
						for(i++; isdigit(argv[0][i]); i++)
							width = width * 10 + argv[0][i] - '0';
						i--;
					}
					else
					{
						width = atoi(argv[1]);
						argv++;
						argc--;
					}
					break;

				default:
					usage();
					break;
				}
			}
	
	if (width != 0)
		if (width < 51)
			width = 80;
		else if (width > 132)
			width = 132;

	xargc = argc;
	xargv = argv;

	setargs();		/* set up various argv buffers */

	runprogs();		/* set up and run pipelines */

	exit (0);
}

/* argv vectors for various commands */

char *docxref[BUFSIZ] = { "docxref" };	/* allows BUFSIZ - 2 files */
char *cxrfilt[] = { "cxrfilt", NULL, NULL, NULL };
char *fmtxref[] = { "fmtxref", NULL, NULL, NULL };
char *sort1[] = { "sort", "-u", "+0b", "-2", "+2n", NULL, NULL };
char *sort2[] = { "sort", "-u", "+0n", "-1", "+1b", "-2", "+2n", NULL };
char *sort3[] = { "sort", "-u", "+0n", "+1n", "-2", "+2b", "-3", "+3n", NULL };

/* pipes to connect programs */

typedef int PIPE[2];

PIPE pipe1, pipe2, pipe3;

setargs()		/* initialize argv vectors */
{
	static char widthbuf[100];
	static char pidbuf[100];

	if (width != 0)
	{
		fmtxref[1] = "-w";
		sprintf(widthbuf, "%d", width);
		fmtxref[2] = widthbuf;
		fmtxref[3] = NULL;
	}

	sprintf(pidbuf, "%d", getpid());

	if (Fflag)
		sort1[5] = "-f";	/* fold case in identifiers */

	if (! cflag && sflag)
	{
		cxrfilt[1] = "-c";
		cxrfilt[2] = pidbuf;
		cxrfilt[3] = NULL;
	}

	else if (cflag && ! sflag)
	{
		cxrfilt[1] = "-s";
		cxrfilt[2] = pidbuf;
		cxrfilt[3] = NULL;
	}

	else if (! cflag && ! sflag)
	{
		cxrfilt[1] = "-cs";
		cxrfilt[2] = pidbuf;
		cxrfilt[3] = NULL;
	}

	else
	{
		cxrfilt[1] = pidbuf;
		cxrfilt[2] = NULL;
	}
}


/*
flow of control is:

	docxref  pipe1 sort1 pipe2 cxrfilt -userargs pipe3 fmtxref -userargs
	sort2 pipe1 cxrfilt -i pipe2 fmtxref -userargs
	sort3 pipe1 cxrfilt -f pipe2 fmtxref -userargs
*/

runprogs()		/* run the programs, obeying user's options */
{
	int i;

	if (sepflag)
	{
		for (i = 0; i < xargc; i++)
		{
			printf("\tC Cross Reference Listing of %s\n\n",
					filename(xargv[i]));
			fflush(stdout);

			docxref[1] = xargv[i];
			docxref[2] = NULL;

			idens();

			if (iflag)
				integers();

			if (fflag)
				floats();

			fflush(stdout);

			if (!isatty(fileno(stdout)))
				putchar('\f');
		}
	}
	else
	{
		if (xargc == 1)
			printf("\tC Cross Reference Listing of %s\n\n",
				filename(xargv[0]));
		else
			printf("\tC Cross Reference Listing\n\n");
		fflush(stdout);

		for (i = 0; xargv[i] != NULL; i++)
			docxref[i+1] = xargv[i];

		docxref[i+1] = NULL;

		idens();

		if (iflag)
			integers();

		if (fflag)
			floats();

		fflush(stdout);

		if (! isatty(fileno(stdout)))
			putchar('\f');
	}

	deltemps();
}

deltemps()	/* delete temp files used for ints and floats */
{
	char buf[BUFSIZ];
	int i;

	for (i = 1; i <= 2; i++)
	{
		sprintf(buf, "/tmp/cxr.%d.%d", getpid(), i);
		unlink(buf);
	}
}

/*
 * now begins the nitty gritty work of forking and setting up pipes.
 */

int level;	/* how many children down are we */

idens()		/* cross reference identifiers */
{
	int status;
	int pid;
	int ischild;
	char buf[BUFSIZ];

	level = 0;	/* starting off as grandparent */

	ischild = ((pid = fork()) == 0);
	
retest:
	switch (level) {
	case 0:			/* first fork */
		if (ischild)
		{
			level++;
			do_pipe(pipe3);

			if (ischild = ((pid = fork()) == 0))
				goto retest;

			close(pipe3[1]);	/* doesn't need this */

			close (0);
			dup(pipe3[0]);
			close(pipe3[0]);
			sprintf (buf, "%s/fmtxref", SRCDIR);
			execv (buf, fmtxref);
			fprintf (stderr, "couldn't exec '%s'\n", buf);
			exit (1);
		}
		else
			while (wait(&status) != pid)
				;
		break;
	
	case 1:			/* second fork */
		level++;
		close (pipe3[0]);

		close(1);
		dup(pipe3[1]);
		close(pipe3[1]);

		/* set up i/o for next child */
		do_pipe(pipe2);

		if (ischild = ((pid = fork()) == 0))
			goto retest;

		close (pipe2[1]);
		close (0);
		dup(pipe2[0]);
		close (pipe2[0]);

		sprintf (buf, "%s/cxrfilt", SRCDIR);
		execv (buf, cxrfilt);
		fprintf (stderr, "couldn't exec '%s'\n", buf);
		exit (1);
		break;

	case 2:
		level++;
		close (pipe2[0]);

		close(1);
		dup(pipe2[1]);
		close(pipe2[1]);	/* now writes to parent */

		/* set up to read from next child */
		do_pipe(pipe1);

		if (ischild = ((pid = fork()) == 0))
			goto retest;

		close (pipe1[1]);

		close (0);
		dup(pipe1[0]);
		close (pipe1[0]);
		execv (SORT, sort1);
		fprintf (stderr, "couldn't exec '%s'\n", SORT);
		exit (1);
		break;

	case 3:
		level++;
		close (pipe1[0]);

		close(1);
		dup(pipe1[1]);
		close(pipe1[1]);	/* now writes to parent */

		sprintf(buf, "%s/docxref", SRCDIR);
		execv (buf, docxref);
		fprintf (stderr, "couldn't exec '%s'\n", buf);
		exit (1);
		break;

	default:
		fprintf(stderr, "in cxref (idens): can't happen\n");
		fflush(stderr);
		break;
	}
}

#include <sys/types.h>
#include <sys/stat.h>

integers()
{
	int status;
	int pid;
	int ischild;
	char buf[BUFSIZ];
	struct stat fbuf;

	sprintf(buf, "/tmp/cxr.%d.1", ancestor);
	if (stat(buf, &fbuf) >= 0 && fbuf.st_size > 0)
		;	/* file is not empty */
	else
		return;

	level = 0;	/* starting off as grandparent */

	ischild = ((pid = fork()) == 0);
	
retest:
	switch (level) {
	case 0:			/* first fork */
		if (ischild)
		{
			level++;
			do_pipe(pipe2);

			if (ischild = ((pid = fork()) == 0))
				goto retest;

			close(pipe2[1]);	/* doesn't need this */

			close (0);
			dup(pipe2[0]);
			close(pipe2[0]);
			sprintf (buf, "%s/fmtxref", SRCDIR);
			execv (buf, fmtxref);
			fprintf (stderr, "couldn't exec '%s'\n", buf);
			exit (1);
		}
		else
			while (wait(&status) != pid)
				;
		break;
	
	case 1:			/* second fork */
		level++;
		close (pipe2[0]);

		close(1);
		dup(pipe2[1]);
		close(pipe2[1]);

		/* set up i/o for next child */
		do_pipe(pipe1);

		if (ischild = ((pid = fork()) == 0))
			goto retest;

		close (pipe1[1]);
		close (0);
		dup(pipe1[0]);
		close (pipe1[0]);

		cxrfilt[1] = "-i";
		cxrfilt[2] = NULL;

		sprintf (buf, "%s/cxrfilt", SRCDIR);
		execv (buf, cxrfilt);
		fprintf (stderr, "couldn't exec '%s'\n", buf);
		exit (1);
		break;

	case 2:
		level++;
		close (pipe1[0]);

		close(1);
		dup(pipe1[1]);
		close(pipe1[1]);	/* now writes to parent */

		/* read from tempfile */

		close (0);
		sprintf (buf, "/tmp/cxr.%d.1", ancestor);
		open (buf, 0);		/* will be fd 0 */

		execv (SORT, sort2);
		fprintf (stderr, "couldn't exec '%s'\n", SORT);
		exit (1);
		break;

	default:
		fprintf(stderr, "in cxref(integers): can't happen\n");
		fflush(stderr);
		break;
	}
}

floats()
{
	int status;
	int pid;
	int ischild;
	char buf[BUFSIZ];
	struct stat fbuf;

	sprintf(buf, "/tmp/cxr.%d.2", ancestor);
	if (stat(buf, &fbuf) >= 0 && fbuf.st_size > 0)
		;	/* file is not empty */
	else
		return;

	level = 0;	/* starting off as grandparent */

	ischild = ((pid = fork()) == 0);
	
retest:
	switch (level) {
	case 0:			/* first fork */
		if (ischild)
		{
			level++;
			do_pipe(pipe2);

			if (ischild = ((pid = fork()) == 0))
				goto retest;

			close(pipe2[1]);	/* doesn't need this */

			close (0);
			dup(pipe2[0]);
			close(pipe2[0]);
			sprintf (buf, "%s/fmtxref", SRCDIR);
			execv (buf, fmtxref);
			fprintf (stderr, "couldn't exec '%s'\n", buf);
			exit (1);
		}
		else
			while (wait(&status) != pid)
				;
		break;
	
	case 1:			/* second fork */
		level++;
		close (pipe2[0]);

		close(1);
		dup(pipe2[1]);
		close(pipe2[1]);

		/* set up i/o for next child */
		do_pipe(pipe1);

		if (ischild = ((pid = fork()) == 0))
			goto retest;

		close (pipe1[1]);
		close (0);
		dup(pipe1[0]);
		close (pipe1[0]);

		cxrfilt[1] = "-f";
		cxrfilt[2] = NULL;

		sprintf (buf, "%s/cxrfilt", SRCDIR);
		execv (buf, cxrfilt);
		fprintf (stderr, "couldn't exec '%s'\n", buf);
		exit (1);
		break;

	case 2:
		level++;
		close (pipe1[0]);

		close(1);
		dup(pipe1[1]);
		close(pipe1[1]);	/* now writes to parent */

		/* read from tempfile */

		close (0);
		sprintf (buf, "/tmp/cxr.%d.2", ancestor);
		open (buf, 0);		/* will be fd 0 */

		execv (SORT, sort3);
		fprintf (stderr, "couldn't exec '%s'\n", SORT);
		exit (1);
		break;

	default:
		fprintf(stderr, "in cxref(floats): can't happen\n");
		fflush(stderr);
		break;
	}
}

usage()
{
	fprintf(stderr, "usage: %s [-SCcsif] [-w width] [files]\n", name);
	fflush(stderr);
	exit (1);
}

char *filename(fname)
char *fname;
{
	char *cp, *basename();

	cp = basename(fname);

	return ( strcmp(cp, "-") == 0 ? "stdin" : cp);
}

catchem()	/* simple signal catcher */
{
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);

	deltemps();

	exit (0);
}

#include "basename.c"

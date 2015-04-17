# include	"../ingres.h"
# include	"../aux.h"
# include	"../unix.h"

# define	CLOSED		077

/*
**  Initialize INGRES Process
**
**	All initialization for a process is performed, including
**	initialization of trace info and global variables.  Typical
**	call is
**		initproc(procname, argv);
**	with procname being a string (e.g., "PARSER"),
**	argv is the parameter of main().
**
**	Calling this routine should be one of the first things done
**	in  main().
**
**	Information passed in argv is as follows:
**	argv[0] - line from process table
**	argv[1] - pipe vector
**		argv[1][0] - R_up
**		argv[1][1] - W_up
**		argv[1][2] - R_down
**		argv[1][3] - W_down
**		argv[1][4] - R_front
**		argv[1][5] - W_front
**	argv[2] - Fileset - unique character string for temp file names.
**	argv[3] - Usercode - two character user identifier
**	argv[4] - Database - database name
**	argv[5->n] - Xparams - additional parameters,
**		different for each process.  See .../files/proctab
**		for what each process actually gets.
**
**	The pipe descriptors are passed with the 0100 bit set, so that
**	they will show up in a readable fashion on a "ps".
**
**	A flag "Equel" is set if running an EQUEL program.  The flag
**	is set to 1 if the EQUEL program is a 6.0 or 6.0 EQUEL program,
**	and to 2 if it is a 6.2 EQUEL program.
**
**	User flags "[+-]x" are processed.  Note that the syntax of some of
**	these are assumed to be correct.  They should be checked in
**	ingres.c.
**
**	Signal 2 is caught and processed by 'rubcatch', which will
**	eventually call 'rubproc', a user supplied routine which cleans
**	up the relevant process.
**
**	History:
**		6/29/79 (eric) (mod 6) -- Standalone variable added.
*/

char		*Fileset;	/* unique string to append to temp file names */
char		*Usercode;	/* code to append to relation names */
char		*Database;	/* database name */
char		**Xparams;	/* other parameters */
char		*Pathname;	/* pathname of INGRES subtree */
int		R_up;		/* pipe descriptor: read from above */
int		W_up;		/* pipe descriptor: write to above */
int		R_down;		/* pipe descriptor: read from below */
int		W_down;		/* pipe descriptor: write to below */
int		R_front;	/* pipe descriptor: read from front end */
int		W_front;	/* pipe descriptor: write to front end */
int		W_err;		/* pipe descriptor: write error message (usually W_up */
int		Equel;		/* flag set if running an EQUEL program */
int		Rublevel;	/* rubout level, set to -1 if turned off */
struct out_arg	Out_arg;	/* output arguments */
int		Standalone;	/* not standalone */

initproc(procname, argv)
char	*procname;
char	**argv;
{
	extern char	*Proc_name;	/* defined in syserr.c */
	register char	*p;
	register char	**q;
	register int	fd;
	extern		rubcatch();	/* defined in rub.c */
	static int	reenter;

	Standalone = FALSE;
	Proc_name = procname;
	reenter = 0;
	setexit();
	if (reenter++)
		exit(-1);
	if (signal(2, 1) == 0)
		signal(2, &rubcatch);
	else
		Rublevel = -1;
	p = argv[1];
	R_up = fd = *p++ & 077;
	if (fd == CLOSED)
		R_up = -1;
	W_up = fd = *p++ & 077;
	if (fd == CLOSED)
		W_up = -1;
	W_err = W_up;
	R_down = fd = *p++ & 077;
	if (fd == CLOSED)
		R_down = -1;
	W_down = fd = *p++ & 077;
	if (fd == CLOSED)
		W_down = -1;
	R_front = fd = *p++ & 077;
	if (fd == CLOSED)
		R_front = -1;
	W_front = fd = *p++ & 077;
	if (fd == CLOSED)
		W_front = -1;

	q = &argv[2];
	Fileset = *q++;
	Usercode = *q++;
	Database = *q++;
	Pathname = *q++;
	Xparams = q;

	/* process flags */
	for (; (p = *q) != -1 && p != NULL; q++)
	{
		if (p[0] != '-')
			continue;
		switch (p[1])
		{
		  case '&':	/* equel program */
			Equel = 1;
			if (p[6] != '\0')
				Equel = 2;
			break;

		  case 'c':	/* c0 sizes */
			atoi(&p[2], &Out_arg.c0width);
			break;

		  case 'i':	/* iNsizes */
			switch (p[2])
			{

			  case '1':
				atoi(&p[3], &Out_arg.i1width);
				break;

			  case '2':
				atoi(&p[3], &Out_arg.i2width);
				break;

			  case '4':
				atoi(&p[3], &Out_arg.i4width);
				break;

			}
			break;

		  case 'f':	/* fN sizes */
			p = &p[3];
			fd = *p++;
			while (*p != '.')
				p++;
			*p++ = 0;
			if ((*q)[2] == '4')
			{
				atoi(&(*q)[4], &Out_arg.f4width);
				atoi(p, &Out_arg.f4prec);
				Out_arg.f4style = fd;
			}
			else
			{
				atoi(&(*q)[4], &Out_arg.f8width);
				atoi(p, &Out_arg.f8prec);
				Out_arg.f8style = fd;
			}
			*--p = '.';	/* restore parm for dbu's */
			break;

		  case 'v':	/* vertical seperator */
			Out_arg.coldelim = p[2];
			break;

		}
	}
}

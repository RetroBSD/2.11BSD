/*
 *	@(#)	ingres.c	1.1	(2.11BSD)	1996/3/22
*/

#include	<errno.h>
# include	"../ingres.h"
# include	"../aux.h"
# include	"../access.h"
# include	"../lock.h"
# include	"../unix.h"


/*
**  INGRES -- INGRES startup
**
**	This program starts up the entire system.
**
**	Parameters:
**		1 -- database name
**		2 -- optional process table name
**		x -- flags of the form +x or -x may be freely inter-
**			sperced in the argument list.
**
**	Return:
**		none if successful
**		1 -- user error (no database, etc)
**		-1 -- system error
**
**	Flags:
**		-&xxxx -- EQUEL flag: xxxx are file descriptors for the
**			status return pipe, the command write pipe, the
**			data return pipe, and the data transfer pipe
**			respectively.
**		-@xxxx -- xxxx is same as EQUEL flag, but no flags
**			are set.
**		-*?? -- Masterid flag. Gives the siteid of the master
**			site in a distributed ingres. (Used in dist.
**			ingres' initproc() function.)
**		-|xxxx -- Network flag.  This flag is just passed to
**			the other processes, to be processed by the
**			DBU's.
**		-uusername -- set effective user to be username.  You
**			must be INGRES or the DBA for the database to
**			use this option.
**		-cN -- set minimum character field output width to be
**			N, default 6.  This is the fewest number of
**			characters which may be output in any "c" type
**			field.
**		-inN -- integer output width.  this is the width of
**			an integer field.  The small "n" is the size
**			of the internal field ("1", "2", or "4") and
**			N is the width of the field for that flag.
**			The defaults are -i16, -i26, and -i413.
**		-fnxN.M -- floating point output width and precision.
**			Small "n" is the internal width in bytes ("4"
**			or "8"), x is the format (f, F, g, G, e, E,
**			n, or N), N is the field width, and M is the
**			precision (number of digits after the decimal
**			point).  The formats are:
**			"f" or "F": FORTRAN-style F format: digits,
**				decimal point, digits, no exponent.
**			"e" or "E": FORTRAN-style E format: digits,
**				decimal point, digits, letter "e" (or
**				"E", depending on "x" in the param-
**				eter), and an exponent.  The scaling
**				factor is always one, that is, there
**				is always one digit before the decimal
**				point.
**			"g" or "G": F format if it will fit in the
**				field, otherwise E format.  Space is
**				always left at the right of the field
**				for the exponent, so that decimal
**				points will align.
**			"n" or "N": like G, except that space is not
**				left for the decimal point in F style
**				format (useful if you expect everything
**				to fit, but you're not sure).
**			The default is -fn10.3.
**		-vx -- set vertical seperator for print operations
**			and retrieves to the terminal to be "x".  The
**			default is vertical bar ("|").
**		+w -- database wait state.  If set ("+w"), you will
**			wait until the database is not busy.  If clear,
**			you will be informed if the database is busy.
**			If not specified, the same operations take
**			place depending on whether or not you are
**			running in background (determined by whether
**			or not your input is a teletype).  If in fore-
**			ground, you are informed; if in background,
**			you wait.
**		-M -- monitor trace flag
**		-P -- parser trace flag
**		-O -- ovqp trace flag
**		-Q -- qrymod trace flag
**		-D -- decomp trace flag
**		-Z -- dbu trace flag.  These flags require the 020 bit
**			in the status field of the users file to be
**			set.  The syntax is loose and is described
**			elsewhere.  Briefly, "-Z" sets all flags except
**			the last 20, "-Z4" sets flag 4, and "-Z5/7"
**			sets all flags from 5 through 7.
**		+L -- enable/disable upper to lower case mapping in the
**			parser.  Used for debugging.
**		-rmode -- retrieve into mode
**		-nmode -- index mode.  These flags give the default
**			modify mode for retrieve into and index.  They
**			default to cheapsort and isam.  "Mode" can be
**			any mode to modify except "truncated".
**		+a -- enable/disable autoclear function in monitor.
**			Default on.
**		+b -- enable/disable batch update.  Default on.
**			The 02 bit is needed to clear this flag.
**		+d -- enable/disable printing of the dayfile.  Default
**			on.
**		+s -- enable/disable printing of almost everything from
**			the monitor.
**		+U -- enable/disable direct update of system catalogs.
**			Default off.  The 04 bit is needed to set this
**			option.
**
**	Files:
**		.../files/usage -- to print a "usage: ..." message.
**		.../data/base/<database>/admin -- to determine
**			existance and some info about <database>.
**		.../files/dayfile<VERSION> -- dayfile (printed by
**			monitor).
**		.../files/users -- file with UNIX uid -> INGRES code
**			mapping, plus a pile of other information about
**			the user.
**		.../files/proctab<VERSION> -- default process table
**
**	History:
**		2/26/79 (marc) -- -* flag added.
**		8/9/78 (eric) -- Changed so that a variable number of
**			parameters may be sent to each process.  These
**			are indicated in the process table by a list
**			of any number of parameters colon-separated
**			at the end of the line.  Notice that even if
**			there are no parameters, the first colon must
**			be stated (actually, it terminates the previous
**			argument, rather than beginning the parameter
**			argument).
**		7/24/78 (eric) -- File descriptor processing on -&,
**			-@, and -| flags changed to stop on first
**			descriptor which is not an upper case letter,
**			used (maybe someday) by -| flag.  Also, fd's
**			changed to be stored with the 0100 bit set
**			internally, so that fd 0 will work.
**		7/21/78 (bob) -- '-|' flag changed back.
**		7/19/78 (eric) -- '-|' changed to not be processed
**			here.
**		7/18/78 (eric) -- code added to close 0 & 2.
**		7/5/78 (eric) -- '-|' network flag added.
**		3/27/78 (eric) -- changed to pass EQUEL flag, add
**			the -@ flag, and drop the Equel variable.
**			Also, the Fdesc vector was added.
**		1/29/78 -- changed to give more reasonable error
**			messages, and to include the .../files/usage
**			file.  Also, the "directory" field in the
**			process table has been changed to a "status"
**			field, bits being assigned to exec the process
**			as the real user (01) and to run in the default
**			directory (rather than the database) (02).
**			[by eric]
**		1/18/78 -- changed to exec processes as user INGRES,
**			so that someone else cannot write a dummy
**			driver which bypasses the protection system.
**			Processes must henceforth be mode 4700. [eric]
*/

# define	PTSIZE		2048		/* maximum size of the process table */
# define	PTPARAM		'$'		/* parameter expansion indicator */
# define	PTDELIM		"$"		/* separator string in proctab */
# define	MAXPARAMS	10		/* max number of params in proctab */
# define	MAXOPTNS	10		/* maximum number of options you can specify */
# define	MAXPROCS	10		/* maximum number of processes in the system */
# define	PVECTSIZE	6		/* number of pipes to each process */
# define	EQUELFLAG	'&'
# define	NETFLAG		'|'		/* network slave flag */
# define	CLOSED		'?'

char		Fileset[10];
char		*Database;
extern char	*Dbpath;		/* defined in initucode */
struct admin	Admin;			/* set in initucode */
char		Fdesc[10] = {'?','?','?','?','?','?','?','?','?','?'};
struct lockreq	Lock;
char		Ptbuf[PTSIZE + 1];
char		*Ptptr = Ptbuf;		/* ptr to freespace in Ptbuf */
char		*Opt[MAXOPTNS + 1];
int		Nopts;
int		No_exec;		/* if set, don't execute */
char		*User_ovrd;		/* override usercode from -u flag */

main(argc, argv)
int	argc;
char	**argv;
{
	register int		i;
	register int		j;
	extern char		*Proc_name;
	int			fd;
	extern int		Status;
	char			*proctab;
	register char		*p;
	char			*ptr;
	extern char		*Flagvect[];	/* defined in initucode.c */
	extern char		*Parmvect[];	/* ditto */
	char			*uservect[4];
	extern char		Version[];	/* version number */

	Proc_name = "INGRES";
	itoa(getpid(), Fileset);
	proctab = NULL;
	Database = NULL;

	/*
	**  Initialize everything, like Flagvect, Parmvect, Usercode,
	**	etc.
	*/

	i = initucode(argc, argv, TRUE, uservect, -1);
	switch (i)
	{
	  case 0:	/* ok */
	  case 5:
		break;

	  case 1:	/* database does not exist */
	  case 6:
		printf("Database %s does not exist\n", Parmvect[0]);
		goto usage;

	  case 2:	/* you are not authorized */
		printf("You may not access database %s\n", Database);
		goto usage;

	  case 3:	/* not a valid user */
		printf("You are not a valid INGRES user\n");
		goto usage;

	  case 4:	/* no database name specified */
		printf("No database name specified\n");
		goto usage;

	  default:
		syserr("initucode %d", i);
	}

	/*
	**  Extract database name and process table name from
	**	parameter vector.
	*/

	Database = Parmvect[0];
	proctab = Parmvect[1];
	if (checkdbname(Database))
	{
		printf("Improper database name: %s\n", Database);
		goto usage;
	}

	/* scan flags in users file */
	for (p = uservect[0]; *p != '\0'; p++)
	{
		/* skip initial blanks and tabs */
		if (*p == ' ' || *p == '\t')
			continue;
		ptr = p;

		/* find end of flag and null-terminate it */
		while (*p != '\0' && *p != ' ' && *p != '\t')
			p++;
		i = *p;
		*p = '\0';

		/* process the flag */
		doflag(ptr, 1);
		if (i == '\0')
			break;
	}

	/* scan flags on command line */
	for (i = 0; (p = Flagvect[i]) != NULL; i++)
		doflag(p, 0);

	/* check for query modification specified for this database */
	if ((Admin.adhdr.adflags & A_QRYMOD) == 0)
		doflag("-q", -1);

	/* do the -u flag processing */
	if (User_ovrd != 0)
		bmove(User_ovrd, Usercode, 2);

	/* close any extraneous files, being careful not to close anything we need */
	for (i = 'C'; i < 0100 + MAXFILES; i++)
	{
		for (j = 0; j < (sizeof Fdesc / sizeof *Fdesc); j++)
		{
			if (Fdesc[j] == i)
				break;
		}
		if (j >= (sizeof Fdesc / sizeof *Fdesc))
			close(i & 077);
	}

	/* determine process table */
	if (proctab == NULL)
	{
		/* use default proctab */
		proctab = uservect[1];
		if (proctab[0] == 0)
		{
			/* no proctab in users file */
			concat("=proctab", Version, Ptbuf);

			/* strip off the mod number */
			for (p = Ptbuf; *p != 0; p++)
				if (*p == '/')
					break;
			*p = 0;
			proctab = Ptbuf;
		}
	}
	else
	{
		/* proctab specified; check permissions */
		if ((Status & (proctab[0] == '=' ? U_EPROCTAB : U_APROCTAB)) == 0)
		{
			printf("You may not specify this process table\n");
			goto usage;
		}
	}

	/* expand process table name */
	if (proctab[0] == '=')
	{
		smove(ztack(ztack(Pathname, "/files/"), &proctab[1]), Ptbuf);
		proctab = Ptbuf;
	}

	/* open and read the process table */
	if ((fd = open(proctab, 0)) < 0)
	{
		printf("Proctab %s: %s\n", proctab, strerror(errno));
		goto usage;
	}

	if ((i = read(fd, Ptbuf, PTSIZE + 1)) < 0)
		syserr("Cannot read proctab %s", proctab);
	if (i > PTSIZE)
		syserr("Proctab %s too big, lim %d", proctab, PTSIZE);

	close(fd);
	Ptbuf[i] = 0;

	/* build internal form of the process table */
	buildint();

	/* don't bother executing if we have found errors */
	if (No_exec)
	{
	  usage:
		/* cat .../files/usage */
		cat(ztack(Pathname, "/files/usage"));
		exit(1);
	}

	/* set locks on the database */
	dolocks();

	/* satisfy process table (never returns) */
	satisfypt();
}



/*
**  Process rubouts (just exit)
*/

rubproc()
{
	exit(2);
}
/*
**  DOFLAG -- process flag
**
**	Parameters:
**		flag -- the flag (as a string)
**		where -- where it is called from
**			-1 -- internally inserted
**			0 -- on user command line
**			1 -- from users file
**
**	Return:
**		none
**
**	Side effects:
**		All flags are inserted on the end of the
**		"Flaglist" vector for passing to the processes.
**		The "No_exec" flag is set if the flag is bad or you
**		are not authorized to use it.
**
**	Requires:
**		Status -- to get the status bits set for this user.
**		syserr -- for the obvious
**		printf -- to print errors
**		atoi -- to check syntax on numerically-valued flags
**
**	Defines:
**		doflag()
**		Flagok -- a list of legal flags and attributes (for
**			local use only).
**		Relmode -- a list of legal relation modes.
**
**	Called by:
**		main
**
**	History:
**		11/6/79 (6.2/8) (eric) -- -u flag processing dropped,
**			since initucode does it anyhow.  -E flag
**			removed (what is it?).  F_USER code dropped.
**			F_DROP is still around; we may need it some-
**			day.  Also, test of U_SUPER flag and/or DBA
**			status was wrong.
**		7/5/78 (eric) -- NETFLAG added to list.
**		3/27/78 (eric) -- EQUELFLAG added to the list.
**		1/29/78 -- do_u_flag broken off by eric
**		1/4/78 -- written by eric
*/

struct flag
{
	char	flagname;	/* name of the flag */
	char	flagstat;	/* status of flag (see below) */
	int	flagsyntx;	/* syntax code for this flag */
	int	flagperm;	/* status bits needed to use this flag */
};

/* status bits for flag */
# define	F_PLOK		01	/* allow +x form */
# define	F_PLD		02	/* defaults to +x */
# define	F_DBA		04	/* must be the DBA to use */
# define	F_DROP		010	/* don't save in Flaglist */

/* syntax codes */
# define	F_ACCPT		1	/* always accept */
# define	F_C_SPEC	3	/* -cN spec */
# define	F_I_SPEC	4	/* -inN spec */
# define	F_F_SPEC	5	/* -fnxN.M spec */
# define	F_CHAR		6	/* single character */
# define	F_MODE		7	/* a modify mode */
# define	F_INTERNAL	8	/* internal flag, e.g., -q */
# define	F_EQUEL		9	/* EQUEL flag */

struct flag	Flagok[] =
{
	'a',		F_PLD|F_PLOK,	F_ACCPT,	0,
	'b',		F_PLD|F_PLOK,	F_ACCPT,	U_DRCTUPDT,
	'c',		0,		F_C_SPEC,	0,
	'd',		F_PLD|F_PLOK,	F_ACCPT,	0,
	'f',		0,		F_F_SPEC,	0,
	'i',		0,		F_I_SPEC,	0,
	'n',		0,		F_MODE,		0,
	'q',		F_PLD|F_PLOK,	F_INTERNAL,	0,
	'r',		0,		F_MODE,		0,
	's',		F_PLD|F_PLOK,	F_ACCPT,	0,
	'v',		0,		F_CHAR,		0,
	'w',		F_PLOK|F_DROP,	F_ACCPT,	0,
	'D',		0,		F_ACCPT,	U_TRACE,
/*	'E',		F_PLOK|F_DBA,	F_ACCPT,	0,	*/
	'L',		F_PLOK,		F_ACCPT,	0,
	'M',		0,		F_ACCPT,	U_TRACE,
	'O',		0,		F_ACCPT,	U_TRACE,
	'P',		0,		F_ACCPT,	U_TRACE,
	'Q',		0,		F_ACCPT,	U_TRACE,
	'U',		F_PLOK,		F_ACCPT,	U_UPSYSCAT,
	'Z',		0,		F_ACCPT,	U_TRACE,
	EQUELFLAG,	0,		F_EQUEL,	0,
	NETFLAG,	0,		F_EQUEL,	0,
	'@',		0,		F_EQUEL,	0,
	'*',		0,		F_ACCPT,	0,
	0,		0,		0,		0
};

/* list of valid retrieve into or index modes */
char	*Relmode[] =
{
	"isam",
	"cisam",
	"hash",
	"chash",
	"heap",
	"cheap",
	"heapsort",
	"cheapsort",
	NULL
};


doflag(flag, where)
char	*flag;
int	where;
{
	register char		*p;
	register struct flag	*f;
	auto int		intxx;
	register char		*ptr;
	int			i;
	extern int		Status;

	p = flag;

	/* check for valid flag format (begin with + or -) */
	if (p[0] != '+' && p[0] != '-')
		goto badflag;

	/* check for flag in table */
	for (f = Flagok; f->flagname != p[1]; f++)
	{
		if (f->flagname == 0)
			goto badflag;
	}

	/* check for +x form allowed */
	if (p[0] == '+' && (f->flagstat & F_PLOK) == 0)
		goto badflag;

	/* check for permission to use the flag */
	if ((f->flagperm != 0 && (Status & f->flagperm) == 0 &&
	     (((f->flagstat & F_PLD) == 0) ? (p[0] == '+') : (p[0] == '-'))) ||
	    ((f->flagstat & F_DBA) != 0 && (Status & U_SUPER) == 0 &&
	     !bequal(Usercode, Admin.adhdr.adowner, 2)))
	{
		printf("You are not authorized to use the %s flag\n", p);
		No_exec++;
	}

	/* check syntax */
	switch (f->flagsyntx)
	{
	  case F_ACCPT:
		break;

	  case F_C_SPEC:
		if (atoi(&p[2], &intxx) || intxx > MAXFIELD)
			goto badflag;
		break;

	  case F_I_SPEC:
		if (p[2] != '1' && p[2] != '2' && p[2] != '4')
			goto badflag;
		if (atoi(&p[3], &intxx) || intxx > MAXFIELD)
			goto badflag;
		break;

	  case F_F_SPEC:
		if (p[2] != '4' && p[2] != '8')
			goto badflag;
		switch (p[3])
		{
		  case 'e':
		  case 'E':
		  case 'f':
		  case 'F':
		  case 'g':
		  case 'G':
		  case 'n':
		  case 'N':
			break;

		  default:
			goto badflag;

		}
		ptr = &p[4];
		while (*ptr != '.')
			if (*ptr == 0)
				goto badflag;
			else
				ptr++;
		*ptr = 0;
		if (atoi(&p[4], &intxx) || intxx > MAXFIELD)
			goto badflag;
		*ptr++ = '.';
		if (atoi(ptr, &intxx) || intxx > MAXFIELD)
			goto badflag;
		break;

	  case F_CHAR:
		if (p[2] == 0 || p[3] != 0)
			goto badflag;
		break;

	  case F_MODE:
		for (i = 0; (ptr = Relmode[i]) != NULL; i++)
		{
			if (sequal(&p[2], ptr))
				break;
		}
		if (ptr == NULL)
			goto badflag;
		break;

	  case F_INTERNAL:
		if (where >= 0)
			goto badflag;
		break;

	  case F_EQUEL:
		ptr = &p[2];
		for (i = 0; i < (sizeof Fdesc / sizeof *Fdesc); i++, ptr++)
		{
			if (*ptr == CLOSED)
				continue;
			if (*ptr == '\0' || *ptr < 0100 || *ptr >= 0100 + MAXFILES)
				break;
			Fdesc[i] = (*ptr & 077) | 0100;
		}
		break;

	  default:
		syserr("doflag: syntx %d", f->flagsyntx);

	}

	/* save flag */
	if (Nopts >= MAXOPTNS)
	{
		printf("Too many options to INGRES\n");
		exit(1);
	}
	if ((f->flagstat & F_DROP) == 0)
		Opt[Nopts++] = p;
	return;

  badflag:
	printf("Bad flag format: %s\n", p);
	No_exec++;
	return;
}
/*
**  DOLOCKS -- set database lock
**
**	A lock is set on the database.
*/

dolocks()
{
	db_lock(flagval('E') > 0 ? M_EXCL : M_SHARE);
	close(Alockdes);
}
/*
**  BUILDINT -- build internal form of process table
**
**	The text of the process table is scanned and converted to
**	internal form.  Non-applicable entries are deleted in this
**	pass, and the EQUEL pipes are turned into real file descrip-
**	tors.
**
**	Parameters:
**		none
**
**	Returns:
**		nothing
**
**	Requires:
**		scanpt -- to return next field of the process table
**		flagval -- to return the value of a given flag
**		Ptbuf -- the text of the process table
**		Ptptr -- pointer to freespace in Ptbuf
**		getptline -- to return next line of process table
**
**	Defines:
**		buildint
**		Proctab -- the internal form of the process table
**		Params -- the parameter symbol table
**
**	Called by:
**		main
**
**	History:
**		8/9/78 (eric) -- moved parameter expansion to
**			ingexec.  Also, changed the pt line selection
**			algorithm slightly to fix possible degenerate
**			bug -- flags could not be '+', '-', '<', '>',
**			or '=' without problems.
**		3/27/78 (eric) -- changed pt line selection criteria
**			to be more explicit in default cases; needed
**			to do networking with the +& flag.
**		1/4/78 -- written by eric
*/

struct proc
{
	char	*prpath;
	char	*prcond;
	int	prstat;
	char	*prstdout;		/* file for standard output this proc */
	char	prpipes[PVECTSIZE + 1];
	char	*prparam;
};

/* bits for prstat */
# define	PR_REALUID	01	/* run the process as the real user */
# define	PR_NOCHDIR	02	/* run in the user's directory */
# define	PR_CLSSIN	04	/* close standard input */
# define	PR_CLSDOUT	010	/* close diagnostic output */

struct proc	Proctab[MAXPROCS];

struct param
{
	char	*pname;
	char	*pvalue;
};

struct param	Params[MAXPARAMS];


buildint()
{
	register struct proc	*pr;
	char			*ptp;
	register char		*p;
	register char		*pipes;
	int			i;
	int			j;
	struct param		*pp;
	char			*getptline();

	/* scan the template */
	pr = Proctab;
	while ((ptp = getptline()) != 0)
	{
		/* check for overflow of internal Proctab */
		if (pr >= &Proctab[MAXPROCS])
			syserr("Too many processes");

		/* collect a line into the fixed-format proctab */
		pr->prpath = ptp;
		scanpt(&ptp, ':');
		pr->prcond = ptp;
		scanpt(&ptp, ':');
		pipes = ptp;
		scanpt(&ptp, ':');
		pr->prstat = oatoi(pipes);
		pr->prstdout = ptp;
		scanpt(&ptp, ':');
		pipes = ptp;
		scanpt(&ptp, ':');
		if (length(pipes) != PVECTSIZE)
			syserr("buildint: pipes(%s, %s)", pr->prpath, pipes);
		smove(pipes, pr->prpipes);
		for (p = pr->prpipes; p < &pr->prpipes[PVECTSIZE]; p++)
			if (*p == 0)
				*p = CLOSED;
		pr->prparam = ptp;

		/* check to see if this entry is applicable */
		for (p = pr->prcond; *p != 0; )
		{
			/* determine type of flag */
			switch (*p++)
			{
			  case '=':	/* flag must not be set */
				j = 1;
				break;

			  case '+':	/* flag must be set on */
			  case '>':
				j = 2;
				if (*p == '=')
				{
					j++;
					p++;
				}
				break;

			  case '-':	/* flag must be set off */
			  case '<':
				j = 4;
				if (*p == '=')
				{
					j++;
					p++;
				}
				break;
			  
			  default:
				/* skip any initial garbage */
				continue;
			}

			/* get value of this flag */
			i = flagval(*p++);

			/* check to see if we should keep looking */
			switch (j)
			{
			  case 1:
				if (i == 0)
					continue;
				break;

			  case 2:
				if (i > 0)
					continue;
				break;

			  case 3:
				if (i >= 0)
					continue;
				break;

			  case 4:
				if (i < 0)
					continue;
				break;

			  case 5:
				if (i <= 0)
					continue;
				break;
			}

			/* mark this entry as a loser and quit */
			p = NULL;
			break;
		}

		if (p == NULL)
		{
			/* this entry does not apply; throw it out */
			continue;
		}

		/* replace EQUEL codes with actual file descriptors */
		for (p = pr->prpipes; *p != 0; p++)
		{
			if (*p < '0' || *p > '9')
				continue;

			*p = Fdesc[*p - '0'];
		}

		/* this entry is valid, move up to next entry */
		pr++;
	}
	pr->prpath = 0;

	/* scan for parameters */
	pp = Params;
	pp->pname = "pathname";
	pp++->pvalue = Pathname;
	while ((p = getptline()) != 0)
	{
		/* check for parameter list overflow */
		if (pp >= &Params[MAXPARAMS])
			syserr("Too many parameters");

		pp->pname = p;

		/* get value for parameter */
		if ((pp->pvalue = p = getptline()) == 0)
		{
			/* if no lines, make it null string */
			pp->pvalue = "";
		}
		else
		{
			/* get 2nd thru nth and concatenate them */
			do
			{
				ptp = &p[length(p)];
				p = getptline();
				*ptp = '\n';
			} while (p != 0);
		}

		/* bump to next entry */
		pp++;
	}

	/* adjust free space pointer for eventual call to expand() */
	Ptptr++;
}
/*
**  FLAGVAL -- return value of flag
**
**	Parameter:
**		flag -- the name of the flag
**
**	Return:
**		-1 -- flag is de-asserted (-x)
**		0 -- flag is not specified
**		1 -- flag is asserted (+x)
**
**	Requires:
**		Opt -- to scan the flags
**
**	Defines:
**		flagval
**
**	Called by:
**		buildint
**		dolocks
**
**	History:
**		3/27/78 (eric) -- changed to handle EQUEL flag
**			normally.
**		1/4/78 -- written by eric
*/

flagval(flag)
char	flag;
{
	register char	f;
	register char	**p;
	register char	*o;

	f = flag;

	/* start scanning option list */
	for (p = Opt; (o = *p) != 0; p++)
	{
		if (o[1] == f)
			if (o[0] == '+')
				return (1);
			else
				return (-1);
	}
	return (0);
}
/*
**  SCANPT -- scan through process table for a set of delimiters
**
**	Parameters:
**		pp -- a pointer to a pointer to the process table
**		delim -- a primary delimiter.
**
**	Returns:
**		The actual delimiter found
**
**	Side effects:
**		Updates pp to point the the delimiter.  The delimiter
**		is replaced with null.
**
**	Requires:
**		nothing
**
**	Defines:
**		scanpt
**
**	Called by:
**		buildint
**
**	History:
**		1/4/78 -- written by eric
*/

scanpt(pp, delim)
char	**pp;
char	delim;
{
	register char	*p;
	register char	c;
	register char	d;

	d = delim;

	for (p = *pp; ; p++)
	{
		c = *p;

		/* check to see if it is a delimiter */
		if (c == d)
		{
			/* we have a delim match */
			*p++ = 0;
			*pp = p;
			return (c);
		}
		if (c == 0)
		{
			syserr("proctab: d=%c, *pp=%s", d, *pp);
		}
	}
}
/*
**  GETPTLINE -- get line from process table
**
**	Parameters:
**		none
**
**	Return:
**		zero -- end of process table or section
**		else -- pointer to a line, which is a null-terminated
**			string (without the newline).
**
**	Side effects:
**		sticks a null byte into Ptbuf at the end of the line.
**
**	Note:
**		sequential lines returned will be sequential in core,
**			that is, they can be concatenated by just
**			changing the null-terminator back to a newline.
**
**	Requires:
**		Ptptr -- should point to the next line of the process
**			table.
**		sequal -- to do a string equality test for "$"
**
**	Defines:
**		getptline
**
**	Called by:
**		buildint
**
**	History:
**		1/8/77 -- written by eric
*/

char *getptline()
{
	register char	c;
	register char	*line;

	/* mark the beginning of the line */
	line = Ptptr;

	/* scan for a newline or a null */
	while ((c = *Ptptr++) != '\n')
	{
		if (c == 0)
		{
			/* arrange to return zero next time too */
			Ptptr--;
			return ((char *) 0);
		}
	}

	/* replace the newline with null */
	Ptptr[-1] = 0;

	/* see if it is an "end-of-section" mark */
	if (sequal(line, PTDELIM))
	{
		line[0] = 0;
		return (0);
	}
	return (line);
}
/*
**  EXPAND -- macro expand a string
**
**	A string is expanded: $name constructs are looked up and
**	expanded if found.  If not, the old construct is passed
**	thru.  The return is the address of a new string that will
**	do what you want.  Calls may be recursive.  The string is
**	not copied unless necessary.
**
**	Parameters:
**		s1 -- the string to expand
**		intflag -- a flag set if recursing
**
**	Return:
**		the address of the expanded string, not copied unless
**		necessary.
**
**	Side effects:
**		Ptbuf is allocated (from Ptptr) for copy space.  There
**			must be enough space to copy, even if the copy
**			is not saved.
**		Ptptr is updated to point to the new free space.
**
**	Requires:
**		Ptbuf, Ptptr -- to get buffer space
**		Params -- to scan parameter
**		sequal -- to check for parameter name match
**		syserr
**
**	Defines:
**		expand
**
**	Syserrs:
**		"Proctab too big to macro expand"
**			ran out of space making a copy of the parame-
**			ter.
**
**	History:
**		1/8/78 -- written by eric
*/

char *expand(s1, intflag)
char	*s1;
int	intflag;
{
	register struct param	*pp;
	register char		*s;
	register char		c;
	int			flag;
	int			count;
	char			*mark, *xmark;

	s = s1;
	xmark = Ptptr;
	flag = 0;		/* set if made a macro expansion */
	count = 0;		/* set if any characters copied directly */

	/* scan input string to end */
	while ((c = *s++) != 0)
	{
		/* check for possible parameter expansion */
		if (c != PTPARAM)
		{
			/* nope, copy through if space */
			if (Ptptr >= &Ptbuf[PTSIZE])
				goto ptsizeerr;
			*Ptptr++ = c;
			count++;
			continue;
		}

		/* possible expansion; mark this point */
		mark = Ptptr;

		/* collect the name of the parameter */
		do
		{
			if (Ptptr >= &Ptbuf[PTSIZE])
			{
			  ptsizeerr:
				syserr("Proctab too big to macro expand");
			}
			*Ptptr++ = c;
			c = *s++;
		} while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
		s--;

		/* look up in parameter table */
		*Ptptr = 0;
		for (pp = Params; pp->pname != 0; pp++)
		{
			if (sequal(pp->pname, mark + 1))
				break;
		}

		if (pp->pname == 0)
		{
			/* search failed, take string as is */
			count++;
			continue;
		}

		/* reprocess name delimiter unless a newline */
		if (c == '\n')
			s++;

		/* found entry, copy it in instead */
		/* back up over name */
		Ptptr = mark;

		/* check to see if we can return expansion as value */
		if (count == 0 && *s == 0)
		{
			/* then nothing before or after this call */
			if (intflag == 0)
				Ptptr = xmark;
			return (pp->pvalue = expand(pp->pvalue, 1));
		}

		/* expand to new buffer */
		pp->pvalue = expand(pp->pvalue, 1);

		/* we are appending */
		Ptptr--;
		flag++;
	}

	/* null terminate the new entry (which must be copied) */
	*Ptptr++ = 0;

	/* see if we expanded anything */
	if (flag)
	{
		/* yes, return the new stuff in buffer */
		return (xmark);
	}

	/* no, then we can return the original and reclaim space */
	if (intflag == 0)
		Ptptr = xmark;
	return (s1);
}
/*
**  SATISFYPT -- satisfy the process table
**
**	Well folks, now that you've read this far, this is it!!!  I
**	mean, this one really does it!!!  It takes the internal form
**	built by buildint() and creates pipes as necessary, forks, and
**	execs the INGRES processes.  Isn't that neat?
**
**		* * * * *   W A R N I N G   * * * * *
**
**	In some versions, the pipe() system call support function
**	in libc.a clobbers r2.  In versions 6 and 7 PDP-11 C compilers,
**	this is the third register variable declared (in this case,
**	"i").  This isn't a problem here, but take care if you change
**	this routine.
**
**	Parameters:
**		none
**
**	Returns:
**		never
**
**	Requires:
**		Proctab -- the internal form
**		ingexec -- to actually exec the process
**		pipe -- to create the pipe
**		syserr -- for the obvious
**		fillpipe -- to extend a newly opened pipe through all
**			further references to it.
**		checkpipes -- to see if a given pipe will ever be
**			referenced again.
**		fork -- to create a new process
**
**	Defines:
**		satisfypt
**
**	Called by:
**		main
**
**	History:
**		7/24/78 (eric) -- Actual file descriptors stored in
**			'prpipes' are changed to have the 0100 bit
**			set internally (as well as externally), so
**			fd 0 will work correctly.
**		1/4/78 -- written by eric
*/

satisfypt()
{
	register struct proc	*pr;
	int			pipex[2];
	register char		*p;
	register int		i;

	/* scan the process table */
	for (pr = Proctab; pr->prpath != 0; pr++)
	{
		/* scan pipe vector, creating new pipes as needed */
		for (p = pr->prpipes; *p != 0; p++)
		{
			if ((*p >= 0100 && *p < 0100 + MAXFILES) || *p == CLOSED)
			{
				/* then already open or never open */
				continue;
			}

			/* not yet open: create a pipe */
			if (pipe(pipex))
				syserr("satisfypt: pipe %c", *p);

			/* propagate pipe through rest of proctab */
			fillpipe(*p, pr, pipex);
		}

		/* fork if necessary */
		if (pr[1].prpath == 0 || (i = fork()) == 0)
		{
			/* child!! */
			ingexec(pr);
		}

		/* parent */
		if (i < 0)
			syserr("satisfypt: fork");

		/* scan pipes.  close all not used in the future */
		for (p = pr->prpipes; *p != 0; p++)
		{
			if (*p == CLOSED)
				continue;
			if (checkpipes(&pr[1], *p))
				if (close(*p & 077))
					syserr("satisfypt: close(%s, %d)", pr->prpath, *p & 077);
		}
	}
	syserr("satisfypt: fell out");
}
/*
**  FILLPIPE -- extend pipe through rest of process table
**
**	The only tricky thing in here is that "rw" alternates between
**	zero and one as a pipe vector is scanned, so that it will know
**	whether to get the read or the write end of a pipe.
**
**	Parameters:
**		name -- external name of the pipe
**		proc -- the point in the process table to scan from
**		pipex -- the pipe
**
**	Returns:
**		nothing
**
**	Requires:
**		nothing
**
**	Defines:
**		fillpipe
**
**	Called by:
**		satisfypt
**
**	History:
**		7/24/78 (eric) -- 0100 bit set on file descriptors.
**		1/4/78 -- written by eric
*/

fillpipe(name, proc, pipex)
char		name;
struct proc	*proc;
int		pipex[2];
{
	register struct proc	*pr;
	register char		*p;
	register int		rw;

	/* scan rest of processes */
	for (pr = proc; pr->prpath != 0; pr++)
	{
		/* scan the pipe vector */
		rw = 1;
		for (p = pr->prpipes; *p != 0; p++)
		{
			rw = 1 - rw;
			if (*p == name)
				*p = pipex[rw] | 0100;
		}
	}
}
/*
**  CHECKPIPES -- check for pipe referenced in the future
**
**	Parameters:
**		proc -- point in the process table to start looking
**			from.
**		fd -- the file descriptor to look for.
**
**	Return:
**		zero -- it will be referenced in the future.
**		one -- it is never again referenced.
**
**	Requires:
**		nothing
**
**	Defines:
**		checkpipes
**
**	Called by:
**		satisfypt
**
**	History:
**		7/24/78 (eric) -- 0100 bit on file descriptors handled.
**		1/4/78 -- written by eric
*/

checkpipes(proc, fd)
struct proc	*proc;
int		fd;
{
	register struct proc	*pr;
	register char		*p;
	register int		fdx;

	fdx = fd | 0100;

	for (pr = proc; pr->prpath != 0; pr++)
	{
		for (p = pr->prpipes; *p != 0; p++)
			if (*p == fdx)
				return (0);
	}
	return (1);
}
/*
**  INGEXEC -- execute INGRES process
**
**	This routine handles all the setup of the argument vector
**	and then executes a process.
**
**	Parameters:
**		process -- a pointer to the process table entry which
**			describes this process.
**
**	Returns:
**		never
**
**	Side Effects:
**		never returns, but starts up a new overlay.  Notice
**			that it does NOT fork.
**
**	Requires:
**		none
**
**	Called By:
**		satisfypt
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		chdir %s -- could not change directory into the data-
**			base.
**		creat %s -- could not create the redirected standard
**			output file.
**		%s not executable -- could not execute the process.
**
**	History:
**		8/9/78 (eric) -- changed "prparam" to be a colon-
**			separated list of parameters (so the number
**			is variable); also, moved parameter expansion
**			into this routine from buildint() so that
**			the colons in the dbu part of the proctab
**			would not confuse things.
**		7/24/78 (eric) -- changed the technique of closing
**			files 0 & 2 so that they will never be closed
**			(even if requested in the status field)
**			if they are mentioned in the pipe vector.
**			Also, some fiddling is done to handle the
**			0100 bit on file descriptors correctly.
*/

ingexec(process)
struct proc	*process;
{
	char			*vect[30];
	register char		*p;
	register char		**v;
	char			**opt;
	int			i;
	register struct proc	*pr;
	int			outfd;

	v = vect;
	pr = process;

	*v++ = expand(pr->prpath);
	*v++ = pr->prpipes;
	*v++ = Fileset;
	*v++ = Usercode;
	*v++ = Database;
	*v++ = Pathname;

	/* expand variable number of parameters */
	for (p = pr->prparam; *p != 0; )
	{
		for (*v = p; *p != 0; p++)
		{
			if (*p == ':')
			{
				*p++ = 0;
				break;
			}
		}

		/* expand macros in parameters */
		*v++ = expand(*v);
	}

	/* insert flag parameters */
	for (opt = Opt; *opt; opt++)
		*v++ = *opt;
	*v = 0;

	/* close extra pipes (those not in pipe vector) */
	for (i = 0100; i < 0100 + MAXFILES; i++)
	{
		for (p = pr->prpipes; *p != 0; p++)
		{
			if (i == *p)
				break;
		}
		if (*p == 0 && i != 0100 + 1 &&
		    (i != 0100 || (pr->prstat & PR_CLSSIN) != 0) &&
		    (i != 0100 + 2 || (pr->prstat & PR_CLSDOUT) != 0))
			close(i & 077);
	}

	/* change to the correct directory */
	if ((pr->prstat & PR_NOCHDIR) == 0)
	{
		if (chdir(Dbpath))
			syserr("ingexec: chdir %s", Dbpath);
	}

	/* change to normal userid/groupid if a non-dangerous process */
	if ((pr->prstat & PR_REALUID) != 0)
	{
		setuid(getuid());
#		ifndef xB_UNIX
		setgid(getgid());
#		endif
	}

	/* change standard output if specified in proctab */
	p = pr->prstdout;
	if (*p != 0)
	{
		/* chew up fd 0 (just in case) */
		outfd = dup(1);
		close(1);
		if (creat(p, 0666) != 1)
		{
			/* restore standard output and print error */
			close(1);
			dup(outfd);	/* better go into slot 1 */
			syserr("ingexec: creat %s", p);
		}
		close(outfd);
	}

	/* give it the old college (or in this case, University) try */
	execv(vect[0], vect);
	syserr("\"%s\" not executable", vect[0]);
}
/*
**  CHECKDBNAME -- check for valid database name
**
**	Parameter:
**		name -- the database name
**
**	Return:
**		zero -- ok
**		else -- bad database name
**
**	Side effects:
**		none
**
**	Requires:
**		nothing
**
**	Defines:
**		checkdbname
**
**	History:
**		1/9/78 -- written by eric
*/

checkdbname(name)
char	*name;
{
	register char	*n;
	register char	c;

	n = name;

	if (length(n) > 14)
		return (1);
	while ((c = *n++) != 0)
	{
		if (c == '/')
			return (1);
	}
	return (0);
}

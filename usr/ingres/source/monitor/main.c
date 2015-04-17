# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"../unix.h"
# include	"../pipes.h"
# include	"monitor.h"

# define	ERRDELIM	'~'

/*
**  INTERACTIVE TERMINAL MONITOR
**
**	The monitor gathers text from the standard input and performs
**	a variety of rudimentary editting functions.  This program
**	is the main setup.  Monitor() is then called, which does the
**	real work.
**
**	variables:
**	Nodayfile -- zero prints all messages; positive one suppresses
**		dayfile and logout but not prompts; negative one
**		suppresses all printed material except results from \p.
**	Newline -- set when the last character in the query buffer
**		is a newline.
**	Prompt -- set when a prompt character is needed.
**	Autoclear -- set when the query buffer should be cleared before
**		putting another character in.
**	Nautoclear -- if set, suppresses the autoclear function
**		entirely.
**
**	flags:
**	-M -- trace flag
**	-d -- suppress dayfile
**	-s -- suppress prompt (sets -d)
**	-a -- disable autoclear function
**
**	The last three options can be set by stating "+x".
**
**	Uses trace flag 9
**	Proc_error uses trace flag 11
**
**	History:
**		8/15/79 (eric) (6.2/7) -- added {database} and
**			{usercode} macros.
**		3/2/79 (eric) -- rubproc changed to know about the
**			Trapfile stuff.
*/

extern char	*Usercode;
extern char	*Database;

main(argc, argv)
int	argc;
char	*argv[];
{
	register int		ndx;
	extern char		*Proc_name;
	register char		*p;
	extern			quit();
	extern int		(*Exitfn)();
	extern int		Equel;
	char			buff[100];
	extern char		**Xparams;
	char			*getufield();

	/* insure that permissions are ok */
	setuid(getuid());
#	ifndef xB_UNIX
	setgid(getgid());
#	endif

	setexit();
	signal(13, &quit);

#	ifdef xMTR1
	tTrace(&argc, argv, 'M');
#	endif
	initproc("MONITOR", argv);
	Exitfn = &quit;

	/* process arguments */
	if (!setflag(argv, 'd', 1))
		Nodayfile = 1;
	if (!setflag(argv, 's', 1))
		Nodayfile = -1;
	Nautoclear = !setflag(argv, 'a', 1);
#	ifdef xMTR3
	if (tTf(9, 0))
	{
		close(ndx = dup(0));
		printf("0 dups as %d\n", ndx);
	}
#	endif
#	ifdef xMTR2
	if (tTf(9, 1))
		prargs(argc, argv);
	if (tTf(9, 2))
	{
		printf("R_up %d W_up %d R_down %d W_down %d\n",
			R_up, W_up, R_down, W_down);
		printf("R_front %d W_front %d Equel %d\n",
			R_front, W_front, Equel);
	}
#	endif
	smove(Version, Versn);
	for (p = Versn; *p != '/'; p++)
		if (!*p)
			break;
	*p = '\0';

	/* preinitialize macros */
	macinit(0, 0, 0);
	macdefine("{pathname}", Pathname, 1);
	macdefine("{database}", Database, 1);
	macdefine("{usercode}", Usercode, 1);

	/* print the dayfile */
	if (Nodayfile >= 0)
	{
		time(buff);
		printf("INGRES version %s login\n%s", Version, ctime(buff));
	}
	if (Nodayfile == 0 && (Qryiop = fopen(ztack(ztack(Pathname, "/files/dayfile"), Versn), "r")) != NULL)
	{
		while ((ndx = getc(Qryiop)) > 0)
			putchar(ndx);
		fclose(Qryiop);
	}

	/* SET UP LOGICAL QUERY-BUFFER FILE */
	concat("/tmp/INGQ", Fileset, Qbname);
	if ((Qryiop = fopen(Qbname, "w")) == NULL)
		syserr("main: open(%s)", Qbname);

	/* GO TO IT ... */
	Prompt = Newline = TRUE;
	Userdflag = Nodayfile;
	Nodayfile = -1;

	/* run the system initialization file */
	setexit();
	Phase++;
	include(Xparams[0]);

	/* find out what the user initialization file is */
	setexit();
	if (getuser(Usercode, buff) == 0)
	{
		p = getufield(buff, 7);
		if (*p != 0)
			include(p);
	}
	getuser(0, 0);

	Nodayfile = Userdflag;

	/* get user input from terminal */
	Input = stdin;
	setbuf(stdin, NULL);
	setexit();
	xwait();
	monitor();
	quit();
}





/*
**  CATCH SIGNALS
**
**	clear out pipes and respond to user
**
**	Uses trace flag 10
*/

rubproc()
{
	register int	i;
	long		ltemp;

#	ifdef xMTR3
	if (tTf(10, 0))
		printf("caught sig\n");
#	endif
	if (Xwaitpid == 0)
		printf("\nInterrupt\n");
	resyncpipes();
	for (i = 3; i < MAXFILES; i++)
	{
		if (i == Qryiop->_file || i == R_down || i == W_down ||
		    (Trapfile != NULL && i == Trapfile->_file))
			continue;
		close(i);
	}
	ltemp = 0;
	lseek(stdin->_file, ltemp, 2);
	Newline = Prompt = TRUE;
	Nodayfile = Userdflag;
	Oneline = FALSE;
	Idepth = 0;
	setbuf(stdin, NULL);
	Input = stdin;
}


/*
**  PROCESS ERROR MESSAGE
**
**	This routine takes an error message off of the pipe and
**	processes it for output to the terminal.  This involves doing
**	a lookup in the .../files/error? files, where ? is the thous-
**	ands digit of the error number.  The associated error message
**	then goes through parameter substitution and is printed.
**
**	In the current version, the error message is just printed.
**
**	Uses trace flag 11
*/

proc_error(s1, rpipnum)
struct pipfrmt	*s1;
int		rpipnum;
{
	register struct pipfrmt	*s;
	register char		c;
	char			*pv[10];
	char			parm[512];
	int			pc;
	register char		*p;
	int			i;
	char			buf[sizeof parm + 30];
	int			err;
	FILE			*iop;
	char			*errfilen();
	char			*mcall();

	s = s1;
	err = s->err_id;
	Error_id = err;

	/* read in the parameters */
	pc = 0;
	p = parm;
	while (i = rdpipe(P_NORM, s, R_down, p, 0))
	{
		pv[pc] = p;
#		ifdef xMTR3
		if (tTf(11, 1))
			printf("pv[%d] = %s, i=%d\n", pc, p, i);
#		endif
		p += i;
		pc++;
		if (pc >= 10 || (p - parm) >= 500)
		{
			/* buffer overflow; throw him off */
			syserr("Your error buffer overfloweth @ pc %d err %d p-p %d",
				pc, err, p - parm);
		}
	}
	pv[pc] = 0;
	rdpipe(P_PRIME, s);

	/* try calling the {catcherror} macro -- maybe not print */
	p = buf;
	p += smove("{catcherror; ", p);
	p += smove(iocv(err), p);
	p += smove("}", p);

	p = mcall(buf);
	if (sequal(p, "0"))
		return (1);

	/* open the appropriate error file */
	p = errfilen(err / 1000);

#	ifdef xMTR3
	if (tTf(11, -1))
		printf("proc_error: ");
	if (tTf(11, 0))
		printf("%d, %s", err, p);
#	endif

	if ((iop = fopen(p, "r")) == NULL)
		syserr("proc_error: open(%s)", p);

	/* read in the code and check for correct */
	for (;;)
	{
		p = buf;
		while ((c = getc(iop)) != '\t')
		{
			if (c <= 0)
			{
				/* no code exists, print the args */
				printf("%d:", err);
				for (i = 0; i < pc; i++)
					printf(" `%s'", pv[i]);
				printf("\n");
				fclose(iop);
				return (1);
			}
			*p++ = c;
		}
		*p = 0;
		if (atoi(buf, &i))
			syserr("proc_error: bad error file %d\n%s",
				err, buf);
		if (i != err)
		{
			while ((c = getc(iop)) != ERRDELIM)
				if (c <= 0)
					syserr("proc_error: format err %d", err);
			getc(iop);	/* throw out the newline */
			continue;
		}

		/* got the correct line, print it doing parameter substitution */
		printf("%d: ", err);
		c = '\n';
		for (;;)
		{
			c = getc(iop);
			if (c <= 0 || c == ERRDELIM)
			{
				printf("\n");
				fclose(iop);
				return (1);
			}
			if (c == '%')
			{
				c = getc(iop);
				for (p = pv[c - '0']; c = *p; p++)
				{
					if (c < 040 || c >= 0177)
						printf("\\%o", c);
					else
						putchar(c);
				}
				continue;
			}
			printf("%c", c);
		}
	}
}



/*
**  END_JOB -- "cannot happen"
*/

end_job()
{
	syserr("end_job??");
}

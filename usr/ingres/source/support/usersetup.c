# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"../unix.h"

/*
**  Initialize Users File From Passwd File
**
**	Everyone in /etc/passwd is entered into the users file.  All
**	users can access all databases.
**
**	User codes are assigned sequentially.  This should therefore
**	probably be run once only, when INGRES is first installed.
**	Otherwise, usercodes can change mysteriously.
**
**	The optional parameter replaces the root of the INGRES subtree
**	as found in /etc/passwd.  The INGRES user must be installed
**	(with that name) when usersetup is run.  If this parameter
**	is a minus ("-"), output goes to the standard output.
**
**	The initialization file is initialized to "<home>/.ingres",
**	where <home> is the home directory in the passwd file.
*/

main(argc, argv)
int	argc;
char	**argv;
{
	register int		i;
	char			buf[MAXLINE + 1];
	char			*pathname;
	register char		*code;
	char			*field[UF_NFIELDS];
	register FILE		*iop;
	extern char		*Proc_name;

	Proc_name = "USERSETUP";
	pathname = NULL;

	code = "aa";
	if ((iop = fopen("/etc/passwd", "r")) == NULL)
		syserr(0, "cannot open /etc/passwd for reading");

	/* scan for INGRES in /etc/passwd */
	while (fgetline(buf, MAXLINE, iop))
	{
		i = decode(buf, field);
		if (!sequal(USERINGRES, field[0]))
			continue;
		pathname = field[i - 1];

		break;
	}

	/* test for INGRES entry found */
	if (!pathname)
		syserr(0, "USERINGRES not installed as UNIX user");

	/* get override pathname */
	if (argc > 1)
		pathname = argv[1];

	/* rewind passwd file */
	if (fclose(iop))
		syserr("fclose");
	if ((iop = fopen("/etc/passwd", "r")) == NULL)
		syserr("open /etc/passwd 2");

	/* open output file as needed */
	if (pathname[0] != '-')
	{
		concat(pathname, "/files/users", buf);
		if ((i = open(buf, 0)) >= 0)
			syserr(0, "%s already exists", buf);
		if ((i = creat(buf, 0644)) < 0)
			syserr("Cannot create %s", buf);
		close(i);
		if (freopen(buf, "w", stdout) == NULL)
			syserr("cannot open %s", buf);
	}

	while (fgetline(buf, MAXLINE, iop))
	{
		i = decode(buf, field);
		/* print username & code */
		printf("%s:%s:%s:%s:%s:::%s/.ingres::\n",
			field[0],	/* user name */
			code,
			field[2],	/* user id */
			field[3],	/* user group */
			sequal(field[0], USERINGRES) ? "177777" : "000000",
			field[i - 1]);	/* working directory */
		next(code);
	}
	flush();
}


decode(buf, field)
char	*buf;
char	*field[];
{
	register char	*cp, c;
	register int	i;

	field[0] = buf;
	for (i = 0, cp = buf; (c = *cp) != '\0'; cp++)
	{
		if (c == ':')
		{
			*cp = '\0';
			i++;
			field[i] = cp + 1;
		}
	}

	return (i);
}




next(code)
char	code[2];
{
	register char	*c;
	register char	a, b;

	c = code;
	a = c[0];
	b = c[1];

	if (++b > 'z')
	{
		b = '0';
	}
	else if (b == '9' + 1)
	{
		b = 'a';
		if (a == 'Z')
		{
			write(2, "Too many users\n", 15);
			exit(-1);
		}
		if (++a > 'z')
		{
			a = 'A';
		}
	}

	c[0] = a;
	c[1] = b;
}

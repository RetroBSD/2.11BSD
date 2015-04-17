# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"../access.h"
# include	"../pipes.h"
# include	"../symbol.h"
# include	"../unix.h"

/*
**  CREATDB -- create database (or modify database status)
**
**	This program creates a new database.  It takes the name of
**	the new database (syntax defined below) and a series of
**	flags (also defined below).
**
**	In order to perform this command, you must be enabled by
**	having the U_CREATDB bit set in the user status word
**	of the users file.
**
**	The -m flag specifies that the directory for the database
**	already exists.  It stands for "mounted-file-system",
**	because this is presumably when you might use this feature.
**	The directory must be empty.
**
**	The -e flag specifies that the database already exists.
**	It must be in all ways a valid database.  This mode allows
**	you to turn flags on and off, as controlled by the other
**	flags.
**
**	Usage:
**		creatdb [flags] databasename
**
**	Positional Parameters:
**		databasename -- the name of the database to create.
**			It must conform to all the usual rules
**			about names.  Notice that this is more
**			restrictive than UNIX usually is:  names
**			must begin with an alpha, and must be
**			composed of alphanumerics.  It may be
**			at most 14 characters long.  Underscore
**			counts as an alphabetic.
**
**	Flags:
**		-m
**			This is a mounted filesystem.  Actually,
**			this just means that the directory in which
**			the database is to reside already exists.
**			It must be empty.
**		-e
**			This database exists.  When the -e flag is
**			specified, the database is brought up to date,
**			rather than created.  Things which may be
**			changed with the -e flag is anything that
**			affects the database status or the relation
**			status bits.
**		-uXX
**			Run as user XX (usercode or login name).  This
**			flag may only be used by the INGRES superuser.
**			Normally, the database administrator for the
**			new database is the user who performs the
**			command, but the -u flag allows INGRES to
**			give the database to someone else.  Thus, it
**			is possible for someone to be a DBA without
**			having the U_CREATDB bit set.
**		-Fpathname
**			Use the named file as the database template.
**			This is, of course, for debugging use only.
**		+-c
**			Turn concurrency control on/off.  The default
**			for a new database depends on the dbtmplt file,
**			but as of this writing it defaults on.
**		+-q
**			Turn query modification on/off.
**		+-l
**			Turn protection violation logging on/off.
**
**	Files:
**		.../files/dbtmplt<VERSION>
**			This file drives the entire program.  The
**			syntax of this file is defined below in
**			readdbtemp().  Briefly, it tells the database
**			status, the relations in an 'empty' database,
**			and the status and format of those relations.
**		.../data/base
**			This directory is the directory in which all
**			databases eventually reside.  Each database is
**			a subdirectory of this directory.
**
**	Return Codes:
**		zero -- success
**		else -- failure.
**
**	Defined Constants:
**		MAXRELNS
**			This defines the maximum number of relations
**			which may be declared in the dbtmplt file.
**		MAXDBTEMP
**			The maximum size of the dbtmplt file.  This
**			determines the maximum number of characters
**			which may be in the file.
**
**	Requires:
**		Much.  Notably, be careful about changes to any
**		access methods, especially automatic references to
**			to the 'admin' file or relation relation.
**		create() -- to actually create relations.
**
**	Compilation Flags:
**		xB_UNIX -- if defined, says that we have a "Berkeley
**			UNIX" system, with no group id's.  If
**			undefined, specifies either a version seven
**			UNIX (with 16-bit group id's) or version six
**			UNIX (with 8-bit group id's); either way,
**			"setgid(getgid())" will work.
**
**	Trace Flags:
**		-Tn, as below:
**
**		1 -- makereln()
**		2 -- create()
**		10 -- makeadmin()
**		12 -- makefile()
**		20 -- makedb()
**
**	Diagnostics:
**		No database name specified.
**		You may not access this database
**			Your entry in the users file says this is
**			not a valid database for you.
**		You are not a valid INGRES user
**			You do not have a users file entry, and can
**			not do anything with INGRES at all.
**		You are not allowed this command
**			The U_CREATDB bit is not set in your users
**			file entry.
**		You may not use the -u flag
**			Only the INGRES superuser may say s/he is
**			someone else.
**		%s does not exist
**			With -e or -m, the directory does not exist.
**		%s already exists
**			Without either -e or -m, the database
**			(actually, the directory) already exists.
**		%s is not empty
**			With the -m flag, the directory you named
**			must be empty.
**		You are not the DBA for this database
**			With the -e flag, you must be the database
**			administrator.
**
**	Syserrs:
**		fork
**			A fork request (for the mkdir command) failed;
**			probably if you try again later things will
**			work
**		setuid
**			The call to setuid() (to reset the user id
**			to that of the actual user) failed.  This
**			"cannot happen".
**		setgid
**			Same for setgid().
**		exec /bin/mkdir
**			It was not possible to execute the mkdir
**			command, used to create a directory for
**			the database.
**		chmod
**			The chmod command on the database (after the
**			return from mkdir) failed -- probably due
**			to the mkdir command failing somehow before
**			creating the directory.  Notice that we do
**			not check the return from mkdir, since we
**			are not sure that it is unpolluted on all
**			systems.
**		chdir %s
**			After creating the database, we could not
**			change directory into it.
**		open .../files/dbtmplt
**			The dbtmplt file does not exist or is not
**			readable.
**		creatdb: closer(rel) %d
**			It was not possible to close the relation
**			relation after finishing the bulk of the
**			creatdb.
**		closer(att) %d
**			Ditto for attribute relation.
**		main: creat admin
**			It was not possible to create the 'admin'
**			file in the database.  This is, of course,
**			a total disaster.
**		main: write Admin
**			It was not possible to write the admin file
**			out -- probably out of disk space.
**
**	Compilation Instructions:
**		% setup creatdb
**
**		- which translates to -
**
**		% cc -n -O creatdb.c error.c ../../lib/dbulib \
**			../../lib/access ../../lib/utility
**		% chmod 4711 a.out
**
**	History:
**		8/15/79 (eric) (6.2/7) -- changed 'mkdir' stuff to
**			change into /mnt/ingres/data/base and then
**			mkdir on 'database' instead of 'Dbpath' --
**			to avoid dependence on a mkdir bug on our
**			system.  Changed to assume that argv[argc]
**			is NULL instead of -1 for v7.  Added #include
**			of unix.h to get unix version flags.
**		10/11/78 (eric) -- -F option added.
**		8/3/78 (eric) -- written.
*/




# define	MAXRELNS	20
# define	MAXDBTEMP	2000

/* relation & attribute reln descriptors used in access methods */
extern struct descriptor	Reldes;
extern struct descriptor	Attdes;

extern int	Status;		/* user status, set by initucode */
int		Dbstat;		/* database status */
int		Dbson, Dbsoff;	/* same: bits turned on, bits turned off */
struct reldes
{
	int	bitson;
	int	bitsoff;
	char	*parmv[MAXPARMS];
};
struct reldes	Rellist[MAXRELNS];
char		Delim;
extern char	*Dbpath;



main(argc, argv)
int	argc;
char	*argv[];
{
	register int		i;
	int			admin;			/* file desc */
	char			adminbuf[BUFSIZ];
	extern struct admin	Admin;
	extern int		errno;
	auto int		code;
	struct relation		relk;
	char			*database;
	char			**av;
	register char		*p;
	char			*user_ovrd;
	int			faterr;
	register int		*flagv;
	char			*dbtmpfile;
	extern char		*Parmvect[];
	extern char		*Flagvect[];
	int			exists;
	extern char		Version[];
	int			*flaglkup();
	char			*ztack();

#	ifdef xSTR1
	tTrace(&argc, argv, 'T');
#	endif

	/*
	**  Do a lot of magic initialization.
	*/

	exists = 0;
	i = initucode(argc, argv, TRUE, NULL, -1);
	switch (i)
	{
	  case 0:
	  case 5:
		exists = 1;
		break;

	  case 6:
		exists = -1;

	  case 1:
		break;

	  case 2:
		printf("You are not authorized to create this database\n");
		exit(-1);

	  case 3:
		printf("You are not a valid INGRES user\n");
		exit(-1);

	  case 4:
		printf("No database name specified\n");
	usage:
		printf("Usage: creatdb [-uname] [-e] [-m] [+-c] [+-q] dbname\n");
		exit(-1);

	  default:
		syserr("initucode %d", i);
	}

	faterr = 0;
	dbtmpfile = 0;
	for (av = Flagvect; (p = *av) != NULL; av++)
	{
		if (p[0] != '-' && p[0] != '+')
			syserr("flag %s", p);
		switch (database[1])
		{
		  case 'F':		/* dbtmplt file */
			if (p[2] == 0)
				goto badflag;
			dbtmpfile = &p[2];
			break;
		
		  default:
			if (flagv = flaglkup(p[1], p[0]))
			{
				if (p[0] == '+')
					*flagv = 1;
				else
					*flagv = -1;
				continue;
			}
		badflag:
			printf("bad flag %s\n", p);
			faterr++;
			continue;

		}
		if (p[0] == '+')
			goto badflag;
	}

	/* check for legality of database name */
	database = Parmvect[0];
	if (Parmvect[1] != NULL)
	{
		printf("Too many parameters to creatdb");
		goto usage;
	}
	if (!check(database))
	{
		printf("Illegal database name %s\n", database);
		exit(-1);
	}

	if ((Status & U_CREATDB) == 0)
	{
		printf("You are not allowed this command\n");
		exit(-1);
	}

	/* end of input checking */
	if (faterr != 0)
		exit(2);

	/* now see if it should have been there */
	if (flagval('m') || flagval('e'))
	{
#		ifdef xSTR3
		if (tTf(1, 14))
			printf("Dbpath = '%s'\n", Dbpath);
#		endif
		if (flagval('e') && exists <= 0)
		{
			printf("Database %s does not exist\n", database);
			exit(-1);
		}

		if (chdir(Dbpath) < 0)
		{
			printf("Directory for %s does not exist\n", database);
			exit(-1);
		}

		if (!flagval('e'))
		{
			/* make certain that it is empty */
			freopen(".", "r", stdin);
			for (i = 0; i < 16; i++)
				getw(stdin);
			while ((i = getw(stdin)) != EOF)
			{
				if (i != 0)
					syserr(0, "%s is not empty", database);
				for (i = 0; i < 7; i++)
					getw(stdin);
			}
		}
		else
		{
			/* check for correct owner */
			acc_init();
			if (!bequal(Usercode, Admin.adhdr.adowner, 2))
				syserr(0, "You are not the DBA for this database");
		}
	}
	else
	{
		if (exists != 0)
		{
			printf("Database %s already exists\n", database);
			exit(-1);
		}

		/* create it */
		i = fork();
		if (i < 0)
			syserr("fork err");
		if (i == 0)
		{
			/* enter database directory */
			initdbpath(NULL, adminbuf, FALSE);
			if (chdir(adminbuf) < 0)
				syserr("%s: cannot enter", adminbuf);

			/* arrange for proper permissions */
			if (setuid(getuid()))
				syserr("setuid");
#			ifndef xB_UNIX
			if (setgid(getgid()))
				syserr("setgid");
#			endif
			execl("/bin/mkdir", "mkdir", database, 0);
			syserr("exec /bin/mkdir");
		}
		while (wait(&code) != -1)
			continue;
		if (chdir(Dbpath) < 0)
			syserr("could not chdir into %s; probably bad default mode in '/bin/mkdir'", Dbpath);
		i = fork();
		if (i < 0)
			syserr("fork 2");
		if (i == 0)
		{
			setuid(getuid());
			if (chmod(".", 0777))
				syserr("chmod");
			exit(0);
		}

		while (wait(&code) != -1)
			;
		if ((code & 0377) != 0)
			exit(code);

	}

	/* reset 'errno', set from possibly bad chdir */
	errno = 0;

	/* determine name of dbtmplt file and open */
	if (dbtmpfile == NULL)
	{
		smove(Version, adminbuf);
		for (i = 0; adminbuf[i] != 0; i++)
			if (adminbuf[i] == '/')
				break;
		adminbuf[i] = 0;
		dbtmpfile = ztack(ztack(Pathname, "/files/dbtmplt"), adminbuf);
	}
	if (freopen(dbtmpfile, "r", stdin) == NULL)
		syserr("dbtmplt open %s", dbtmpfile);
	
	readdbtemp();

	/* check for type -- update database status versus create */
	if (flagval('e'))
		changedb();
	else
		makedb();

	/* close the cache descriptors */
#	ifdef xSTR3
	if (tTf(50, 0))
	{
		printf("Attdes.reltid = ");
		dumptid(&Attdes.reltid);
		printf("Reldes.reltid = ");
		dumptid(&Reldes.reltid);
	}
#	endif
	if (i = closer(&Attdes))
		syserr("creatdb: closer(att) %d", i);
	if (i = closer(&Reldes))
		syserr("creatdb: closer(rel) %d", i);

	/* bring tupcount in 'admin' file to date */
	bmove(&Reldes.reltups, &Admin.adreld.reltups, sizeof Reldes.reltups);
	bmove(&Attdes.reltups, &Admin.adattd.reltups, sizeof Attdes.reltups);

	/* clean up some of the fields to catch problems later */
	Admin.adreld.relfp = Admin.adattd.relfp = -1;
	Admin.adreld.relopn = Admin.adattd.relopn = 0;

	if ((admin = creat("admin", FILEMODE)) < 0)
		syserr("main: creat admin");
	if (write(admin, &Admin, sizeof Admin) != sizeof Admin)
		syserr("main: write Admin");
	close(admin);

	/* exit successfully */
	exit(0);
}




/*
**  Rubout processing.
*/

rubproc()
{
	exit(-2);
}
/*
**  READDBTEMP -- read the dbtmplt file and build internal form
**
**	This routine reads the dbtmplt file (presumably openned as
**	the standard input) and initializes the Dbstat (global database
**	status word) and Rellist variables.
**
**	Rellist is an array of argument vectors, exactly as passed to
**	'create'.
**
**	The syntax of the dbtmplt file is as follows:
**
**	The first line is a single status word (syntax defined below)
**	which is the database status.
**
**	The rest of the file is sets of lines separated by blank lines.
**	Each set of lines define one relation.  Two blank lines in a
**	row or an end-of-file define the end of the file.  Each set
**	of lines is broken down:
**
**	The first line is in the following format:
**		relname:status
**	which defines the relation name and the relation status for
**	this relation (syntax defined in 'getstat' below).  Status
**	may be omitted, in which case a default status is assumed.
**
**	Second through n'th lines of each set define the attributes.
**	They are of the form:
**		attname		format
**	separated by a single tab.  'Format' is the same as on a
**	'create' statement in QUEL.
**
**	Notice that we force the S_CATALOG bit to be on in any
**	case.  This is because the changedb() routine will fail
**	if the -e flag is ever used on this database if any
**	relation appears to be a user relation.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		Dbstat gets the database status.
**		Rellist is created with a list of the relations,
**			(as parameter vectors -- just as passed to
**			create).  The entry after the last entry
**			has its pv[0] == NULL.
**
**	Requires:
**		getstat -- to read status words.
**		getname -- to read names.
**		Delim -- set to the delimiter which terminated getstat
**			or getname.
**
**	Called By:
**		main
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		readdbtemp: bad Dbstat -- the Dbstat entry was not
**			terminated with a newline.
**		readdbtemp: bad rel delim -- a relation name entry
**			did not terminate with a newline.
**		readdbtemp: bad att delim -- an attribute entry
**			did not terminate with a colon.
**		readdbtemp: bad type delim -- an attribute type entry
**			did not end with a newline.
*/

readdbtemp()
{
	static char		buf[MAXDBTEMP];
	register struct reldes	*r;
	register char		**q;
	register int		i;
	int			j;
	char			*p;
	int			defrstat;
	auto int		bitson, bitsoff;

	/* read database status */
	defrstat = S_CATALOG | S_NOUPDT | S_CONCUR | S_PROTALL;
	bitson = bitsoff = 0;
	Dbstat = getstat(A_DBCONCUR, &Dbson, &Dbsoff);
	if (Delim == ':')
		defrstat = getstat(defrstat, &bitson, &bitsoff);
	if (Delim != '\n')
		syserr("readdbtemp: bad Dbstat %c", Delim);

	/* compute default relation status */

	/* start reading relation info */
	p = buf;
	for (r = Rellist; ; r++)
	{
		r->bitson = bitson;
		r->bitsoff = bitsoff;

		q = r->parmv;

		/* get relation name */
		q[1] = p;
		p += getname(p) + 1;

		/* test for end of dbtmplt file */
		if (q[1][0] == 0)
			break;
		
		/* get relation status */
		i = getstat(defrstat, &r->bitson, &r->bitsoff);
		i |= S_CATALOG;		/* guarantee system catalog */
		*q++ = p;
		*p++ = ((i >> 15) & 1) + '0';
		for (j = 12; j >= 0; j -= 3)
			*p++ = ((i >> j) & 07) + '0';
		*p++ = 0;
		q++;
		if (Delim != '\n')
			syserr("readdbtemp: bad rel %c", Delim);
		
		/* read attribute names and formats */
		for (;;)
		{
			/* get attribute name */
			*q++ = p;
			p += getname(p) + 1;
			if (q[-1][0] == 0)
				break;
			if (Delim != '\t')
				syserr("readdbtemp: bad att %c", Delim);
			
			/* get attribute type */
			*q++ = p;
			p += getname(p) + 1;
			if (Delim != '\n')
				syserr("readdbtemp: bad type %c", Delim);
		}

		/* insert end of argv signal */
		*--q = NULL;

		/* ad-hoc overflow test */
		if (p >= &buf[MAXDBTEMP])
			syserr("readdbtemp: overflow");
	}
	/* mark the end of list */
	q[1] = NULL;
}
/*
**  GETSTAT -- Get status word
**
**	A status word is read from the standard input (presumably
**	'dbtmplt').  The string read is interpreted as an octal
**	number.  The syntax is:
**		N{+c+N[~N]}
**	where N is a number, + is a plus or a minus sign, and c is
**	a flag.  '+c+N1[~N2]' groups are interpreted as follows:
**	If flag 'c' is set (assuming the preceeding character is a +,
**	clear if it is a -), then set (clear) bits N1.  If tilde N2
**	is present, then if flag 'c' is unset (called as '-c' ('+c'))
**	clear (set) bits N2; if ~N2 is not present, clear (set)
**	bits N1.
**
**	For example, an entry might be (but probably isn't):
**		1-c-1+q+6~2
**	having the following meaning:
**
**	1. Default to the 1 bit set.
**
**	2. If the -c flag is specified, clear the '1' bit.  If the
**	+c flag is specified, set the '1' bit.  If it is unspecified,
**	leave the '1' bit alone.
**
**	3. If the +q flag is specified, set the '2' bit and the '4'
**	bit.  If the -q flag is specified, clear the '2' bit (but leave
**	the '4' bit alone).  If the +-q flag is unspecified, leave
**	those bits alone.
**
**	Thus, a database with this entry is initially created with
**	the 1 bit on.  The '4' bit is a history, which says if the
**	'q' flag has ever been set, while the '2' bit tells if it is
**	currently set.
**
**	Parameters:
**		def -- the default to return if there is no number
**			there at all.
**		bitson -- a pointer to a word to contain all the
**			bits to be turned on -- used for the -e flag.
**		bitsoff -- same, for bits turned off.
**
**	Returns:
**		The value of the status word.
**		There are no error returns.
**
**	Side Effects:
**		File activity.
**
**	Requires:
**		getchar -- to get input characters
**		roctal -- to read an octal number.
**		ungetc -- to give a character back during parsing.
**		Delim -- set the the terminating delimitor in roctal().
**		flagval() -- to get the value of a flag.
**
**	Called By:
**		readdbtemp
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		getstat: bad fmt %c -- a '+' or '-' was expected.
*/

getstat(def, bitson, bitsoff)
int	def;
int	*bitson;
int	*bitsoff;
{
	register int	c;
	register int	stat;
	register int	i;
	int		setbits;
	int		clrbits;
	char		ctlch;

	/* reset bits being turned on and off */
	*bitson = *bitsoff = 0;

	/* check to see if a base status word is defined */
	if (Delim == '\n' || (Delim = c = getchar()) < '0' || c > '7')
	{
		/* no, use default */
		stat = def;
	}
	else
	{
		/* read base status field */
		ungetc(c, stdin);
		stat = roctal();
	}

	/* scan '+c+N' entries */
	for (;;)
	{
		/* check for a flag present */
		c = Delim;
		if (c == '\n' || c == ':')
			return (stat);
		if (c != '+' && c != '-')
		{
		badfmt:
			syserr("getstat: bad fmt %c", c);
		}
		
		/* we have some flag -- get it's value */
		i = flagval(getchar());

		/* save sign char on flag */
		ctlch = c;

		/* get sign on associated number and the number */
		c = getchar();
		if (c != '+' && c != '-')
			goto badfmt;
		setbits = roctal();

		/* test whether action on -X same as on +X */
		if (Delim == '~')
		{
			/* they have different bits (see above) */
			clrbits = roctal();
		}
		else
		{
			/* on 'creatdb -e -X', use opposite bits of +X */
			clrbits = setbits;
		}

		/* test for any effect at all */
		if (i == 0)
			continue;

		/* test whether we should process the '+N' */
		if ((ctlch == '-') ? (i < 0) : (i > 0))
			i = setbits;
		else
		{
			i = clrbits;

			/* switch sense of bit action */
			if (c == '+')
				c = '-';
			else
				c = '+';
		}

		if (c == '+')
		{
			stat |= i;
			*bitson |= i;
		}
		else
		{
			stat &= ~i;
			*bitsoff |= i;
		}
	}
}
/*
**  ROCTAL -- Read an octal number from standard input.
**
**	This routine just reads a single octal number from the standard
**	input and returns its value.  It will only read up to a non-
**	octal digit.  It will also skip initial and trailing blanks.
**	'Delim' is set to the next character in the input stream.
**
**	Parameters:
**		none
**
**	Returns:
**		value of octal number in the input stream.
**
**	Side Effects:
**		'Delim' is set to the delimiter which terminated the
**			number.
**		File activity on stdin.
**
**	Requires:
**		getchar() -- to get the input characters.
**		Delim -- as noted above.
**
**	Called By:
**		getstat()
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

roctal()
{
	register int	c;
	register int	val;

	val = 0;

	/* skip initial blanks */
	while ((c = getchar()) == ' ')
		continue;

	/* get numeric value */
	while (c >= '0' && c <= '7')
	{
		val = (val << 3) | (c - '0');
		c = getchar();
	}

	/* skip trailing blanks */
	while (c == ' ')
		c = getchar();

	/* set Delim and return numeric value */
	Delim = c;
	return (val);
}
/*
**  GETNAME -- get name from standard input
**
**	This function reads a name from the standard input.  A
**	name is defined as a string of letters and digits.
**
**	The character which caused the scan to terminate is stored
**	into 'Delim'.
**
**	Parameters:
**		ptr -- a pointer to the buffer in which to dump the
**			name.
**
**	Returns:
**		The length of the string.
**
**	Side Effects:
**		File activity on standard input.
**
**	Requires:
**		getchar()
**		Delim
**
**	Called By:
**		readdbtemp
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

getname(ptr)
char	*ptr;
{
	register int	len;
	register int	c;
	register char	*p;

	len = 0;

	for (p = ptr; (c = getchar()) != EOF; len++)
	{
		/* check for end of name */
		if ((c < 'a' || c > 'z') &&
		    (c < '0' || c > '9'))
			break;

		/* store character into buffer */
		*p++ = c;
	}

	/* null-terminate the string */
	*p = '\0';

	/* store the delimiting character and return length of string */
	Delim = c;
	return (len);
}
/*
**  MAKEDB -- make a database from scratch
**
**	This is the code to make a database if the -e flag is off.
**
**	The first step is to make a copy of the admin file
**	in the internal 'Admin' struct.  This is the code which
**	subsequently gets used by openr and opencatalog.  Notice
**	that the admin file is not written out; this happens after
**	makedb returns.
**
**	Next, the physical files are created with one initial (empty)
**	page.  This has to happen before the 'create' call so
**	that it will be possible to flush 'relation' and 'attribute'
**	relation pages during the creates of the 'relation' and
**	'attribute' relations.  Other relations don't need this,
**	but it is more convenient to be symmetric.
**
**	The next step is to create the relations.  Of course, all
**	this really is is inserting stuff into the system catalogs.
**
**	When we are all done we open the relation relation for the
**	admin cache (which of course should exist now).  Thus,
**	the closer's in main (which must be around to update the
**	tuple count) will work right.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		A database is created!!
**		Several files will be created in the current directory,
**			one for each relation mentioned in the
**			'dbtmplt' file.
**		The 'Admin' struct will be filled in.
**
**	Requires:
**		makefile -- to create the physical file.
**		makereln -- to create the relations.
**		Rellist -- containing the list of relations to be
**			created with specs.
**		makeadmin -- to initialize the Admin struct.
**
**	Called By:
**		main
**
**	Trace Flags:
**		20
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		open(rel)
**			It was not possible to open the relation
**			relation for the Admin.adreld cache.
*/

makedb()
{
	struct descriptor	d;
	register struct reldes	*r;
	register int		pc;
	register char		**pv;

#	ifdef xSTR3
	if (tTf(51, 0))
		printf(">>makedb, Usercode = %s (%u)\n", Usercode, Usercode);
#	endif

	/* create the physical files */
	for (r = Rellist; r->parmv[1] != 0; r++)
	{
		makefile(r);
	}

	/* initialize the admin file internal cache */
	bmove(Usercode, Admin.adhdr.adowner, 2);
	Admin.adhdr.adflags = Dbstat;
	makeadmin(&Admin.adreld, Rellist[0].parmv);
	makeadmin(&Admin.adattd, Rellist[1].parmv);

	/* done with admin initialization */

	/* initialize relations */
	for (r = Rellist; r->parmv[1] != 0; r++)
	{
		makereln(r);
	}
}
/*
**  MAKEADMIN -- manually initialize descriptor for admin file
**
**	The relation descriptor pointed to by 'pv' is turned into
**	a descriptor, returned in 'd'.  Presumably, this descriptor
**	is later written out to the admin file.
**
**	Notice that the 'reltid' field is filled in sequentially.
**	This means that the relations put into the admin file
**	must be created in the same order that they are 'made'
**	(by this routine), that the format of tid's must not
**	change, and that there can not be over one page worth of
**	relations in the admin file.  Our current system currently
**	handles this easily.
**
**	Parameters:
**		d -- the descriptor to get the result.
**		pv -- a parm vector in 'create' format, which drives
**			this routine.
**
**	Returns:
**		none
**
**	Side Effects:
**		none
**
**	Requires:
**		ingresname
**		oatoi
**		atoi
**		typeconv
**
**	Called By:
**		main
**
**	Trace Flags:
**		10
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		makeadmin: type err %c -- a bad type specifier
**			occured in the dbtmplt file.
**		makeadmin: len err %s -- a length was specified
**			which was not an integer.
*/



makeadmin(d, pv)
struct descriptor	*d;
char			*pv[];
{
	register struct descriptor	*des;
	register char			**p;
	register int			i;
	auto int			len;
	static int			tid;
	char				fname[MAXNAME + 3];

	des = d;
	p = pv;

#	ifdef xSTR2
	if (tTf(10, -1))
		printf("creating %s in admin\n", p[1]);
#	endif
	i = oatoi(*p++);
	ingresname(*p++, Usercode, fname);
	bmove(fname, des->relid, MAXNAME + 2);
	des->relstat = i;
	des->relatts = 0;
	des->relwid = 0;
	des->relspec = M_HEAP;
	des->reltid = tid++;
	des->relfp = open(fname, 2);
	if (des->relfp < 0)
		syserr("makeadmin: open %s", fname);
	des->relopn = (des->relfp + 1) * -5;

	/* initialize domain info */
	for (; *p++ != NULL; p++)
	{
		i = typeconv(p[0][0]);
		if (i < 0)
			syserr("dbtmplt: type err %c", p[0][0]);
		des->relfrmt[++(des->relatts)] = i;
		if (atoi(&p[0][1], &len) != 0)
			syserr("makeadmin: len err %s", p[0]);
		des->relfrml[des->relatts] = len;
		des->reloff[des->relatts] = des->relwid;
		des->relwid += len;
	}
}
/*
**  MAKEFILE -- make an 'empty' file for a relation
**
**	This routine creates a file with a single (empty) page
**	on it -- it is part of the 'create' code, essentially.
**
**	Parameters:
**		rr -- a pointer to the 'reldes' structure for this
**			relation (file).
**
**	Returns:
**		none
**
**	Side Effects:
**		A file with one page is created.
**
**	Requires:
**		ingresname -- to create the actual file name.
**		formatpg -- to put the single page into the relation.
**		creat -- to create the file
**		FILEMODE -- the mode of the created file.
**
**	Called By:
**		makedb
**		changedb
**
**	Trace Flags:
**		12
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		creat %s
**			The file named could not be created.
**		formatpg
**			The 'formatpg' call failed; check the
**			access method error for why.
*/

makefile(rr)
struct reldes	*rr;
{
	register struct reldes	*r;
	struct descriptor	d;
	long			npages;

	r = rr;

	ingresname(r->parmv[1], Usercode, d.relid);
#	ifdef xSTR1
	if (tTf(12, 0))
		printf("creat %s\n", d.relid);
#	endif
	if ((d.relfp = creat(d.relid, FILEMODE)) < 0)
		syserr("creat %s", d.relid);
	npages = 1;
	if (formatpg(&d, npages))
		syserr("formatpg");
	close(d.relfp);
}
/*
**  MAKERELN -- make a relation
**
**	This is the second half of the create, started by 'makefile'.
**
**	This routine just sets up argument vectors and calls create,
**	which does the real work.
**
**	Parameters:
**		rr -- a pointer to the Rellist entry for the relation
**			to be created.
**
**	Returns:
**		none
**
**	Side Effects:
**		Information will be inserted into the 'relation' and
**			'attribute' relations.
**
**	Requires:
**		Rellist -- with a list of relations to be created
**			and data descriptions.
**		create -- to actually do the create.
**		'Admin' must be filled in.  Openr must use 'Admin'
**			and never look at the physical 'admin' file.
**
**	Called By:
**		makedb
**		changedb
**
**	Trace Flags:
**		1
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		create %d
**			The call to 'create' failed, for reason
**			given.
*/

makereln(rr)
struct reldes	*rr;
{
	register struct reldes	*r;
	register int		pc;
	register char		**pv;
	int			i;

	r = rr;

	pc = 0;
	for (pv = r->parmv; *pv != NULL; pv++)
		pc++;
	pv = r->parmv;
#	ifdef xSTR1
	if (tTf(1, 0))
		prargs(pc, pv);
#	endif
	i = create(pc, pv);
	if (i != 0)
		syserr("create %d", i);
}
/*
**  CHECK -- check database name syntax
**
**	The name of a database is checked for validity.  A valid
**	database name is not more than 14 characters long, begins
**	with an alphabetic character, and contains only alpha-
**	numerics.  Underscore is considered numeric.
**
**	Parameters:
**		st -- the string to check.
**
**	Returns:
**		TRUE -- ok.
**		FALSE -- failure.
**
**	Side Effects:
**		none
**
**	Requires:
**		length -- to check the length of the string.
**
**	Called By:
**		main
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

check(st)
char	*st;
{
	register char	c;
	register char	*p;

	p = st;

	/* check string length */
	if (length(p) > 14)
		return (FALSE);

	/* check the first character of the string for alphabetic */
	c = *p++;
	if (c < 'a' || c > 'z')
		return (FALSE);

	/* check the rest for alphanumeric */
	while ((c = *p++) != 0)
	{
		if (c == '_')
			continue;
		if (c >= '0' && c <= '9')
			continue;
		if (c >= 'a' && c <= 'z')
			continue;
		return (FALSE);
	}
	return (TRUE);
}
/*
**  FLAGLKUP -- look up user flag
**
**	This routine helps support a variety of user flags.  The
**	routine takes a given user flag and looks it up (via a
**	very crude linear search) in the 'Flags' vector, and
**	returns a pointer to the value.
**
**	The 'flag' struct defines the flags.  The 'flagname' field
**	is the character which is the flag id, for example, 'c'
**	in the flag '-c'.  The 'flagtype' field defines how the
**	flag may appear; if negative, only '-c' may appear, if
**	positive, only '+c' may appear; if zero, either form may
**	appear.  Finally, the 'flagval' field is the value of the
**	flag -- it may default any way the user wishes.
**
**	Parameters:
**		flagname -- the name (as defined above) of the
**			flag to be looked up.
**		plusminus -- a character, '+' means the '+x' form
**			was issued, '-' means the '-x' form was
**			issued, something else means *don't care*.
**			If an illegal form was issued (that is,
**			that does not match the 'flagtype' field
**			in the structure), the "not found" return
**			is taken.
**
**	Returns:
**		NULL -- flag not found, or was incorrect type,
**			as when the '+x' form is specified in the
**			parameters, but the 'Flags' struct says
**			that only a '-x' form may appear.
**		else -- pointer to the 'flagval' field of the correct
**			field in the 'Flags' vector.
**
**	Side Effects:
**		none
**
**	Requires:
**		Flags vector -- of type 'struct flag', defined below.
**			This vector should be terminated with a
**			zero 'flagname'.
**
**	Called By:
**		main
**		flagval
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

struct flag
{
	char	flagname;	/* the name of the flag */
	char	flagtype;	/* -1: -x form; +1: +x form; 0: both */
	int	flagval;	/* user-defined value of the flag */
};

struct flag	Flags[] =
{
	'q',	0,	0,
	'l',	0,	0,
	'c',	0,	0,
	'e',	-1,	0,
	'm',	-1,	0,
	0
};

int *
flaglkup(flagname, plusminus)
char	flagname;
char	plusminus;
{
	register char		f;
	register struct flag	*p;
	register char		pm;

	f = flagname;
	pm = plusminus;

	/* look up flag in vector */
	for (p = Flags; p->flagname != f; p++)
	{
		if (p->flagname == 0)
			return (NULL);
	}

	/* found in list -- check type */
	if ((pm == '+' && p->flagtype < 0) ||
	    (pm == '-' && p->flagtype > 0))
		return (NULL);
	
	/* type is OK -- return pointer to value */
	return (&p->flagval);
}
/*
**  FLAGVAL -- return value of a flag
**
**	Similar to 'flaglkup', except that the value is returned
**	instead of the address, and no error return can occur.
**
**	Parameters:
**		fx -- the flag to look up (see flaglkup).
**
**	Returns:
**		The value of flag 'fx'.
**
**	Side Effects:
**		none
**
**	Requires:
**		flaglkup()
**
**	Called By:
**		readdbtemp()
**		main()
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		flagval: flag %c -- the value of a flag which does
**			not exist was asked for.
*/

flagval(fx)
char	fx;
{
	register char	f;
	register char	*p;

	f = fx;

	/* get value of flag */
	p = (char *) flaglkup(f, 0);

	/* test for error return, syserr if so */
	if (p == NULL)
		syserr("flagval: flag %c", f);

	/* return value */
	return (*p);
}
/*
**  CHANGEDB -- change status bits for database/relations
**
**	In this function we change the status bits for use with the
**	-e flag.
**
**	This module always uses the differential status
**	change information, so that existing bits are not touched.
**
**	We check to see that invalid updates, such as turning off
**	query modification when it is already on, can not occur.
**	This is because of potential syserr's when the system is
**	later run, e.g., because of views without instantiations.
**
**	In the second step, the database status is updated.  This is
**	done strictly in-core, and will be updated in the database
**	after we return.
**
**	The list of valid relations are then scanned.  For each
**	relation listed, a series of steps occurs:
**
**	(1) The relation is checked for existance.  If it does not
**	exist, it is created, and we return to the beginning of the
**	loop.  Notice that we don't have to change modes in this
**	case, since it already has been done.
**
**	(2) If the relation does exist, we check to see that it
**	is a system catalog.  If it is not, we have an error, since
**	this is a user relation which just happenned to have the
**	same name.  We inform the user and give up.
**
**	(3) If the relation exists, is a system catalog, and all
**	that, then we check the changes we need to make in the
**	bits.  If no change need be made, we continue the loop;
**	otherwise, we change the bits and replace the tuple in
**	the relation relation.
**
**	(4) If the relation being updated was the "relation" or
**	"attribute" relation, we change the Admin struct accordingly.
**
**	Notice that the result of all this is that all relations
**	which might ever be used exist and have the correct status.
**
**	Notice that it is fatal for either the attribute or relation
**	relations to not exist, since the file is created at the
**	same time that relation descriptors are filled in.  This
**	should not be a problem, since this is only called on an
**	existing database.
**
**	As a final note, we open the attribute relation cache not
**	because we use it, but because we want to do a closer
**	in main() to insure that the tupcount is updated in all
**	cases.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		The database is brought up to date, as described
**			above.
**		Tuples may be added or changed in system catalogs.
**		Files may be created.
**
**	Requires:
**		Dbson, Dbsoff -- bits to set/clear for the database
**			status.
**		opencatalog() -- to open the relation relation.
**		getequal() -- to get the relation relation tuple
**			for specific relations.
**		Rellist -- a list of needed relations and their
**			data description.
**		Reldes -- result of opencatalog() call.
**		replace() -- to update the relstat in the relation
**			relation for entries.
**		Admin struct -- must be filled in with the current
**			contents of the 'admin' file.
**		makefile(), makereln() -- to make the relation
**			if it does not exist at all.
**
**	Called By:
**		main
**
**	Trace Flags:
**		40
**
**	Diagnostics:
**		Relation %s already exists
**			A system catalog which must be created does
**			already exists as a user relation.
**		I'm sorry, it is not possible to turn query modification off.
**			The user tried to turn query modification off.
**			We can't allow this because it could cause
**			a syserr later; for instance, if the user
**			referenced a view which might still hang around
**			the 'relation' catalog.  This would result in
**			an attempt to open the file for the view, which
**			of course does not exist.  BANG!
**
**	Syserrs:
**		getequal
**			getequal() failed.
**		replace %d
**			replace() failed.
*/

changedb()
{
	register struct reldes	*r;
	struct relation		relk, relt;
	struct tup_id		tid;
	register int		i;

#	ifdef xSTR1
	if (tTf(40, 0))
		printf(">>> changedb: Dbson=%o, Dbsoff=%o\n", Dbson, Dbsoff);
#	endif

	/* check to see we aren't doing anything illegal */
	if (flagval('q') < 0)
	{
		syserr(0, "I'm sorry, it is not possible to turn query modification off");
	}

	/* update the database status field */
	Admin.adhdr.adflags = (Admin.adhdr.adflags | Dbson) & ~Dbsoff;

	/* open the system catalog caches */
	opencatalog("relation", 2);
	opencatalog("attribute", 0);

	/* scan the relation list:- Rellist */
	for (r = Rellist; r->parmv[1] != 0; r++)
	{
		/* see if this relation exists */
		clearkeys(&Reldes);
		setkey(&Reldes, &relk, r->parmv[1], RELID);
		i = getequal(&Reldes, &relk, &relt, &tid);

		if (i < 0)
			syserr("changedb: getequal");

		if (i > 0)
		{
			/* doesn't exist, create it */
			printf("Creating relation %s\n", r->parmv[1]);
			makefile(r);
			makereln(r);
		}
		else
		{
			/* exists -- check to make sure it is the right one */
			if ((relt.relstat & S_CATALOG) == 0)
			{
				/* exists as a user reln -- tough luck buster */
				printf("Relation %s already exists -- I cannot bring this database\n", r->parmv[1]);
				printf("  up to date.  Sorry.\n");
				exit(3);
			}

			/* it exists and is the right one -- update status */
			if (r->bitson == 0 && r->bitsoff == 0)
				continue;

			/* actual work need be done */
			relt.relstat = (relt.relstat | r->bitson) & ~r->bitsoff;

			/* replace tuple in relation relation */
			i = replace(&Reldes, &tid, &relt, FALSE);
			if (i != 0)
				syserr("changedb: replace %d", i);

			/* update Admin struct if "relation" or "attribute" */
			if (sequal(r->parmv[1], "relation"))
				Admin.adreld.relstat = relt.relstat;
			else if (sequal(r->parmv[1], "attribute"))
				Admin.adattd.relstat = relt.relstat;
		}
	}
}
/*
**  READADMIN -- read the admin file into the 'Admin' cache
**
**	This routine opens and reads the 'Admin' cache from the
**	'admin' file in the current directory.
**
**	This version of the routine is modified for creatdb --
**	the '-e' flag is checked, and nothing is performed
**	unless it is set.
**
**	If not set, the 'relation' and 'attribute' relations
**	are opened, and the descriptors for them in the Admin
**	struct are filled in with their file descriptors.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		The 'Admin' struct is filled in.
**		The 'relation...xx' and 'attribute...xx' files are
**			opened.
**
**	Requires:
**		'Admin' struct.
**		'Usercode' must have the user code of the DBA.
**		ingresname() -- to create the physical file name.
**		flagval() -- to test the -e flag.
**
**	Called By:
**		acc_init (accbuf.c)
**		changedb
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		adminread: open admin
**			It was not possible to open the admin file.
**		adminread: read admin
**			There was a read error when reading the 'admin'
**			file.
**		adminread: open rel
**			It was not possible to open the 'relation..xx'
**			file.
**		adminread: open att
**			It was not possible to open the 'attribute..xx'
**			file.
*/

readadmin()
{
	register int		i;
	char			relname[MAXNAME + 4];

	/* read the stuff from the admin file */
	if (flagval('e'))
	{
		i = open("admin", 0);
		if (i < 0)
			syserr("readadmin: open admin %d", i);
		if (read(i, &Admin, sizeof Admin) != sizeof Admin)
			syserr("readadmin: read err admin");
		close(i);

		/* open the physical files for 'relation' and 'attribute' */
		ingresname(Admin.adreld.relid, Admin.adreld.relowner, relname);
		if ((Admin.adreld.relfp = open(relname, 2)) < 0)
			syserr("readadmin: open rel %d", Admin.adreld.relfp);
		ingresname(Admin.adattd.relid, Admin.adattd.relowner, relname);
		if ((Admin.adattd.relfp = open(relname, 2)) < 0)
			syserr("readadmin: open att %d", Admin.adattd.relfp);
		Admin.adreld.relopn = (Admin.adreld.relfp + 1) * -5;
		Admin.adattd.relopn = (Admin.adattd.relfp + 1) * 5;
	}

	return (0);
}

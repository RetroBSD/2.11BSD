# include	<stdio.h>
# include	<sys/types.h>
# include	<sys/dir.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"../access.h"

/*
**  DESTROY DATA BASE
**
**	This program destroys an existing database.  To be able
**	to wield this awesome power, you must be the dba for
**	the database.  Also, anyone has this power if the admin
**	the database, or the ingres superuser, and have the "-s"
**	flag requested.  If admin is trashed, the INGRES superuser
**	must either destroy the database or recover it.
**
**	If -m is specified, then the directory is not removed.
**	This is useful if the directory is a mounted file system.
**
**	History:
**		2/8/78 [eric] -- changed to use fread, and to not
**			try to unlink files with i# = 0 (null
**			entries).
*/

extern char	*Usercode;
extern int	Status;
extern char	*Pathname;
extern char	*Parmvect[];
extern char	*Flagvect[];
extern char	*Dbpath;
struct admin	Admin;

main(argc, argv)
int	argc;
char	*argv[];
{
	register int	i;
	register char	*dbase;
	int		superuser, mounted;
	char		**av;
	register char	*p;
	char		*q;

#	ifdef xSTR1
	tTrace(&argc, argv, 'T');
#	endif
	
	i = initucode(argc, argv, TRUE, NULL, -1);
	dbase = Parmvect[0];
	switch (i)
	{
	  case 0:
	  case 5:
		break;

	  case 1:
	  case 6:
		printf("Database %s does not exist\n", dbase);
		exit(-1);

	  case 2:
		printf("You are not authorized to access database %s\n", dbase);
		exit(-1);

	  case 3:
		printf("You are not an authorized INGRES user\n");
		exit(-1);

	  case 4:
		printf("No database name specified\n");
	usage:
		printf("Usage: destroydb [-s] [-m] dbname\n");
		exit(-1);

	  default:
		syserr("initucode %d", i);
	}

	mounted = superuser = 0;
	for (av = Flagvect; (p = *av) != NULL; av++)
	{
		if (p[0] != '-')
		{
		badflag:
			printf("Bad flag %s\n", p);
			goto usage;
		}
		switch (p[1])
		{

		  case 's':
			superuser++;
			break;

		  case 'm':
			mounted++;
			break;

		  default:
			goto badflag;
		}
	}

	if (Parmvect[1] != NULL)
	{
		printf("Too many parameters to destroydb\n");
		goto usage;
	}
	if (length(dbase) > 14)
		syserr(0, "invalid dbname %s", dbase);
	if (superuser && (Status & U_SUPER) == 0)
		syserr(0, "you may not use the -s flag");

	if (!superuser)
	{
		if (!bequal(Admin.adhdr.adowner, Usercode, 2))
		{
			printf("You are not the DBA for %s\n", dbase);
			exit(-1);
		}
	}

	if (chdir(Dbpath) < 0)
		syserr("chdir %s", Dbpath);

	clean(".");

	if (!mounted)
	{
		/* find end of Dbpath and trim it off. */
		for (p = q = Dbpath; *p != '\0'; p++)
			if (*p == '/')
				q = p;
		*q++ = '\0';
		if (chdir(Dbpath) < 0)
			syserr("chdir(%s)", Dbpath);
		execl("/bin/rmdir", "/bin/rmdir", q, 0);
	}
}



clean(dirname)
char	*dirname;
{
	DIR		*dirp;
	struct direct	*directp;

	if ((dirp = opendir(dirname)) == NULL)
		syserr("can't open \".\" for cleaning");

	while ((directp = readdir(dirp)) != NULL) {
		if (!strcmp(directp->d_name, "."))
			continue;
		if (!strcmp(directp->d_name, ".."))
			continue;
		unlink(directp->d_name);
	}

	closedir(dirp);
}



/*
**  Rubout processing.
*/

rubproc()
{
	exit(-2);
}

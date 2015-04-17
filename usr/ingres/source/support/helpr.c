# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"../access.h"
# include	"../lock.h"


extern int	Status;

main(argc, argv)
int	argc;
char 	*argv[];
{
	extern struct out_arg	Out_arg;
	register char		**av;
	register int		i;
	register char		*p;
	extern char		*Parmvect[];
	extern char		*Flagvect[];
	extern char		*Dbpath;
	int			nc;
	char			*newpv[MAXPARMS];
	char			**nv, *qm;
	char			*qmtest();

#	ifdef xSTR1
	tTrace(&argc, argv, 'T');
#	endif

	i = initucode(argc, argv, TRUE, NULL, M_SHARE);
#	ifdef xSTR2
	if (tTf(0, 1))
		printf("initucode=%d, Dbpath='%s'\n", i, Dbpath);
#	endif
	switch (i)
	{
	  case 0:
	  case 5:
		break;

	  case 1:
	  case 6:
		printf("Database %s does not exist\n", Parmvect[0]);
		exit(-1);

	  case 2:
		printf("You are not authorized to access this database\n");
		exit(-1);

	  case 3:
		printf("You are not a valid INGRES user\n");
		exit(-1);

	  case 4:
		printf("No database name specified\n");
	usage:
		printf("usage: helpr database [relname ...]\n");
		exit(-1);

	  default:
		syserr("initucode %d", i);
	}

	if (Flagvect[0] != NULL)
	{
		printf("No flags are allowed for this command\n");
		goto usage;
	}

	if (chdir(Dbpath) < 0)
		syserr("cannot access data base %s", p);
#	ifdef xTTR2
	if (tTf(1, 0))
		printf("entered database %s\n", Dbpath);
#	endif

	/* initialize access methods (and Admin struct) for user_ovrd test */
	acc_init();

	av = &Parmvect[1];	/* get first param after datbase name */
	p = *av;
	if (p == NULL)
	{
		/* special case of no relations specified */
		nv = newpv;
		*nv++ = "2";
		*nv = (char *) -1;
		help(1, newpv);
	}
	else
	{
		do
		{
			nc = 0;
			nv = newpv;

			if ((qm = qmtest(p)) != NULL)
			{
				/* either help view, integrity or protect */
				av++;
				while ((p = *av++) != NULL)
				{
					if ((i = (int) qmtest(p)) != NULL)
					{
						/* change of qmtest result */
						qm = (char *) i;
						continue;
					}
					*nv++ = qm;
					*nv++ = p;
					nc += 2;
				}
				*nv = (char *) -1;
				display(nc, newpv);
			}
			else
			{
				/* help relname */
				while ((p = *av++) != NULL && qmtest(p) == NULL)
				{
					if (sequal("all", p))
					{
						*nv++ = "3";
						nc++;
					}
					else
					{
						*nv++ = "0";
						*nv++ = p;
						nc += 2;
					}
				}
				*nv = (char *) -1;
				help(nc, newpv);
				/* this backs av up one step, so 
				 * that it points at the keywords (permit,
				 * integrity, view) or the NULL
				 */
				--av;
			}
		} while (p != NULL);
	}
	flush();
	exit(0);
}



char *qmtest(parm)
char	*parm;
{
	register char	*p;

	p = parm;
	if (sequal("view", p))
		return ("4");
	else if (sequal("permit", p))
		return ("5");
	else if (sequal("integrity", p))
		return ("6");
	else
		return (NULL);
}


rubproc()
{
	exit(1);
}

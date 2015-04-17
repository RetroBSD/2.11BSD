# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"


struct proctemplate
{
	char	*proc_name;
	int	proc_code;
	char	proc_exec;
	char	proc_func;
};

struct proctemplate Proc_template[] =
{
	"copy",		mdCOPY,		0,	0,
	"create",	mdCREATE,	0,	0,
	"destroy",	mdDESTROY,	0,	0,
	"display", 	mdDISPLAY,	0,	0,
	"help",		mdHELP,		0,	0,
	"index",	mdINDEX,	0,	0,
	"modify",	mdMODIFY,	0,	0,
	"print",	mdPRINT,	0,	0,
	"save",		mdSAVE,		0,	0,
	"update",	mdUPDATE,	0,	0,
	"resetrel",	mdRESETREL,	0,	0,
	"remqm",	mdREMQM,	0,	0,
	0
};


init_proctab(proc, my_id)
char	*proc;
char	my_id;

/*
**	Initialize from the run time process table
**
**	Each entry in the proctable is seached for in the
**	runtime process table. If an entry isn't found it
**	is considered a syserr. If an entry is found the
**	first one which has an exec_id of "my_id" is taken.
**	Otherwise, the very first entry given will be used.
*/

{
	register struct proctemplate	*pt;
	register char			*p, *name;
	char				*found;
	int				i;

#	ifdef xZTR1
	if (tTf(0, 0))
		printf("INIT_PTAB:\n");
#	endif
	
	for (pt = Proc_template; name = pt->proc_name; pt++)
	{
		i = length(name);
		p = proc;
		found = NULL;

		while (*p)
		{
			if (bequal(p, name, i))
			{
				/* found a match. skip past the colon */
				while (*p++ != ':')
					;

				/* look for an alias in this dbu */
				found = p;
				do
				{
					if (*p == my_id)
					{
						found = p;
						break;	/* found one */
					}
					p++;
					p++;
				} while (*p++ == ',');

#				ifdef xZTR1
				if (tTf(0, 1))
					printf("%s: %.2s\n", name, found);
#				endif

				pt->proc_exec = *found++;
				pt->proc_func = *found;
				break;

			}

			/* skip to next */
			while (*p++ != '\n');
		}

		if (found == NULL)
			syserr("init_proctab: %s missing from proctab", name);
	}
}


get_proctab(code, exec, func)
int	code;
char	*exec;
char	*func;

/*
**	Find the correct exec & func for a given code
*/

{
	register struct proctemplate	*p;
	register int			i;

	i = code;
	for (p = Proc_template; p->proc_name != NULL; p++)
	{
		if (i == p->proc_code)
		{
			/* found entry */
			*exec = p->proc_exec;
			*func = p->proc_func;
			return;
		}
	}

	/* entry not there */
	syserr("get_proc:bad code %d", i);
}

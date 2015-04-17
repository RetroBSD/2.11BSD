# include	"../ingres.h"

/*
**  TRIM_RELNAME -- trim blanks from relation name for printing
**
**	A relation name (presumably in 'ingresname' format: MAXNAME
**	characters long with no terminating null byte) has the
**	trailing blanks trimmed off of it into a local buffer, so
**	that it can be printed neatly.
**
**	Parameters:
**		name -- a pointer to the relation name
**
**	Returns:
**		a pointer to the trimmed relation name.
**
**	Side Effects:
**		none
**
**	Files:
**		none
**
**	Compilation Flags:
**		none
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
**
**	History:
**		3/22/78 (eric) -- stripped off from 'help'
*/

char *trim_relname(name)
char	*name;
{
	register char	*old, *new;
	register int	i;
	static char	trimname[MAXNAME + 1];

	old = name;
	new = trimname;
	i = MAXNAME;

	while (i--)
		if ((*new++ = *old++) == ' ')
		{
			new--;
			break;
		}

	*new = '\0';

	return (trimname);
}

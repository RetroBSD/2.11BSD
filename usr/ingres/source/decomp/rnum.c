# include	"../ingres.h"
# include	"decomp.h"

# define	FIRSTNUM	MAXRANGE + 3
# define	LASTNUM		100

/*
**	Internal numbers are used in decomp to
**	represent relation names. The numbers
**	from 0 to FIRSTNUM-1 refer to the names
**	stored in Name_table[].
**
**	The number from FIRSTNUM to LASTNUM have
**	names which are computed from aa, ab, etc.
*/
char	Name_table[FIRSTNUM-1][MAXNAME];

char	Num_used[LASTNUM+1];


rnum_assign(name)
char	*name;

/*
**	Assign an internal number rnum to name.
*/

{
	register int	i;

	for (i = 0; i < FIRSTNUM; i++)
		if (Num_used[i] == 0)
		{
			bmove(name, Name_table[i], MAXNAME);
			Num_used[i]++;
			return (i);
		}
	syserr("rnum_assign:no room");
}

rnum_alloc()

/*
**	Allocate the next available name
*/

{
	register int	i;
	register char	*cp;

	cp = &Num_used[FIRSTNUM];
	for (i = FIRSTNUM; i < LASTNUM; i++)
		if (*cp++ == 0)
		{
			--cp;
			(*cp)++;
			return (i);
		}
	syserr("no free names");
}


char *rnum_convert(num)
int	num;

/*
**	Convert internal relation number
**	to its real name. Guarantee '\0' at end.
*/

{
	register int	i;
	register char	*ret, *cp;
	static char	temp[MAXNAME+1];
	extern char	*Fileset;
	extern char	*concat();

	i = num;
	if (i > LASTNUM || Num_used[i] == 0)
		syserr("no name for %d", i);

	ret = temp;

	if (i < FIRSTNUM)
	{
		bmove(Name_table[i], ret, MAXNAME);
	}
	else
	{
		/* compute temp name */
		cp = concat("_SYS", Fileset, ret);
		pad(ret, MAXNAME);
		i -= FIRSTNUM;
		*cp++ = i/26 + 'a';
		*cp = i%26 + 'a';
	}
	return (ret);
}

rnum_remove(num)
int	num;

/*
**	Remove a num from the used list
*/

{
	register int	i;
	register char	*cp;

	cp = &Num_used[num];

	if (*cp == 0)
		syserr("cant remove %d", num);
	*cp = 0;
}


rnum_last()

/*
**	returns number of largest assigned temp number.
**	zero if none
*/

{
	register int	i;
	register char	*cp;

	for (i = LASTNUM; i >= FIRSTNUM; i--)
		if (Num_used[i])
			return (i);

	return (0);
}


rnum_temp(rnum)
int	rnum;

/*
**	Predicate to check whether rnum is a temporary relation or not
*/

{
	register int	i;

	i = rnum;

	return (i >= FIRSTNUM || bequal("_SYS", rnum_convert(i), 4));
}

rnum_init()

/*
**	Clear tag fields from previous query
*/

{
	register char	*cp;
	register int	i;

	cp = Num_used;
	i = FIRSTNUM;
	while (--i)
		*cp++ = 0;
}

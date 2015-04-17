# include	"../ingres.h"
# include	"decomp.h"

/*
** Allocation of range table.
** The size of the range table for decomp is
** MAXRANGE plus 2; 1 for a free aggregate slot and 1 for
** a free secondary index slot.
*/
struct rang_tab	Rangev[MAXRANGE+2];	/* global range table with extra slot for FREEVAR and SECINDXVAR */



initrange()
{
	register struct rang_tab	*rt;

	for (rt = Rangev; rt <= &Rangev[MAXRANGE+1]; rt++)
		rt->relnum = -1;
}


savrang(locrang, var)
int		locrang[];
int		var;

/*
**	Save the entry for var in the range table.
*/

{
	register int	i;

	i = var;
	locrang[i] = Rangev[i].relnum;
}

rstrang(locrang, var)
int		locrang[];
int		var;

/*
**	Restore the entry for var from the local range
**	table locrang.
*/

{
	register int	i;

	i = var;
	Rangev[i].relnum = locrang[i];
}



new_range(var, relnum)
int	var;
int	relnum;

/*
**	Update the range name. It is up to
**	the calling routine to openr the new rel.
*/

{
	register int	i, old;

	i = var;

	old = Rangev[i].relnum;
	Rangev[i].relnum = relnum;

	return (old);
}


newquery(locrang)
int	locrang[];

/*
**	Make a copy of the current range table.
*/

{
	register struct rang_tab	*rp;
	register int			*ip, i;

	ip = locrang;
	rp = Rangev;

	for (i = 0; i < MAXRANGE; i++)
		*ip++ = (rp++)->relnum;
}


endquery(locrang, reopen)
int	locrang[];
int	reopen;

/*
**	Check the range table to see if any
**	relations changed since the last call
**	to newquery. If so, they were caused
**	by reformat. Restore back the orig relation
**	Reopen it if reopen == TRUE.
*/

{
	register struct rang_tab	*rp;
	register int			*ip, i;
	int				old;

	rp = Rangev;
	ip = locrang;

	initp();
	for (i = 0; i < MAXRANGE; i++)
	{
		if (rp->relnum != *ip)
		{
#			ifdef xDTR1
			if (tTf(15, -1))
			printf("reformat or reduct changed var %d (%d,%d)\n", i, *ip, rp->relnum);
#			endif

			old = new_range(i, *ip);
			dstr_mark(old);
			if (reopen)
				openr1(i);
		}

		ip++;
		rp++;
	}

	dstr_flush(0);
}


char *rangename(var)
int	var;

/*
**	Return the name of the variable "var"
**	in the range table
*/

{
	char	*rnum_convert();

	return (rnum_convert(Rangev[var].relnum));
}

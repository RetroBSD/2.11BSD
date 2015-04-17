# include	"../ingres.h"
# include	"../symbol.h"
# include	"../access.h"

clr_tuple(desc, tuple)
struct descriptor	*desc;
char			*tuple;

/*
**	Clr_tuple initializes all character domains
**	to blank and all numeric domains to zero.
*/

{
	register struct descriptor	*d;
	register char			*tup;
	register int			i;
	int				j, pad;

	d = desc;

	for (i = 1; i <= d->relatts; i++)
	{
		if (d->relfrmt[i] == CHAR)
			pad = ' ';
		else
			pad = 0;

		tup = &tuple[d->reloff[i]];
		j = d->relfrml[i] & I1MASK;

		while (j--)
			*tup++ = pad;
	}
}

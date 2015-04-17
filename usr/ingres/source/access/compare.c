# include	"../ingres.h"
# include	"../access.h"
# include	"../symbol.h"
# include	"../aux.h"

icompare(ax, bx, frmt, frml)
char	*ax, *bx, frmt, frml;
{
	register char	*a, *b;
	register int	length;
	int		atemp[4], btemp[4];

	length = frml & I1MASK;
	if (frmt == CHAR)
		return (scompare(ax, length, bx, length));
	a = (char *) atemp;
	b = (char *) btemp;
	bmove(ax, a, length);
	bmove(bx, b, length);
	if (bequal(a, b, length))
		return (0);
	switch (frmt)
	{
	  case INT:
		switch (length)
		{
		  case 1:
			return (i1deref(a) - i1deref(b));
		  case 2:
			return (i2deref(a) - i2deref(b));
		  case 4:
			return (i4deref(a) > i4deref(b) ? 1 : -1);
		};
	  case FLOAT:
		if (frml == 4)
		{
			return (f4deref(a) > f4deref(b) ? 1 : -1);
		}
		else
		{
			return (f8deref(a) > f8deref(b) ? 1 : -1);
		}
	};
}


kcompare (dx, tuple, key)
struct descriptor	*dx;	/*relation descriptor	*/
char	tuple[MAXTUP];		/*tuple to be compared	*/
char	key[MAXTUP];		/*second tuple or key	*/
/*
 * compares all domains indicated by SETKEY in the tuple to the
 * corressponding domains in the key.
 * the comparison is done according to the format of the domain
 * as specified in the descriptor.
 *
 * function values:
 *	<0 tuple < key
 * 	=0 tuple = key
 *	>0 tuple > key
 *
 */
{
	register int			i, tmp;
	register struct descriptor	*d;

	d = dx;
	for (i = 1; i <= d->relatts; i++)
		if (d->relgiven[i])
			if (tmp = icompare(&tuple[d->reloff[i]],
			        &key[d->reloff[i]],
			        d->relfrmt[i],
				d->relfrml[i]))
			{
				return (tmp);
			}
	return (0);
}

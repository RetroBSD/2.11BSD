/*
**  BLOCK EQUALITY TEST
**
**	blocks `a' and `b', both of length `l', are tested
**		for absolute equality.
**	Returns one for equal, zero otherwise.
*/

bequal(a, b, l)
char	*a, *b;
int	l;
{
	register char		*p, *q;
	register int		m;

	p = a;
	q = b;
	for (m = l; m > 0; m -= 1)
		if (*p++ != *q++)
			return(0);
	return(1);
}

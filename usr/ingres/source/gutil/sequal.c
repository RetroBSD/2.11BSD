/**
 **	STRING EQUALITY TEST
 **		null-terminated strings `a' and `b' are tested for
 **			absolute equality.
 **		returns one if equal, zero otherwise.
 **/

sequal(a, b)
char	*a, *b;
{
	register char		*p, *q;

	p = a;
	q = b;
	while (*p || *q)
		if (*p++ != *q++)
			return(0);
	return(1);
}

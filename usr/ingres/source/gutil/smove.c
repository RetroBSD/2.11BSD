/*
**  STRING MOVE
**
**	The string `a' is moved to the string `b'.  The length
**	of the string is returned.  `a' must be null terminated.
**	There is no test for overflow of `b'.
*/

smove(a, b)
char	*a, *b;
{
	register int	l;
	register char	*p, *q;

	p = a;
	q = b;
	l = 0;
	while (*p)
	{
		*q++ = *p++;
		l++;
	}
	*q = '\0';
	return (l);
}

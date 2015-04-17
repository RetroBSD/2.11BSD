/*
**  LOCAL STRING CONCATENATE
**	Strings `a' and `b' are concatenated and left in an
**	internal buffer.  A pointer to that buffer is returned.
**
**	Ztack can be called recursively as:
**		ztack(ztack(ztack(w, x), y), z);
*/

char *ztack(a, b)
char	*a, *b;
{
	register char	*c;
	register char	*q;
	static char	buf[101];
	
	c = buf;
	
	q = a;
	while (*q)
		*c++ = *q++;
	q = b;
	while (*q)
		*c++ = *q++;
	*c = '\0';
	if (buf[100] != 0)
		syserr("ztack overflow: %s", buf);
	return (buf);
}

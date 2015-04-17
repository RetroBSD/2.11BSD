char *locv(lg)
long	lg;
{
	register char	*a;
	register char	*j;
	static char	b[11];

	a = b;
	if (lg < 0)
	{
		*a++ = '-';
		lg = -lg;
	}
	j = &b[10];
	*j-- = 0;
	do
	{
		*j-- = lg % 10 + '0';
		lg /= 10;
	} while (lg);
	do
	{
		*a++ = *++j;
	} while (*j);
	return (b);
}

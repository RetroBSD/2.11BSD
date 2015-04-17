itoa(i1, a1)
int	i1;
char	*a1;
{
	register char	*a;
	register int	i;
	register char	*j;
	char		b[6];

	i = i1;
	a = a1;
	if (i < 0)
	{
		*a++ = '-';
		i = -i;
	}
	j = &b[5];
	*j-- = 0;
	do
	{
		*j-- = i % 10 + '0';
		i /= 10;
	} while (i);
	do
	{
		*a++ = *++j;
	} while (*j);
	return (0);
}

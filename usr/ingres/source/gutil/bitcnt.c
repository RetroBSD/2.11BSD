bitcnt(var)
int	var;

/*
**	Count the number of 1's in the integer var. As long
**	as left shift is zero fill this routine is machine
**	independent.
*/

{
	register int	i, j, ret;

	j = var;

	for (ret = 0, i = 1; i; i <<= 1)
		if (i & j)
			ret++;

	return (ret);
}

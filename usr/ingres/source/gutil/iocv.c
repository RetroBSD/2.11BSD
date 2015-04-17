/*
**  INTEGER OUTPUT CONVERSION
**
**	The integer `i' is converted to ascii using itoa and put
**	into the static buffer `buf'.  The address of `buf' is
**	returned.
*/

char *iocv(i)
int	i;
{
	static char	buf[7];

	itoa(i, buf);
	return (buf);
}

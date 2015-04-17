/*
**  ASCII CHARACTER STRING TO 32-BIT INTEGER CONVERSION
**
**	`a' is a pointer to the character string, `i' is a
**	pointer to the doubleword which is to contain the result.
**
**	The return value of the function is:
**		zero:	succesful conversion; `i' contains the integer
**		+1:	numeric overflow; `i' is unchanged
**		-1:	syntax error; `i' is unchanged
**
**	A valid string is of the form:
**		<space>* [+-] <space>* <digit>* <space>*
*/

atol(a1, i)
char	*a1;
long	*i;
{
	register int	sign;	/* flag to indicate the sign */
	long		x;	/* holds the integer being formed */
	register char	c;
	register char	*a;
	long		longconst();

	a = a1;
	sign = 0;
	/* skip leading blanks */
	while (*a == ' ')
		a++;
	/* check for sign */
	switch (*a)
	{

	  case '-':
		sign = -1;

	  case '+':
		while (*++a == ' ');
	}

	/* at this point everything had better be numeric */
	x = 0;
	while ((c = *a) <= '9' && c >= '0')
	{
		/* since most c compilers don't support long constants, */
		/* the code below is the contortion for: */
		/* if (x > 2147483647 / 10) */
		if (x > longconst(06314, 0146314))	/* check if mult by 10 will overflow */
			return (1);
		x = x * 10 + (c - '0');
		if (x < 0)	/* check if new digit caused overflow */
			return (1);
		a++;
	}

	/* eaten all the numerics; better be all blanks */
	while (c = *a++)
		if(c != ' ')			/* syntax error */
			return (-1);
	*i = sign ? -x : x;
	return (0);		/* successful termination */
}

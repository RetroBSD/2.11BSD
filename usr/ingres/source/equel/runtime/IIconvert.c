# include	"../../symbol.h"

/*
**
**	IIconvert -- Equel run-tme routine to convert
**		numeric values of one type and length, to a
**		(not necessarily) different type and length.
**
**		The source numeric can be i1, i2, i4, f4, or f8.
**
**		The source number will be converted to the
**		type and length specified in the destination.
**		It also must be one of i1, i2, i4, f4, or f8.
**
**		IIconvert returns 0 if no overflow occured,
**		otherwise it returns -1
**
*/


IIconvert(inp, outp, sf, slen, df, dlen)
char		*inp;		/* input area */
char		*outp;		/* output area */
int		sf;		/* format of the source number */
int		slen;		/* length of the source number */
int		df;		/* format of the dest */
int		dlen;		/* length of the dest */
{
	char		number[8];	/* dummy buffer */
	register char	*num;
	register int	sl;		/* refers to length
					 * of "number"
					 */
	register int	dl;

#	define	i1deref(x)	(*((char *)(x)))
#	define	i2deref(x)	(*((int *)(x)))
#	define	i4deref(x)	(*((long *)(x)))
#	define	f4deref(x)	(*((float *)(x)))
#	define	f8deref(x)	(*((double *)(x)))

	dl = dlen;
	sl = slen;
	num = number;
	IIbmove(inp, num,  sl);	/* copy number into buffer */

	if (sf != df)
	{
		/* if the source and destination formats are
		 * different then the source must be converted
		 * to i4 if the dest is int, otherwise to f8 
		 */

		if (df == FLOAT)	/* {sf == INT} INT->f8 */
		{
			switch (sl)
			{

			  case 1:
				f8deref(num) = i1deref(num);	/* i1 to f8 */
				break;

			  case 2:
				f8deref(num) = i2deref(num);	/* i2 to f8 */
				break;

			  case 4:
				f8deref(num) = i4deref(num);	/* i4 to f8 */
			}
			sl = 8;			/* df == INT && sf == FLOAT 
						 * && sl == 8
						 */
		}
		else
		{
			/* {df == INT && sf == FLOAT} FLOAT->i4 */

			/* check if float >  2**31 */
			if (sl == 8)
				f4deref(num) = f8deref(num);	/* f8 to f4 */

			if (f4deref(num) > 2147483647.0 || f4deref(num) < -2147483648.0)
				return (-1);
			i4deref(num) = f4deref(num);
			sl = 4;
		}
	}

	/* number is now the same type as destination */
	/* convert lengths to match */

	if (sl != dl)
	{
		/* lengths don't match. convert. */
		if (df == FLOAT)
		{
			if (dl == 8)
				f8deref(num) = f4deref(num);	/* f4 to f8 */
			else
				f4deref(num) = f8deref(num);	/* f8 to f4 with rounding */
		}
		else
		{
			switch (dl)
			{

			  case 1:
				if (sl == 2)		/* i2 to i1 */
				{
					if (i2deref(num) > 127 || i2deref(num) < -128)
						return (-1);
					i1deref(num) = i2deref(num);	
				}
				else			/* i4 to i1 */
				{
					if (i4deref(num) > 127 || i4deref(num) < -128)
						return (-1);
					i1deref(num) = i4deref(num);
				}
				break;

			  case 2:
				if (sl == 1)		/* i1 to i2 */
				{
					i2deref(num) = i1deref(num);
				}
				else			/* i4 to i2 */
				{
					if (i4deref(num) > 32767 || i4deref(num) < -32768)
						return (-1);
					i2deref(num) = i4deref(num);
				}
				break;

			  case 4:
				if (sl == 1)		/* i1 to i4 */
					i4deref(num) = i1deref(num);
				else			/* i2 to i4 */
					i4deref(num) = i2deref(num);
			}
		}
	}

	/* conversion is complete 
	 * copy the result into outp
	 */

	IIbmove(num, outp, dl);
	return (0);
}

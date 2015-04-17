# include	<stdio.h>

/*
**  FGETLINE -- get line from file
**
**	This routine reads a single newline-terminated line from
**	a file.
**
**	Parameters:
**		buf -- the buffer in which to place the result.
**		maxch -- the maximum number of characters the
**			buffer can hold.
**		iop -- the file from which to read the characters.
**
**	Returns:
**		NULL -- end-of-file or error.
**		else -- 'buf'.
**
**	Side Effects:
**		File activity on 'iop'.
**
**	Requires:
**		getc -- to get the characters.
**
**	Called By:
**		USER
**
**	Compilation Flags:
**		none
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
**
**	History:
**		8/2/78 (eric) -- written
*/

char *fgetline(buf, maxch, iop)
char	*buf;
int	maxch;
FILE	*iop;
{
	register char	*p;
	register int	n;
	register int	c;

	p = buf;

	for (n = maxch; n > 0; n--)
	{
		c = getc(iop);

		/* test for end-of-file or error */
		if (c == EOF)
		{
			*p = 0;
			return (NULL);
		}

		/* test for newline */
		if (c == '\n')
			break;

		/* insert character into buffer */
		*p++ = c;
	}

	/* null-terminate the string */
	*p = 0;
	return (buf);
}

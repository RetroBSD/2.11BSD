/*
**  PUT CHARACTER
**
**	This routine just calls putchar normally, unless the character
**	to be printed is a control character, in which case the octal
**	equivalent is printed out.  Note that tab, newline, and so
**	forth are considered to be control characters.
**
**	Parameters:
**		ch -- the character to print.
**
**	Returns:
**		nothing.
**
**	Side Effects:
**		none
**
**	Requires:
**		putchar
**
**	Called by:
**		printatt
**		prargs
**		(maybe others?)
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
**		2/27/78 (eric) -- adapted from old xputchar in 6.0.
*/



xputchar(ch)
char	ch;
{
	register int	c;

	c = ch;
	if (c < 040 || c >= 0177)
	{
		putchar('\\');
		putchar(((c >> 6) & 03) | '0');
		putchar(((c >> 3) & 07) | '0');
		putchar((c & 07) | '0');
	}
	else
		putchar(c);
}

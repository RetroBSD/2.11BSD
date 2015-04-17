/*
**  FIND HIGH ORDER BIT POSITION
**
**	The position of the highest ordered one bit in `word' is
**	found and returned.  Bits are numbered 0 -> 15, from
**	right (low-order) to left (high-order) in word.
*/

bitpos(word)
int	word;
{
	register int	wd, i, j;
	int		pos;

	wd = word;

	pos = -1;

	for (i = 1, j = 0; wd; i <<= 1, j++)
	{
		if (wd & i)
		{
			pos = j;
			wd &= ~i;
		}
	}

	return (pos);
}

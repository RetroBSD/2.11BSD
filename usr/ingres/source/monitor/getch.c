# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"monitor.h"

/*
**  GET CHARACTER
**
**	This routine is just a getchar, except it allows a single
**	backup character ("Peekch").  If Peekch is nonzero it is
**	returned, otherwise a getchar is returned.
*/

char
getch()
{
	register char	c;

	c = Peekch;
	if (c)
		Peekch = 0;
	else
	{
		c = getc(Input);
	}
	if (c < 0)
		c = 0;

	/* deliver EOF if newline in Oneline mode */
	if (c == '\n' && Oneline)
	{
		Peekch = c;
		c = 0;
	}

	return (c);
}

# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"monitor.h"

/*
**  GET FILE NAME
**
**	This routine collects a file name up to a newline and returns a
**	pointer to it.  Keep in mind that it is stored in a static
**	buffer.
**
**	Uses trace flag 17
*/

char *getfilename()
{
	static char	filename[81];
	register char	c;
	register int	i;
	register char	*p;
	extern char	getch();

	Oneline = TRUE;
	macinit(&getch, 0, 0);

	/* skip initial spaces */
	while ((c = macgetch()) == ' ' || c == '\t')
		;

	i = 0;
	for (p = filename; c > 0; )
	{
		if (i++ <= 80)
			*p++ = c;
		c = macgetch();
	}
	*p = 0;
	Prompt = Newline = TRUE;

#	ifdef xMTR2
	if (tTf(17, 0))
		printf("filename \"%s\"\n", filename);
#	endif
	Oneline = FALSE;
	Peekch = 0;
	return (filename);
}

# include	"../ingres.h"

/*
**  MCALL -- call a macro
**
**	This takes care of springing a macro and processing it for
**	any side effects.  Replacement text is saved away in a static
**	buffer and returned.
**
**	Parameters:
**		mac -- the macro to spring.
**
**	Returns:
**		replacement text.
**
**	Side Effects:
**		Any side effects of the macro.
**
**	Called By:
**		go.c
**		proc_error
**
**	Compilation Flags:
**		none
**
**	Trace Flags:
**		18
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
**
**	History:
**		8/10/78 (eric) -- broken off from go().
*/

char *
mcall(mac)
char	*mac;
{
	register char	c;
	register char	*m;
	register char	*p;
	static char	buf[100];
	extern char	macsget();

	m = mac;

#	ifdef xMTR2
	tTfp(18, -1, "mcall('%s')\n", m);
#	endif

	/* set up to process the macro */
	macinit(&macsget, &mac, FALSE);

	/* process it -- throw away result */
	for (p = buf; (c = macgetch()) > 0; )
	{
#		ifdef xMTR2
		if (tTf(18, 1))
			putchar(c);
#		endif
		if (p < &buf[sizeof buf])
			*p++ = c;
	}

	*p = 0;

	return (buf);
}

# include	"../ingres.h"
# include	"../pipes.h"

/*
** GETSCR
**	returns a single character from the R_up pipe.
**	modes are:
**		0 = read normally
**		1 = prime the pipe
**		2 = sync (or flush) the pipe
*/
getscr(mode)
int	mode;
{
	extern int		Pctr;		/* vble for backup stack in scanner */
	static struct pipfrmt	a;
	register int		ctr;
	char			c;
#	ifdef xPTM
	static int		Ptimflg;	/* marks the first time through */
#	endif

	if (!mode)
	{
		ctr = rdpipe(P_NORM, &a, R_up, &c, 1);
#		ifdef xPTM
		if (Ptimflg == 0 && tTf(76, 1))
		{
			timtrace(1, 0);
			Ptimflg = 1;
		}
		if (tTf(76, 3))
			qtrace(ctr ? c : '\0');
#		endif
		return (ctr ? c : 0);
	}
	if (mode == 1)
	{
		rdpipe(P_PRIME, &a);
		Pctr = 0;
		return (0);
	}
	if (mode == 2)
	{
#		ifdef xPTM
		if (tTf(76, 1))
		{
			timtrace(2, 0);
			Ptimflg = 0;
		}
#		endif
		rdpipe(P_SYNC, &a, R_up);
		return (0);
	}
	syserr("bad arg '%d' in getscr", mode);
}

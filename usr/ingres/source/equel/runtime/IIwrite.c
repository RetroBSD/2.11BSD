# include	"../../ingres.h"
# include	"../../symbol.h"
# include	"../../pipes.h"
# include	"IIglobals.h"

/*
**	IIwrite is used to write a string to the
**	quel parser
*/

IIwrite(str)
char	*str;
{
	register char	*s;
	register int	i;

	s = str;
#	ifdef xETR1
	if (IIdebug)
		printf("write:string='%s'\n", s);
#	endif
	if (!IIingpid)
		IIsyserr("no preceding ##ingres statement");
	if (IIin_retrieve)
		IIsyserr("IIwrite:you cannot call ingres while in a retrieve");

	if((i = IIlength(s)) != 0)
		if (IIwrpipe(P_NORM, &IIoutpipe, IIw_down, s, i) != i)
			IIsyserr("IIwrite:can't write to parser(3) %d", i);
}

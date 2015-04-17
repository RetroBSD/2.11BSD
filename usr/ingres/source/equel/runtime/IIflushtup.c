# include	"../../ingres.h"
# include	"../../aux.h"
# include	"../../symbol.h"
# include	"../../tree.h"
# include	"../../pipes.h"
# include	"IIglobals.h"

/*
**	IIflushtup is called to syncronize the data pipe
**	after a retrieve.
*/

IIflushtup(file_name, line_no)
char	*file_name;
int	line_no;
{
	register int		i;
	struct retcode		ret;

	if (IIproc_name = file_name)
		IIline_no = line_no;

#	ifdef xATR1
	if (IIdebug)
		printf("IIflushtup : IIerrflag %d\n",
		IIerrflag);
#	endif

	if (IIerrflag < 2000)
	{
		/* flush the data pipe */
		IIrdpipe(P_SYNC, &IIeinpipe, IIr_front);
		IIrdpipe(P_PRIME, &IIeinpipe);

		/* flush the control pipe */
		if ((i = IIrdpipe(P_NORM, &IIinpipe, IIr_down, &ret, sizeof (ret))) == sizeof (ret))
		{
			/* there was a tuple count field */
			IItupcnt = ret.rc_tupcount;
		}
		IIrdpipe(P_SYNC, &IIinpipe, IIr_down);
		IIrdpipe(P_PRIME, &IIinpipe);
	}
	IIin_retrieve = 0;
	IIndomains = 0;
	IIdomains = 0;
}

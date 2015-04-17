# include	"../../ingres.h"
# include	"../../symbol.h"
# include	"../../pipes.h"
# include	"IIglobals.h"

/*
**	IIsync is called to syncronize the running
**	of a query with the running of the equel process.
**
**	The query is flushed and an EOP is written
**	to the quel parser.
**
**	The quel parser will write an end-of-pipe when
**	an operation is complete.
*/

IIsync(file_name, line_no)
char	*file_name;
int	line_no;
{
	if (IIproc_name = file_name)
		IIline_no = line_no;

#	ifdef xETR1
	if (IIdebug)
		printf("IIsync\n");
#	endif

	IIwrpipe(P_END, &IIoutpipe, IIw_down);
	IIwrpipe(P_PRIME, &IIoutpipe, IIw_down);

	IIerrflag = 0;	/* reset error flag. If an error occures,
			** IIerrflag will get set in IIerror
			*/
	IIrdpipe(P_SYNC, &IIinpipe, IIr_down);
	IIrdpipe(P_PRIME, &IIinpipe);
}

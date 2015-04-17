# include	"../pipes.h"

/*
**  ENDJOB -- default end of job processing
**
**	This routine is called on a clean exit from the system.  It is
**	supposed to do any cleanup processing and the like.  This copy
**	is a default version, which just returns zero.
**
**	The value of end_job() is passed as the exit status, and should
**	normally be zero.
**
**	Parameters:
**		none
**
**	Returns:
**		exit status for process.
**		(zero for this dummy version).
**
**	Side Effects:
**		as needed by user who redefines this, but none in
**		this copy.
**
**	Called From:
**		rdpipe()
*/

end_job()
{
	return (0);
}

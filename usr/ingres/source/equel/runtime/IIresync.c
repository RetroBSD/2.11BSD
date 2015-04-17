# include	"../../pipes.h"
# include	"../../ingres.h"
# include	"IIglobals.h"

/*
**  RESYNCHRONIZE PIPES AFTER AN INTERRUPT
**
**	The pipes are all cleared out.  This routines must be called
**	by all processes in the system simultaneously.  It should be
**	called from the interrupt catching routine.
**
**	History:
**		11/2/79 -- (marc) added IIeinpipe PRIME to get rid
**			of old data from the pipe which was being read
**			in the retrieve after the interrupt.
*/

extern	exit();
int	(*IIinterrupt)()	= &exit;
IIresync()
{
	struct pipfrmt		buf;
	register struct pipfrmt	*pbuf;
	extern struct pipfrmt	IIeinpipe;

	signal(2,1);
	pbuf = &buf;
	IIwrpipe(P_PRIME, pbuf, 0, 0, 0);
	/* write the sync block out */
	IIwrpipe(P_INT, pbuf, IIw_down);

	/* read the sync block back on both pipes */
	/* must be in right order */
	IIrdpipe(P_INT, pbuf, IIr_front);
	IIrdpipe(P_INT, pbuf, IIr_down);

	/* flush remaining old data from buffer */
	IIrdpipe(P_PRIME, &IIeinpipe, 0, 0, 0);

	/* Get out of a retrieve and clear errors */
	IIin_retrieve = 0;
	IIerrflag = 0;
	IIndomains = IIdomains = 0;


	/* reset the signal */
	signal(2,&IIresync);
	/* allow the user to service the interrupt */
	(*IIinterrupt)(-1);
	/*
	** If IIinterupt returns the user might hang in a retrieve
	*/

	IIsyserr("Interupt returned");
}

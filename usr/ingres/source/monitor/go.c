# include	<stdio.h>

# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"monitor.h"

/*
**  PROCESS QUERY
**
**	The appropriate messages are printed, and the query is scanned.
**	Tokens are passed to the parser.  A parser response is then
**	expected.
**
**	Uses trace flag 5
**
**	History:
**		3/2/79 (eric) -- {querytrap} stuff installed.
*/

# define	QRYTRAP		"{querytrap}"


go()
{
	struct pipfrmt	iobuf;
	FILE		*iop;
	register int	i;
	char		c;
	extern int	fgetc();
	struct retcode	rc;
	register char	*p;
	char		*macro();

	clrline(1);
	fflush(Qryiop);
	if ((iop = fopen(Qbname, "r")) == NULL)
		syserr("go: open 1");
	if (Nodayfile >= 0)
		printf("Executing . . .\n\n");

#	ifdef xMTM
	if (tTf(76, 1))
		timtrace(3, 0);
#	endif

	if (!Nautoclear)
		Autoclear = 1;

	wrpipe(P_PRIME, &iobuf, 'M', 0, 0);
	macinit(&fgetc, iop, 1);

	while ((c = macgetch()) > 0)
	{
		wrpipe(P_NORM, &iobuf, W_down, &c, 1);
	}

	wrpipe(P_END, &iobuf, W_down);
	Error_id = 0;
	rdpipe(P_PRIME, &iobuf);

	/* read number of tuples return */
	if (rdpipe(P_NORM, &iobuf, R_down, &rc, sizeof rc) == sizeof rc)
	{
		rc.rc_status = RC_OK;
		macdefine("{tuplecount}", locv(rc.rc_tupcount), TRUE);
	}
	else
		rc.rc_status = RC_BAD;
	
	if (Error_id == 0 && (p = macro(QRYTRAP)) != NULL)
		trapquery(&rc, p);
	/* clear out the rest of the pipe */
	rdpipe(P_SYNC, &iobuf, R_down);

	mcall("{continuetrap}");

#	ifdef xMTM
	if (tTf(76, 1))
		timtrace(4, 0);
#	endif
	prompt("\ncontinue");
	fclose(iop);
}

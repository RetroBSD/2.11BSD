# include	"../ingres.h"  /* needed for trace flag */
# include	"../symbol.h"
# include	"../pipes.h"
# include	"ovqp.h"
/*
**	OVQP -- the One Variable Query Processor 
**
**	accepts one variable queries from
**	decompostion. Queries are sent as a
**	list arranged in postfix order.
**	OVQP returns one of three
**	possible values for each query:
**
**			EMPTY:	no tuples satisfied the query
**
**			NONEMPTY:	at least one tuple satisfied
**					the query. if the query was
**					a simple aggregate then the
**					aggregate value will immediately
**					follow.
**
**			ERROR (CODE):	A user caused error occured
**					during query processing.
**
**/

char	*Ovqpbuf;	/* pointer to buffer area */

ovqp()
{
	register int		code;
	extern int		Equel;
	extern long		Accuread, Accuwrite, Accusread;
	char			qrybuf[LBUFSIZE];

#	ifdef xOTR1
	if (tTf(27, 0))
		printf("OVQP-	Equel=%d\n", Equel);
#	endif
#	ifdef xOTM
	if (tTf(76, 1))
		timtrace(7, 0);
#	endif
#	ifdef xOTR1
	if (tTf(50, 0))
 	{
		Accuread = 0;
		Accuwrite = 0;
		Accusread = 0;
	}
#	endif

	Ovqpbuf = qrybuf;	/* global buffer is allocated from stack */

	startovqp();

	for (;;)
	{
		if (code = getqry())	/* get next logical query */
			break;		/* query processing complete */

		/* in certain very rare instances, the strategy
		** routine will detect the fact that no tuples
		** can satisfy the qualification. If that case
		** arrises then no scan need be performed.
		*/

		if (strategy())
		{
#			ifdef xOTM
			if (tTf(76, 2))
				timtrace(9, 0);
#			endif
			code = scan();	/* scan the relation */
#			ifdef xOTM
			if (tTf(76, 2))
				timtrace(10, 0);
#			endif
		}
		else
			code = EMPTY;	/* else query is false */

		retdecomp(code);	/* return result to decomp */
	}
#	ifdef xOTM
	if (tTf(76, 1))
		timtrace(8, 0);
#	endif

	endqry(code);	/* close all relations */

#	ifdef xOTR1
	if (tTf(50, 1))
	{
		printf("OVQP read %s pages,", locv(Accuread));
		printf("%s catalog pages,", locv(Accusread));
		printf("wrote %s pages\n", locv(Accuwrite));
	}
#	endif
}

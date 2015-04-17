# include	"../ingres.h"
# include	"../access.h"

/*
closer - close a relation
DESCRIPTION

CLOSER is used to close a relation which was opened by OPENR.
CLOSER should always be called once for each OPENR.

	function values:

		<0  fatal error
		 0  success
		 1  relation was not open
 */


closer(d)
struct descriptor	*d;
{
	register struct descriptor	*dx;
	register int			i;
	register struct accbuf		*b;

	dx = d;
#	ifdef xATR1
	if (tTf(90, 8))
		printf("closer: %.14s,%s\n", dx->relid, locv(dx->reladds));
#	endif

	if (i = noclose(dx))
		return (i);

	flush_rel(dx, TRUE);	/* No error is possible since noclose()
				** has already flushed any pages
				*/

	if (close(dx->relfp))	/*close the relation*/
		i = acc_err(AMCLOSE_ERR);

	dx->relopn = 0;
	return (i);
}

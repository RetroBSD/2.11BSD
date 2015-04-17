# include	"../ingres.h"
# include	"../access.h"

/*
inpcloser - close an input relation

The relation must have been opened by openr with 
	mode 0 (read only)

return values:
	<0 fatal error
	 0 success
	 1 relation was not open
	 2 relation was opened in write mode
*/

inpcloser(dx)
struct descriptor	*dx;

{
	register struct descriptor	*d;
	register int			i;

	d = dx;
#	ifdef xATR1
	tTfp(90, 9, "inpcloser: %.14s\n", d->relid);
#	endif
	if (abs(d->relopn) != (d->relfp + 1) * 5)
		/* relation not open */
		return (1);

	if (d->relopn < 0)
		return (2);	/* relation open in write mode */

	i = flush_rel(d, TRUE);	/* flush and reset all pages */

	if (close(d->relfp))
		i = acc_err(AMCLOSE_ERR);
	d->relopn = 0;
	return (i);
}

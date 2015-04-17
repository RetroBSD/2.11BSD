# include	"../ingres.h"
# include	"../aux.h"
# include	"../access.h"

/*
noclose - update system catalogs for a relation
DESCRIPTION


	function values:

		<0  fatal error
		 0  success
		 1  relation was not open
 */


noclose(d)
struct descriptor	*d;
{
	char				name[MAXNAME + 4];
	register struct descriptor	*dx;
	register int			i;
	register struct accbuf		*b;
	struct relation			rel;

	dx = d;
#	ifdef xATR1
	if (tTf(90, 8))
		printf("noclose: %.14s,%s\n", dx->relid, locv(dx->reladds));
#	endif

	/* make sure relation relation is read/write mode */
	if (abs(dx->relopn) != (dx->relfp + 1) * 5)
		return (1);

	/* flush all pages associated with relation */
	/* if system catalog, reset all the buffers so they can't be reused */
	i = flush_rel(dx, dx->relstat & S_CATALOG);

	/* check to see if number of tuples has changed */
	if (dx->reladds != 0)
	{
		/* yes, update the system catalogs */
		/* get tuple from relation relation */
		Admin.adreld.relopn = (Admin.adreld.relfp + 1) * -5;
		if (i = get_page(&Admin.adreld, &dx->reltid))
			return (i);	/* fatal error */

		/* get the actual tuple */
		get_tuple(&Admin.adreld, &dx->reltid, &rel);

		/* update the reltups field */
		rel.reltups += dx->reladds;
		dx->reltups = rel.reltups;

		/* put the tuple back */
		put_tuple(&dx->reltid, &rel, sizeof rel);
		i = resetacc(Acc_head);
		dx->reladds = 0;
	}
	return (i);
}

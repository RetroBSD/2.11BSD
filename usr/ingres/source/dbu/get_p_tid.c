# include	"../ingres.h"
# include	"../catalog.h"
/*
**  GET_P_TID -- Get the primary tid for a relation for locking
**
**	Finds the correct tid for locking the relation. If the
**	relation is a primary relation, then the tid of the
**	relation is returned.
**
**	If the relation is a secondary index then the tid of the
**	primary relation is returned.
**
**	Parameters:
**		des - an open descriptor for the relation.
**		tidp - a pointer to a place to store the tid.
**
**	Returns:
**		none
**
**	Side Effects:
**		alters the value stored in "tidp",
**		may cause access to the indexes relation
**
**	Requires:
**		opencatalog
**
**	Called By:
**		modify
**
**	Syserrs:
**		if the indexes relation cannot be accessed or
**		is inconsistent.
**
**	History:
**		12/28/78 (rse) - written
*/



get_p_tid(des, tidp)
struct descriptor	*des;
struct tup_id		*tidp;
{
	register struct descriptor	*d;
	register struct tup_id		*tp;
	register int			i;
	struct index			indkey, itup;
	struct descriptor		ides;
	extern struct descriptor	Inddes;

	d = des;
	tp = tidp;
	if (d->relindxd < 0)
	{
		/* this is a secondary index. lock the primary rel */
		opencatalog("indexes", 0);
		setkey(&Inddes, &indkey, d->relowner, IOWNERP);
		setkey(&Inddes, &indkey, d->relid, IRELIDI);
		if (getequal(&Inddes, &indkey, &itup, tp))
			syserr("No prim for %.14s", d->relid);

		if (i = openr(&ides, -1, itup.irelidp))
			syserr("openr prim %d,%.14s", i, itup.irelidp);

		bmove(&ides.reltid, tp, sizeof (*tp));
	}
	else
		bmove(&d->reltid, tp, sizeof (*tp));
}

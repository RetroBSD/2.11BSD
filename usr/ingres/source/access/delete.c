# include	"../ingres.h"
# include	"../access.h"

/*
**	Delete - delete the tuple specified by tid
**
**	Delete removes the tuple specified by tid
**	and reclaims the tuple space.
**
**	returns:
**		<0  fatal error
**		0   success
**		2   tuple specified by tid aleady deleted
*/



delete(dx, tidx)
struct descriptor	*dx;
struct tup_id		*tidx;
{
	register struct descriptor	*d;
	register struct tup_id		*tid;
	register int			i;

	d = dx;
	tid = tidx;

#	ifdef xATR1
	if (tTf(91, 0))
	{
		printf("delete: %.14s,", d->relid);
		dumptid(tid);
	}
#	endif

	if (i = get_page(d, tid))
		return (i);

	if (i = invalid(tid))
		return (i);

	i = tup_len(tid);

	del_tuple(tid, i);
	d->reladds--;

	return (0);
}

# include	"../ingres.h"
# include	"../aux.h"
# include	"../access.h"

/*
**	Get - get a single tuple
**
**	Get either gets the next sequencial tuple after
**	"tid" or else gets the tuple specified by tid.
**
**	If getnxt == TRUE, then tid is incremented to the next
**	tuple after tid. If there are no more, then get returns
**	1. Otherwise get returns 0 and "tid" is set to the tid of
**	the returned tuple.
**
**	Under getnxt mode, the previous page is reset before
**	the next page is read. This is done to prevent the previous
**	page from hanging around in the am's buffers when we "know"
**	that it will not be referenced again.
**
**	If getnxt == FALSE then the tuple specified by tid is
**	returned. If the tuple was deleted previously,
**	get retuns 2 else get returns 0.
**
**	If getnxt is true, limtid holds the the page number
**	of the first page past the end point. Limtid and the
**	initial value of tid are set by calls to FIND.
**
**	returns:
**		<0  fatal error
**		0   success
**		1   end of scan (getnxt=TRUE only)
**		2   tuple deleted (getnxt=FALSE only)
*/


get(d, tid, limtid, tuple, getnxt)
struct descriptor	*d;
struct tup_id		*tid, *limtid;
int			getnxt;
char			*tuple;
{
	register int		i;
	register struct tup_id	*tidx;
	long			pageid, lpageid;

#	ifdef xATR1
	if (tTf(93, 0))
	{
		printf("get: %.14s,", d->relid);
		dumptid(tid);
		printf("get: lim");
		dumptid(limtid);
	}
#	endif
	tidx = tid;
	if (get_page(d, tidx))
	{
		return (-1);
	}
	if (getnxt)
	{
		pluck_page(limtid, &lpageid);
		do
		{
			while (((++(tidx->line_id)) & I1MASK) >= Acc_head->nxtlino)
			{
				tidx->line_id = -1;
				pageid = Acc_head->ovflopg;
				stuff_page(tidx, &pageid);
				if (pageid == 0)
				{
					pageid = Acc_head->mainpg;
					stuff_page(tidx, &pageid);
					if (pageid == 0 || pageid == lpageid + 1)
						return (1);
				}
				if (i = resetacc(Acc_head))
					return (i);
				if (i = get_page(d, tidx))
					return (i);
			}
		} while (!Acc_head->linetab[-(tidx->line_id & I1MASK)]);
	}
	else
	{
		if (i = invalid(tidx))
			return (i);
	}
	get_tuple(d, tidx, tuple);
#	ifdef xATR2
	if (tTf(93, 1))
	{
		printf("get: ");
		printup(d, tuple);
	}
#	endif
	return (0);
}

# include	"../ingres.h"
# include	"../access.h"
# include	"../aux.h"
# include	"../lock.h"

/*
**	ADD_OVFLO -- Allocates an overflow page which will be
**		attached to current page.  TID must be for current page.
**		TID will be updated to id of new overflow page.
*/

add_ovflo(dx, tid)
struct descriptor	*dx;
struct tup_id		*tid;
{
	register struct descriptor	*d;
	register struct accbuf		*b;
	register int			lk;
	int				i;
	long				mpage, newpage;
	struct tup_id			tidx;
	struct accbuf			*choose_buf();

	d = dx;

/*
**	save main page pointer so that it may be used when
**	setting up new overflow page.
*/
	mpage = Acc_head->mainpg;

	if (lk = (Acclock && (d->relstat & S_CONCUR) && (d->relopn < 0 )))
	{
		setcsl(Acc_head->rel_tupid);
		Acclock = FALSE;
	}

/*
**	Determine last page of the relation
*/
	last_page(d, &tidx, Acc_head);
	pluck_page(&tidx, &newpage);
	newpage++;

/*
**	choose an available buffer as victim for setting up
**	overflow page.
*/
	if ((b = choose_buf(d, newpage)) == NULL)
	{
		if (lk)
		{
			Acclock = TRUE;
			unlcs(Acc_head->rel_tupid);
		}
		return(-1);
	}

/*
**	setup overflow page
*/
	b->mainpg = mpage;
	b->ovflopg = 0;
	b->thispage = newpage;
	b->linetab[0] = b->firstup - b;
	b->nxtlino = 0;
	b->bufstatus |= BUF_DIRTY;
	if (pageflush(b))
		return (-2);

/*
**	now that overflow page has successfully been written,
**	get the old current page back and link the new overflow page
**	to it.
**	If the relation is a heap then don't leave the old main
**	page around in the buffers. This is done on the belief
**	that it will never be accessed again.
*/

	if (get_page(d, tid))
		return (-3);
	Acc_head->ovflopg = newpage;
	Acc_head->bufstatus |= BUF_DIRTY;
	i = pageflush(Acc_head);
	if (lk)
	{
		Acclock = TRUE;
		unlcs(Acc_head->rel_tupid);
	}
	if (i)
		return (-4);
	if (abs(d->relspec) == M_HEAP)
		resetacc(Acc_head);	/* no error is possible */

/*
**	now bring the overflow page back and make it current.
**	if the overflow page is still in AM cache, then this will not
**	cause any disk activity.
*/

	stuff_page(tid, &newpage);
	if (get_page(d, tid))
		return (-5);
	return (0);
}

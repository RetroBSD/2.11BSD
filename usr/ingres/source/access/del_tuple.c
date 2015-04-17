# include	"../ingres.h"
# include	"../access.h"

del_tuple(tid, width)
struct tup_id	*tid;
int		width;

/*
**	Delete the specified tuple from the
**	current page.
**
**	The space occupied by the tuple is
**	compacted and the effected remaining
**	tuple line offsets are adjusted.
*/

{
	register char	*startpt, *midpt;
	register int	i;
	char		*endpt;
	int		cnt, offset, nextline;
	int		linenum;
	char		*get_addr();

	linenum = tid->line_id & I1MASK;
	offset = Acc_head->linetab[-linenum];
	nextline = Acc_head->nxtlino;

	startpt = get_addr(tid);
	midpt = startpt + width;
	endpt = &((struct raw_accbuf *)Acc_head)->acc_buf[0] + Acc_head->linetab[-nextline];

	cnt = endpt - midpt;

	/* delete tuple */
	Acc_head->linetab[-linenum] = 0;

	/* update affected line numbers */
	for (i = 0; i <= nextline; i++)
	{
		if (Acc_head->linetab[-i] > offset)
			Acc_head->linetab[-i] -= width;
	}

	/* compact the space */
	while (cnt--)
		*startpt++ = *midpt++;
	Acc_head->bufstatus |= BUF_DIRTY;
}

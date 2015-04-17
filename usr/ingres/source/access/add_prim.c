# include	"../ingres.h"
# include	"../access.h"

/*
**	ADD_PRIM -- Add a primary page to the relation.  Assumes it is to
**		be tacked onto page in current access method buffer.  No
**		disk write is done but the page is marked for writing.
**		It is assumed that the current page in access method buffer
**		is the last physical page in the relation.
*/

/* tTf flag 86	TTF	add_prim() */

add_prim(d, tidx)
struct descriptor	*d;
struct tup_id		*tidx;
{
	register struct accbuf	*b;
	register int		i;

	b = Acc_head;
	b->mainpg = b->thispage + 1;
	b->bufstatus |= BUF_DIRTY;
	if (i = pageflush(b))
		return (i);

	/*
	** Now form the new primary page
	*/

	b->thispage = b->mainpg;
	b->mainpg = 0;
	b->ovflopg = 0;
	b->linetab[0] = b->firstup - b;
	b->nxtlino = 0;
	b->bufstatus |= BUF_DIRTY;

	/*
	** Update tid to be new page
	*/
	stuff_page(tidx, &b->thispage);
	return (0);
}

/*
** Routines for fiddlin' linked lists
** Erik E. Fair <fair@ucbarpa.berkeley.edu>
*/

#include <sys/types.h>
#include "llist.h"

extern free();
extern caddr_t	malloc();

/*
** a little wasteful for some cases, but it works, and I don't mind
** the extra padding - think of it as insurance.
*/
#define	ALIGN(x)	((x) + (sizeof(long) - (x) % sizeof(long)))

/*
** recursively free a linked list
*/
void
l_free(lp)
register ll_t *lp;
{
	if (lp->l_next == (ll_t *)NULL)
		return;
	l_free(lp->l_next);
	(void) free(lp->l_item);
}

/*
** allocate a new element in a linked list, along with enough space
** at the end of the item for the next list element header.
*/
ll_t *
l_alloc(lp, s, len)
register ll_t	*lp;
caddr_t	s;
register int len;
{
	if (s == (caddr_t)NULL || lp == (ll_t *)NULL || len <= 0)
		return((ll_t *)NULL);

	lp->l_len = len;
	len = ALIGN(len);

	if ((lp->l_item = malloc((unsigned)len + sizeof(ll_t))) == (caddr_t)NULL)
		return((ll_t *)NULL);

	bcopy(s, lp->l_item, lp->l_len);
	lp->l_next = (ll_t *)(&lp->l_item[len]);

	/*
	** set up next list entry
	*/
	lp = lp->l_next;
	lp->l_next = (ll_t *)NULL;
	lp->l_item = (caddr_t)NULL;
	lp->l_len = 0;
	return(lp);
}

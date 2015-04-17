# include	"../ingres.h"
# include	"../access.h"

dumptid(tid1)
struct tup_id	*tid1;
{
	register struct tup_id	*tid;
	long			pageid;

	tid = tid1;

	pluck_page(tid, &pageid);
	printf("tid: %s/%d\n", locv(pageid), (tid->line_id & I1MASK));
	return (0);
}

/*
**	struct for extracting page number from a tid
**	and storing in a long
*/

struct lpage
{
	char	lpg0, lpgx;
	char	lpg2, lpg1;
};

/* pluck_page extracts the three byte page_id from the tup_id struct
** and puts it into a long variable with proper allignment.
*/

pluck_page(tid, var)
struct tup_id	*tid;
long		*var;
{
	register struct tup_id	*t;
	register struct lpage	*v;

	t = tid;
	v = (struct lpage *) var;
	v->lpg0 = t->pg0;
	v->lpg1 = t->pg1;
	v->lpg2 = t->pg2;
	v->lpgx = 0;
	return (0);
}

/*	stuff_page is the reverse of pluck_page	*/

stuff_page(tid, var)
struct tup_id	*tid;
long		*var;
{
	register struct tup_id	*t;
	register struct lpage	*v;

	v = (struct lpage *) var;
	t = tid;
	t->pg0 = v->lpg0;
	t->pg1 = v->lpg1;
	t->pg2 = v->lpg2;
	return (0);
}

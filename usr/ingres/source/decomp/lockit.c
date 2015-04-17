# include	"../ingres.h"
# include	"../tree.h"
# include	"decomp.h"
# include	"../lock.h"
/*
**	lockit - sets relation locks for integrity locking
**
**	arguements:
**		root- the root of a query tree;
**		resvar- index of variable to be updated.
**
*/
lockit(root, resvar)
struct querytree	*root;
int			resvar;
{
	register struct	querytree	*r;
	register int			i, j;
	long				vlist[MAXRANGE];
	int				bmap, cv;
	char				mode;
	int				skvr;
	int				redo;
	struct descriptor		*d, *openr1();
	long				restid;
	int				k;

	r = root;
	bmap = ((struct qt_root *)r)->lvarm | ((struct qt_root *)r)->rvarm;
	if (resvar >= 0)
		bmap |= 01 << resvar;
	else
		restid = -1;
	i = 0;
	/* put relids of relations to be locked into vlist
	   check for and remove duplicates */
	for (j = 0; j < MAXRANGE; j++)
		if (bmap & (01 << j))
		{
			d = openr1(j);
			if (j == resvar)
				restid = d->reltid;
			for (k = 0; k < i; k++)
				if (vlist[k] == d->reltid)
					break;
			if (k == i)
				vlist[i++] = d->reltid;
		}
	cv = i;
/*
 *	set the locks: set the first lock with the sleep option
 *			set other locks checking for failure;
 *			if failure, release all locks, sleep on blocking
 *			lock.
 */
	skvr = -1;
	do
	{
		/* skvr is the index of the relation already locked
		   try to lock the remaining relations */
		redo = FALSE;
		for (i = 0; i < cv; i++)
			if (i != skvr)
			{
				if (restid == vlist[i])
					mode = M_EXCL;
				else
					mode = M_SHARE;
				if (setrll(A_RTN, vlist[i], mode) < 0)
					/* a lock request failed */
				{
					unlall();	/* release all locks */
					setrll(A_SLP, vlist[i], mode);
							/* wait on problem lock*/
					skvr = i;
					redo = TRUE;
					break;	/* reset the other locks */
				}
			}
	}
	while (redo);
}

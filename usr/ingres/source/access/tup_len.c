# include	"../ingres.h"
# include	"../access.h"

/*
**	Tup_len finds the amount of space occupied by the
**	tuple specified by "tid"
*/

tup_len(tid)
struct tup_id	*tid;
{
	register int	*lp, nextoff, off;
	int		lineoff, i;


	/* point to line number table */
	lp = &Acc_head->linetab[0];

	/* find offset for tid */
	lineoff = lp[-(tid->line_id & I1MASK)];

	/* assume next line number follows lineoff */
	nextoff = lp[-Acc_head->nxtlino];

	/* look for the line offset following lineoff */
	for (i = 0; i < Acc_head->nxtlino; i++)
	{
		off = *lp--;

		if (off <= lineoff)
			continue;

		if (off < nextoff)
			nextoff = off;
	}
#	ifdef xATR3
	if (tTf(89,1))
		printf("tup_len ret %d\n", nextoff - lineoff);
#	endif
	return (nextoff - lineoff);
}

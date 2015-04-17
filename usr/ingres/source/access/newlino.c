# include	"../ingres.h"
# include	"../access.h"

/*
** newlino finds a free line number which it returns and adjusts
**	the next line counter (Nxtlino) by the length of the tuple (len).
**	This routine is used to recover unused sections of the
**	line number table (Acc_head->linetab).
*/

newlino(len)
int	len;
{
	register int	*lp, newlno, nextlno;

	nextlno = Acc_head->nxtlino;
	lp = &Acc_head->linetab[0];
	for (newlno = 0; newlno < nextlno; newlno++)
	{
		if (*lp == 0)
		{
			/* found a free line number */
			*lp = Acc_head->linetab[-nextlno];
			Acc_head->linetab[-nextlno] += len;
			return (newlno);
		}
		lp--;
	}

	/* no free line numbers. use nxtlino */
	Acc_head->linetab[-(nextlno + 1)] = *lp + len;
	Acc_head->nxtlino++;
	return (nextlno);
}

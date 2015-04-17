# include	"../ingres.h"
# include	"../access.h"
# include	"../lock.h"

struct lockreq	Lock;
/*
 *	setcsl- set a critical section lock
 */
setcsl(rtid)
long	rtid;
{
	register char	*r;
	register int	i;

#	ifdef xATR1
	if (tTf(98,  0))
		printf(" setcsl r=%s \n", locv(rtid));
#	endif

	if (Alockdes < 0)
		return (1);
	Lock.lract = A_SLP;	/* sleep while waiting on lock */
	Lock.lrtype = T_CS;	/* critical section lock */
	Lock.lrmod = M_EXCL;	/* exclusive access */
	bmove(&rtid, Lock.lrel, 4);	/* copy relid */ 
	r = Lock.lpage;
	for (i = 0; i < 4; i++)
			/* zero out pageid */
		*r++ = 0;
	i = write(Alockdes, &Lock, sizeof(struct lockreq));
	return (i);
}



/*
 *	unlcs- unlock a critical section
 */
unlcs(rtid)
long	rtid;
{
	register char	*r;
	register int	i;

#	ifdef xATR1
	if (tTf(98,  1))
		printf(" unlcs r=%s \n", locv(rtid));
#	endif

	if (Alockdes < 0)
		return (1);
	Lock.lract = A_RLS1;
	Lock.lrtype = T_CS;
	bmove(&rtid, Lock.lrel, 4);	/* copy relation identifier */
	r = Lock.lpage;
	for (i = 0; i < 4; i++)
			/* zero out page id */
		*r++ = 0;
	i = write(Alockdes, &Lock, sizeof(struct lockreq));
	return (i);
}

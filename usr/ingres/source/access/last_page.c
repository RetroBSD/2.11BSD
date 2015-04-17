# include	"../ingres.h"
# include	"../access.h"
# include	"../unix.h"

/*
**	LAST_PAGE -- computes a tid for the last page in the relation.
*/

last_page(dx, tidx, bufx)
struct descriptor	*dx;
struct tup_id		*tidx;
struct accbuf		*bufx;
{
	register struct descriptor	*d;
	register struct tup_id		*tid;
	register struct accbuf		*buf;
	long				lpage;
	struct stat			stats;

	d = dx;
	tid = tidx;
	buf = bufx;
	if ((buf != 0) && (abs(d->relspec) == M_HEAP) && (buf->mainpg == 0) && (buf->ovflopg == 0))
		lpage = buf->thispage;
	else
	{
		if (fstat(d->relfp, &stats) < 0)
			syserr("last_page: fstat err %.14s", d->relid);
#		ifdef xV6_UNIX
		lpage = ((stats.st_sz1 >> 9) & 0177) + ((stats.st_sz0 & 0377) << 7) - 1;
#		endif
#		ifdef xB_UNIX
		lpage = ((stats.st_sz1 >> 9) & 0177) + ((stats.st_sz0 & 0377) << 7) - 1;
#		endif
#		ifndef xV6_UNIX
#		ifndef xB_UNIX
		lpage = stats.st_size / PGSIZE - 1;
#		endif
#		endif
#		ifdef xATR2
		if (tTf(86, 2))
			printf("fstat-lp %.12s %s\n", d->relid, locv(lpage));
#		endif
	}
	stuff_page(tid, &lpage);
	tid->line_id = 0;
	return (0);
}

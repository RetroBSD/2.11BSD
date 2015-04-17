# include	"../ingres.h"
# include	"../access.h"
# include	"../aux.h"
# include	"../lock.h"

/*
**	UNIX read/write counters
*/

long	Accuread, Accuwrite;
long	Accusread;

/* tTf flag 83	TTF	get_page() */

get_page(d1, tid)
struct descriptor	*d1;
struct tup_id		*tid;
{
	register int			i;
	register struct descriptor	*d;
	long				pageid;
	register struct accbuf		*b;
	struct accbuf			*b1;
	int				lk;		/* lock condition*/
	struct accbuf			*choose_buf();

	d = d1;
#	ifdef xATR3
	if (tTf(83, 0))
	{
		printf("GET_PAGE: %.14s,", d->relid);
		dumptid(tid);
	}
#	endif

	pluck_page(tid, &pageid);
	if ((b = choose_buf(d, pageid)) == NULL)
		return (-1);
	top_acc(b);
	lk = Acclock && (d->relstat & S_CONCUR) && (d->relopn < 0);
	if ((b->thispage != pageid) || (lk && !(b->bufstatus & BUF_LOCKED)))
	{
		if (i = pageflush(b))
			return (i);
#		ifdef xATR1
		if (tTf(83, 1))
		{
			printf("GET_PAGE: rdg pg %s", locv(pageid));
			printf(",relid %s\n", locv(d->reltid));
		}
#		endif
		b->thispage = pageid;
		if (lk)
		{
			b1 = Acc_head;
			for (; b1 != 0; b1 = b1->modf)
				if (b1->bufstatus & BUF_LOCKED)
					pageflush(b1);  /*  */
			if (setpgl(b) < 0)
				syserr("get-page: lk err");
		}
#		ifdef xATM
		if (tTf(76, 1))
			timtrace(17, &pageid, &d->reltid);
#		endif
		if ((lseek(d->relfp, pageid * PGSIZE, 0) < 0) ||
		    ((read(d->relfp, b, PGSIZE)) != PGSIZE))
		{
			resetacc(b);
			return (acc_err(AMREAD_ERR));
		}
		Accuread++;
		if (d->relstat & S_CATALOG)
		{
			Accusread++;
		}
#		ifdef xATM
		if (tTf(76, 1))
			timtrace(18, &pageid, &d->reltid);
#		endif
	}
	return (0);
}

/* tTf flag 84	TTF	pageflush() */

pageflush(buf)
struct accbuf	*buf;
{
	register struct accbuf	*b;
	register int		i, allbufs;
	int			err;

	b = buf;
#	ifdef xATR3
	if (tTf(84, 0))
		printf("PAGEFLUSH: (%u=%s)\n", b, locv(b->rel_tupid));
#	endif
	err = FALSE;
	allbufs = FALSE;
	if (b == 0)
	{
		b = Acc_buf;
		allbufs = TRUE;
	}

	do
	{
		if (b->bufstatus & BUF_DIRTY)
		{
#			ifdef xATR1
			if (tTf(84, 1))
			{
				printf("PAGEFLUSH: wr pg %s", locv(b->thispage));
				printf(",relid %s,stat %d\n", locv(b->rel_tupid), b->bufstatus);
			}
#			endif

#			ifdef xATM
			if (tTf(76, 1))
				timtrace(19, &b->thispage, &b->rel_tupid);
#			endif

			/* temp debug step */
			if (b->mainpg > 32767)
			{
				printf("rel %s:", locv(b->rel_tupid));
				printf("pg %s:", locv(b->thispage));
				syserr("pgflush mainpg %s", locv(b->mainpg));
			}
			b->bufstatus &= ~BUF_DIRTY;
			if (lseek(b->filedesc, b->thispage * PGSIZE, 0) < 0 ||
			    (write(b->filedesc, b, PGSIZE) != PGSIZE))
			{
				resetacc(b);
				err = TRUE;
			}
			Accuwrite++;

#			ifdef xATM
			if (tTf(76, 1))
				timtrace(20, &b->thispage, &b->rel_tupid);
#			endif
		}
		if (Acclock && b->bufstatus & BUF_LOCKED)
			unlpg(b);

	} while (allbufs && (b = b->modf) != NULL);

	if (err)
		return (acc_err(AMWRITE_ERR));

	return (0);
}

/*
**	ACC_ERR -- set global error indicator "Accerror"
*/

/* tTf flag 85	TTF	acc_err() */

acc_err(errnum)
int	errnum;
{
	Accerror = errnum;
#	ifdef xATR1
	tTfp(85, 0, "ACC_ERR: %d\n", Accerror);
#	endif
	return (Accerror);
}

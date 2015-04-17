# include	"../ingres.h"
# include	"../access.h"

/*
**	INSERT - add a new tuple to a relation
**
**	Insert puts a given tuple into a relation in
**	the "correct" position.
**
**	If insert is called with checkdups == TRUE then
**	the tuple will not be inserted if it is a duplicate
**	of some already existing tuple. If the relation is a
**	heap then checkdups is made false.
**
**	Tid will be set to the tuple id where the
**	tuple is placed.
**
**	returns:
**		<0  fatal eror
**		0   success
**		1   tuple was a duplicate
*/

insert(desc, tid1, tuple, checkdups)
struct descriptor	*desc;
struct tup_id		*tid1;
char			*tuple;
int			checkdups;
{
	register struct descriptor	*d;
	register struct tup_id		*tid;
	register int			i;
	int				need;

	d = desc;
	tid = tid1;

#	ifdef xATR1
	if (tTf(94, 0))
	{
		printf("insert:%.14s,", d->relid);
		dumptid(tid);
		printup(d, tuple);
	}
#	endif
	/* determine how much space is needed for tuple */
	need = canonical(d, tuple);

	/* find the "best" page to place tuple */
	if (i = findbest(d, tid, tuple, need, checkdups))
		return (i);

	/* put tuple in position "tid" */
	put_tuple(tid, Acctuple, need);

	d->reladds++;

	return (0);
}

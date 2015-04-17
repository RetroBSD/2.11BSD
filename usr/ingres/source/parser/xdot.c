# include	"../ingres.h"
# include	"../aux.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"../symbol.h"
# include	"parser.h"

/*
** XDOT
**	add to attribute stash any missing attributes in the
**	source relation and then build tree with all attribs
**	in the 'attid' order.  This algorithm assumes that
**	the function 'attadd' insert attributes into the list
**	in 'attid' order from 1 -> N.
*/
struct querytree *
xdot(rptr)
struct rngtab	*rptr;
{
	struct attribute		tuple;
	register struct attribute	*ktuple;
	struct attribute		ktup;
	struct tup_id			tid;
	struct tup_id			limtid;
	struct querytree		*tempt;
	register struct querytree	*vnode;
	int				ik;
	register struct atstash		*aptr;
	struct querytree		*tree();
	struct querytree		*addresdom();

#	ifdef	xPTR2
	tTfp(15, 0, "ALL being processed for %s\n", rptr->relnm);
#	endif

	/* if attstash is missing any attribs then fill in list */
	if (rptr->ratts != attcount(rptr))
	{
		/* get all entries in attrib relation */
		clearkeys(&Desc);
		ktuple = &ktup;
		setkey(&Desc, ktuple, rptr->relnm, ATTRELID);
		setkey(&Desc, ktuple, rptr->relnowner, ATTOWNER);
		if (ik = find(&Desc, EXACTKEY, &tid, &limtid, ktuple))
			syserr("bad find in xdot '%d'", ik);
		while (!get(&Desc, &tid, &limtid, &tuple, 1))
			if (!kcompare(&Desc, &tuple, ktuple))
				/* add any that are not in the stash */
				if (!attfind(rptr, tuple.attname))
					attadd(rptr, &tuple);
	}

	/* build tree for ALL */
	tempt = NULL;
	aptr = rptr->attlist;
	while (aptr != 0)
	{
		vnode = tree(NULL, NULL, VAR, 6, rptr->rentno, aptr);
		Trname = aptr->atbname;
		tempt = addresdom(tempt, vnode);
		aptr = aptr->atbnext;
	}
	return(tempt);
}

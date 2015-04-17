# include	"../ingres.h"
# include	"../aux.h"
# include	"../catalog.h"
# include	"../access.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"qrymod.h"

/*
**  INTEGRITY.C -- Integrity Constraint Processor
**
**	This module contains the integrity constraint processor.  This
**	processor modifies the query to add integrity constraints.
**
**	Currently only single-variable aggregate-free constraints are
**	handled.  Thus the algorithm is reduced to scanning the tree
**	for each variable modified and appending the constraints for
**	that variable to the tree.
**
**	Parameters:
**		none
**
**	Returns:
**		The root of the modified tree.
**
**	Side Effects:
**		Much relation I/O.
**		Modifies the tree in place, so the previous one is
**			lost.
**
**	Defines:
**		integrity()
**
**	Requires:
**		lsetbit -- to create domain sets.
**		opencatalog, setkey, find, get -- to read integrity
**			catalog.
**		appqual -- to create new qualifications.
**		subsvars -- to change the simple VAR nodes in the
**			integrity qualifications into the afcn they
**			will become after the update.
**
**	Required By:
**		qrymod.c
**
**	Files:
**		integrity relation
**
**	Trace Flags:
**		40 -> 49
**
**	Diagnostics:
**		none
**
**	History:
**		2/14/79 -- version 6.2 released.
*/

extern struct descriptor	Intdes;

QTREE *integrity(root)
QTREE	*root;
{
	register QTREE		*r;
	int			dset[8];
	register QTREE		*p;
	register int		i;
	auto QTREE		*iqual;
	struct integrity	inttup, intkey;
	struct tup_id		hitid, lotid;
	QTREE			*gettree(), *norml();

#	ifdef xQTR1
	tTfp(40, -1, "\n->INTEGRITY\n");
#	endif

	r = root;

	/*
	**  Check to see if we should apply the integrity
	**  algorithm.
	**
	**  This means checking to insure that we have an update
	**  and seeing if any integrity constraints apply.
	*/

	if (Qmode == mdRETR || (Rangev[Resultvar].rstat & S_INTEG) == 0)
	{
#		ifdef xQTR2
		tTfp(40, 0, "->INTEGRITY: no integ\n");
#		endif
		return(r);
	}

	/*
	**  Create a set of the domains updated in this query.
	*/

	for (i = 0; i < 8; i++)
		dset[i] = 0;
	for (p = r->left; p != NULL && p->sym.type != TREE; p = p->left)
	{
#		ifdef xQTR3
		if (p->sym.type != RESDOM)
			syserr("integrity: RESDOM %d", p->sym.type);
#		endif
		lsetbit(((struct qt_res *)p)->resno, dset);
	}

#	ifdef xQTR3
	if (p == NULL)
		syserr("integrity: NULL LHS");
#	endif
#	ifdef xQTR1
	if (tTf(40, 1))
		pr_set(dset, "dset");
#	endif
	
	/*
	**  Scan integrity catalog for possible tuples.  If found,
	**  include them in the integrity qualification.
	*/

	iqual = NULL;
	opencatalog("integrities", 0);
	setkey(&Intdes, &intkey, Rangev[Resultvar].relid, INTRELID);
	setkey(&Intdes, &intkey, Rangev[Resultvar].rowner, INTRELOWNER);
	find(&Intdes, EXACTKEY, &lotid, &hitid, &intkey);

	while ((i = get(&Intdes, &lotid, &hitid, &inttup, TRUE)) == 0)
	{
		if (kcompare(&Intdes, &intkey, &inttup) != 0)
			continue;

#		ifdef xQTR1
		if (tTf(40, 2))
			printup(&Intdes, &inttup);
#		endif
		
		/* check for some domain set overlap */
		for (i = 0; i < 8; i++)
			if ((dset[i] & inttup.intdomset[i]) != 0)
				break;
		if (i >= 8)
			continue;
		
		/* some domain matches, include in integrity qual */
		i = Resultvar;
		p = gettree(Rangev[i].relid, Rangev[i].rowner, mdINTEG, inttup.inttree, FALSE);
#		ifdef xQTR1
		if (tTf(40, 3))
			treepr(p, "int_qual");
#		endif

		/* trim off (null) target list */
		p = p->right;

		/* merge the 'integrity' var into the Resultvar */
		i = inttup.intresvar;
		if (Remap[i] >= 0)
			i = Remap[i];
		mergevar(i, Resultvar, p);

		/* remove old integrity var */
		Rangev[i].rused = FALSE;

		/* add to integrity qual */
		if (iqual == NULL)
			iqual = p;
		else
			appqual(p, iqual);
	}
	if (i < 0)
		syserr("integrity: get %d", i);
	
	/*
	**  Clean up the integrity qualification so that it will merge
	**  nicely into the tree, and then append it to the user's
	**  qualification.
	*/

	if (iqual != NULL)
	{
		/* replace VAR nodes by corresponding user afcn */
		subsvars(&iqual, Resultvar, r->left, Qmode);

		/* append to tree and normalize */
		appqual(iqual, r);
#		ifdef xQTR3
		if (tTf(40, 8))
			treepr(r, "Unnormalized tree");
#		endif
		r->right = norml(trimqlend(r->right));
	}

#	ifdef xQTR1
	if (tTf(40, 15))
		treepr(r, "INTEGRITY->");
#	endif

	return (r);
}

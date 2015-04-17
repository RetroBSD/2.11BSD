# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"../catalog.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"qrymod.h"


/*
**  D_INTEG -- define integrity constraint
**
**	An integrity constraint (as partially defined by the last
**	tree defined by d_tree) is defined.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		Activity in 'relation' and 'integrities' catalogs.
**
**	Requires:
**		issueinvert -- to check if the qualification is
**			initially satisfied.
**
**	Called By:
**		define
**
**	Trace Flags:
**		49
**
**	Diagnostics:
**		3490 -- there are aggregates in the qualification.
**		3491 -- multivariable constraint.
**
**	Syserrs:
**		On inconsistancies.
**
**	History:
**		2/19/79 (eric) -- broken off from define.c.
*/

extern QTREE			*Treeroot;
extern struct descriptor	Intdes;
extern struct descriptor	Reldes;
extern struct pipfrmt		Pipe;



d_integ()
{
	register int		i;
	register QTREE		*t;		/* definition tree */
	struct integrity	inttup;
	struct tup_id		tid;
	register int		rv;		/* result variable */
	struct retcode		*rc;
	extern struct retcode	*issueinvert();
	struct relation		relkey;
	struct relation		reltup;

	t = Treeroot;
	Treeroot = NULL;
	rv = Resultvar;

	/* pipe is not used, clear it out */
	rdpipe(P_SYNC, &Pipe, R_up);

	/*
	**  Check for valid environment.
	**	The tree must exist, have a qualification, and have
	**	no target list.  The query mode must be mdDEFINE.
	**
	**	User level stuff checks to see that this is single
	**	variable aggregate free, since that's all we know
	**	about thusfar.  Also, the relation in question must
	**	not be a view.
	*/

#	ifdef xQTR3
	if (t == NULL)
		syserr("d_integ: NULL tree");
	if ((i = t->right->sym.type) != AND)
		syserr("d_integ: qual %d", i);
	if ((i = t->left->sym.type) != TREE)
		syserr("d_integ: TL %d", i);
	if (Qmode != mdDEFINE)
		syserr("d_integ: Qmode %d", Qmode);
#	endif
	
	/* check for aggregates */
	if (aggcheck(t))
		ferror(3490, -1, rv, 0);	/* aggregates in qual */

	/* check for multi-variable */
	for (i = 0; i < MAXVAR + 1; i++)
	{
		if (!Rangev[i].rused)
			continue;
		if (i != rv)
		{
#			ifdef xQTR3
			if (tTf(49, 1))
				printf("d_integ: Rv %d(%.14s) i %d(%.14s)\n",
				    rv, Rangev[rv].relid, i, Rangev[i].relid);
#			endif
			ferror(3491, -1, rv, 0);	/* too many vars */
		}
	}

	/* check for the resultvariable being a real relation */
	if ((Rangev[rv].rstat & S_VIEW) != 0)
		ferror(3493, -1, rv, 0);	/* is a view */
	
	/* guarantee that you own this relation */
	if (!bequal(Usercode, Rangev[rv].rowner, 2))
		ferror(3494, -1, rv, 0);	/* don't own reln */

	/*
	**  Guarantee that the integrity constraint is true now.
	**	This involves issuing a retrieve statement for the
	**	inverse of the qualification.  The target list is
	**	already null, so we will get nothing printed out
	**	(only a return status).
	*/

	Qmode = mdRETR;
	Resultvar = -1;

	/* issue the invert of the query */
	rc = issueinvert(t);
	if (rc->rc_tupcount != 0)
		ferror(3492, -1, rv, 0);	/* constraint not satisfied */

	/*
	**  Set up the rest of the environment.
	*/

	opencatalog("integrities", 2);
	clr_tuple(&Intdes, &inttup);
	Resultvar = -1;
	Qmode = -1;

	/*
	**  Set up integrity relation tuple.
	**	The qualification will be scanned, and a set of
	**	domains referenced will be created.  Other stuff
	**	is filled in from the range table and from the
	**	parser.
	**
	**	The tree is actually inserted into the tree catalog
	**	in this step.  Extra information is cleared here.
	*/

	inttup.intresvar = rv;
	bmove(Rangev[rv].relid, inttup.intrelid, MAXNAME);
	bmove(Rangev[rv].rowner, inttup.intrelowner, 2);
	makeidset(rv, t, inttup.intdomset);
	inttup.inttree = puttree(t, inttup.intrelid, inttup.intrelowner, mdINTEG);

	/*
	**  Insert tuple into integrity catalog.
	*/

	i = insert(&Intdes, &tid, &inttup, FALSE);
	if (i < 0)
		syserr("d_integ: insert");
	if (noclose(&Intdes) != 0)
		syserr("d_integ: noclose int");

	/*
	**  Update relstat S_INTEG bit.
	*/

	if ((Rangev[rv].rstat & S_INTEG) == 0)
	{
		opencatalog("relation", 2);
		setkey(&Reldes, &relkey, inttup.intrelid, RELID);
		setkey(&Reldes, &relkey, inttup.intrelowner, RELOWNER);
		i = getequal(&Reldes, &relkey, &reltup, &tid);
		if (i != 0)
			syserr("d_integ: geteq");
		reltup.relstat |= S_INTEG;
		i = replace(&Reldes, &tid, &reltup, FALSE);
		if (i != 0)
			syserr("d_integ: replace");
		if (noclose(&Reldes) != 0)
			syserr("d_integ: noclose rel");
	}
}


makeidset(varno, tree, dset)
int	varno;
QTREE	*tree;
int	dset[8];
{
	register int	vn;
	register QTREE	*t;

	vn = varno;
	t = tree;

	while (t != NULL)
	{
		if (t->sym.type == VAR && ((struct qt_var *)t)->varno == vn)
			lsetbit(((struct qt_var *)t)->attno, dset);
		
		/* handle left subtree recursively */
		makeidset(vn, t->left, dset);

		/* handle right subtree iteratively */
		t = t->right;
	}
}

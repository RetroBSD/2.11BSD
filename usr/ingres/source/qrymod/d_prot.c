# include	"../ingres.h"
# include	"../aux.h"
# include	"../unix.h"
# include	"../catalog.h"
# include	"../access.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"../lock.h"
# include	"qrymod.h"



/*
**  D_PROT -- define protection constraint
**
**	A protection constraint as partially defined by the last tree
**	defined by d_tree is defined.
**
**	The stuff that comes through the pipe as parameters is complex.
**	It comes as a sequence of strings:
**	 # The operation set, already encoded in the parser into a
**	   bit map.  If the PRO_RETR permission is set, the PRO_TEST
**	   and PRO_AGGR permissions will also be set.
**	 # The relation name.
**	 # The relation owner.
**	 # The user name.  This must be a user name as specified in
**	   the 'users' file, or the keyword 'all', meaning all users.
**	 # The terminal id.  Must be a string of the form 'ttyx' or
**	   the keyword 'all'.
**	 # The starting time of day, as minutes-since-midnight.
**	 # The ending time of day.
**	 # The starting day-of-week, with 0 = Sunday.
**	 # The ending dow.
**
**	The domain reference set is build automatically from the
**	target list of the tree.  Thus, the target list must exist,
**	but it is not inserted into the tree.  The target list must
**	be a flat sequence of RESDOM nodes with VAR nodes hanging
**	of the rhs; also, the VAR nodes must all be for Resultvar.
**	If there is no target list on the tree, the set of all var-
**	iables is assumed.
**
**	The relstat field in the relation relation is updated to
**	reflect any changes.
**
**	It only makes sense for the DBA to execute this command.
**
**	If there is one of the special cases
**		permit all to all
**		permit retrieve to all
**	it is caught, and the effect is achieved by diddling
**	relstat bits instead of inserting into the protect catalog.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		Activity in 'protect' and 'relation' catalogs.
**
**	Requires:
**		Prodes, Reldes -- descriptors.
**		opencatalog -- to open these relations.
**		puttree -- to output the tree to the 'tree'
**			relation.
**		insert -- to output the tuple to the 'protect'
**			relation.
**		noclose -- to flush the 'protect' relation.
**		Rangev, Qmode, Resultvar -- for various parameter
**			validation.
**		Treeroot -- a pointer to the protection tree.
**
**	Called By:
**		define
**
**	Trace Flags:
**		59
**
**	Diagnostics:
**		3590 -- bad terminal id.
**		3591 -- bad user name.
**		3592 -- you do not own one of the relations involved.
**		3593 -- attempt to define a protection constraint
**			on a view.
**		3594 -- you are not the dba.
**
**	Syserrs:
**		On inconsistancies between pipe info and system
**		catalogs.
**
**	History:
**		2/19/79 (eric) -- split from define.c.
*/

extern struct admin		Admin;
extern struct descriptor	Prodes;
extern struct descriptor	Reldes;
extern QTREE			*Treeroot;

d_prot()
{
	struct protect	protup;
	struct tup_id	protid;
	struct protect	prokey;
	struct protect	proxtup;
	register int	i;
	auto int	ix;
	int		treeid;
	register QTREE	*t;
	register char	*p;
	struct relation	reltup;
	struct relation	relkey;
	struct tup_id	reltid;
	int		relstat;
	int		all_pro;

	/*
	**  Check for valid tree:
	**	There must be a tree defined, and all variables
	**	referenced must be owned by the current user; this
	**	is because you could otherwise get at data by
	**	mentioning it in a permit statement; see protect.c
	**	for a better explanation of this.
	*/

	if (Treeroot == NULL)
		syserr("d_prot: NULL Treeroot");
	for (i = 0; i < MAXVAR + 1; i++)
	{
		if (!Rangev[i].rused)
			continue;
		if (!bequal(Rangev[i].rowner, Usercode, 2))
			ferror(3592, -1, i, 0);
	}

	/* test for dba */
	if (!bequal(Usercode, Admin.adhdr.adowner, 2))
		ferror(3595, -1, Resultvar, 0);
	
	clr_tuple(&Prodes, &protup);

	all_pro = fillprotup(&protup);
	
	/* get domain reference set from target list */
	/* (also, find the TREE node) */
	t = Treeroot->left;
	if (t->sym.type == TREE)
	{
		for (i = 0; i < 8; i++)
			protup.prodomset[i] = -1;
	}
	else
	{
		for (i = 0; i < 8; i++)
			protup.prodomset[i] = 0;
		for (; t->sym.type != TREE; t = t->left)
		{
			if (t->right->sym.type != VAR ||
			    t->sym.type != RESDOM ||
			    ((struct qt_var *)t->right)->varno != Resultvar)
				syserr("d_prot: garbage tree");
			lsetbit(((struct qt_var *)t->right)->attno, protup.prodomset);
		}
		all_pro = FALSE;
	}

	/* trim off the target list, since it isn't used again */
	Treeroot->left = t;

	/*
	**  Check out the target relation.
	**	We first save the varno of the relation which is
	**	getting the permit stuff.  Also, we check to see
	**	that the relation mentioned is a base relation,
	**	and not a view, since that tuple would never do
	**	anything anyway.  Finally, we clear the Resultvar
	**	so that it does not get output to the tree catalog.
	**	This would result in a 'syserr' when we tried to
	**	read it.
	*/

	protup.proresvar = Resultvar;
#	ifdef xQTR3
	if (Resultvar < 0)
		syserr("d_prot: Rv %d", Resultvar);
#	endif
	if ((Rangev[Resultvar].rstat & S_VIEW) != 0)
		ferror(3593, -1, Resultvar, 0);	/* is a view */

	/* clear the (unused) Qmode */
#	ifdef xQTR3
	if (Qmode != mdDEFINE)
		syserr("d_prot: Qmode %d", Qmode);
#	endif
	Qmode = -1;

	/*
	**  Check for PERMIT xx to ALL case.
	**	The relstat bits will be adjusted as necessary
	**	to reflect these special cases.
	**
	**	This is actually a little tricky, since we cannot
	**	afford to turn off any permissions.  If we already
	**	have some form of PERMIT xx to ALL access, we must
	**	leave it.
	*/

	relstat = Rangev[Resultvar].rstat;
	if (all_pro && (protup.proopset & PRO_RETR) != 0)
	{
		if (protup.proopset == -1)
			relstat &= ~S_PROTALL;
		else
		{
			relstat &= ~S_PROTRET;
			if ((protup.proopset & ~(PRO_RETR|PRO_AGGR|PRO_TEST)) != 0)
			{
				/* some special case: still insert prot tuple */
				all_pro = FALSE;
			}
		}
	}
	else
		all_pro = FALSE;	/* insert tuple if not special case */

	/* see if we are adding any tuples */
	if (!all_pro)
		relstat |= S_PROTUPS;
	
	/*
	**  Change relstat field in relation catalog
	*/

	opencatalog("relation", 2);
	setkey(&Reldes, &relkey, Rangev[Resultvar].relid, RELID);
	setkey(&Reldes, &relkey, Rangev[Resultvar].rowner, RELOWNER);
	i = getequal(&Reldes, &relkey, &reltup, &reltid);
	if (i != 0)
		syserr("d_prot: geteq %d", i);
	reltup.relstat = relstat;
	i = replace(&Reldes, &reltid, &reltup, FALSE);
	if (i != 0 && i != 1)
		syserr("d_prot: repl %d", i);
	if (noclose(&Reldes) != 0)
		syserr("d_prot: noclose(rel)");

	Resultvar = -1;

	if (!all_pro)
	{
		/*
		**  Output the created tuple to the protection catalog
		**  after making other internal adjustments and deter-
		**  mining a unique sequence number (with the protect
		**  catalog locked).
		*/

		if (Treeroot->right->sym.type != QLEND)
			protup.protree = puttree(Treeroot, protup.prorelid, protup.prorelown, mdPROT);
		else
			protup.protree = -1;

		/* compute unique permission id */
		opencatalog("protect", 2);
		setrll(A_SLP, Prodes.reltid, M_EXCL);
		setkey(&Prodes, &prokey, protup.prorelid, PRORELID);
		setkey(&Prodes, &prokey, protup.prorelown, PRORELOWN);
		for (ix = 2; ; ix++)
		{
			setkey(&Prodes, &prokey, &ix, PROPERMID);
			i = getequal(&Prodes, &prokey, &proxtup, &protid);
			if (i < 0)
				syserr("d_prot: geteq");
			else if (i > 0)
				break;
		}
		protup.propermid = ix;

		/* do actual insert */
		i = insert(&Prodes, &protid, &protup, FALSE);
		if (i < 0)
			syserr("d_prot: insert");
		if (noclose(&Prodes) != 0)
			syserr("d_prot: noclose(pro)");
		
		/* clear the lock */
		unlrl(Prodes.reltid);
	}
	Treeroot = NULL;
}





/*
**  CVT_DOW -- convert day of week
**
**	Converts the day of the week from string form to a number.
**
**	Parameters:
**		sdow -- dow in string form.
**
**	Returns:
**		0 -> 6 -- the encoded day of the week.
**		-1 -- error.
**
**	Side Effects:
**		none
**
**	Defines:
**		Dowlist -- a mapping from day of week to number.
**		cvt_dow
**
**	Called By:
**		d_prot
*/

struct downame
{
	char	*dow_name;
	int	dow_num;
};

struct downame	Dowlist[] =
{
	"sun",		0,
	"sunday",	0,
	"mon",		1,
	"monday",	1,
	"tue",		2,
	"tues",		2,
	"tuesday",	2,
	"wed",		3,
	"wednesday",	3,
	"thu",		4,
	"thurs",	4,
	"thursday",	4,
	"fri",		5,
	"friday",	5,
	"sat",		6,
	"saturday",	6,
	NULL
};

cvt_dow(sdow)
char	*sdow;
{
	register struct downame	*d;
	register char		*s;

	s = sdow;

	for (d = Dowlist; d->dow_name != NULL; d++)
		if (sequal(d->dow_name, s))
			return (d->dow_num);
	return (-1);
}

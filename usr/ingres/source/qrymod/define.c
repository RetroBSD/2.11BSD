# include	"../ingres.h"
# include	"../catalog.h"
# include	"../symbol.h"
# include	"../lock.h"
# include	"../pipes.h"
# include	"qrymod.h"

/*
**  DEFINE -- define various types of qrymod constraints
**
**	This module takes care of defining all the various types of
**	constraints that can be presented to qrymod.  It makes sense
**	out of them by looking at the funcid in the pipe.  They
**	correspond as follows:
**
**	mdDEFINE -- a tree.  This tree doesn't have any meaning yet; it is
**		just the raw tree.  It must be immediately followed
**		by a type 1, 2, or three definition, which will link
**		that tree into some other catalog, thereby giving
**		it some sort of semantic meaning.
**	mdVIEW -- a view.  This block has the view name in it.
**	mdPROT -- a protection constraint.
**	mdINTEG -- an integrity constraint.
**
**	Defines:
**		define -- the driver: just calls one of the others.
**		d_tree -- read definition tree.
**		puttree -- write tree into tree catalog.
**		relntrwr -- the physical write routine for puttree.
**
**	Requires:
**		Prodes -- ditto for protection relation.
**		Intdes -- ditto for integrities relation.
**		Treedes -- ditto for tree relation.
**
**	Required By:
**		main
**
**	History:
**		2/19/79 (eric) -- split into four files.
**		2/14/79 -- version 6.2, released.
*/





QTREE				*Treeroot;
/*
**  DEFINE -- define driver
**
**	This function does very little exciting; in fact, it just
**	calls one of the other four functions to do the specific
**	funcid functions.
**
**	Parameters:
**		func -- the function to perform:
**			mdDEFINE -- define tree (used by all).
**			mdVIEW -- define view.
**			mdPROT -- define protection constraint.
**			mdINTEG -- define integrity constraint.
**
**	Returns:
**		none
**
**	Side Effects:
**		none
**
**	Requires:
**		d_tree
**		d_view
**		d_prot
**		d_integ
**
**	Called By:
**		main
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
**		bad arg %c -- a bad argument was passed (not mdDEFINE,
**			mdVIEW, mdPROT, mdINTEG).
*/

extern struct pipfrmt	Pipe;


define(func)
char	func;
{
	switch (func)
	{
	  case mdDEFINE:
		d_tree();
		break;

	  case mdVIEW:
		d_view();
		break;

	  case mdPROT:
		d_prot();
		break;

	  case mdINTEG:
		d_integ();
		break;

	  default:
		syserr("define: bad arg %d", func);
	}
}
/*
**  D_TREE -- insert tree into system catalogs
**
**	This routine reads in and saves a tree for further use.
**	The root of this tree is saved in the global 'Treeroot'.
**	The tree will ultimately be written to the 'tree' catalog
**	using the 'puttree' routine.
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		The input pipe is read and the tree found there
**		is built.  A pointer to that tree is saved in
**		'Treeroot'.  Notice that 'Qbuf' and the range
**		table are clobbered.
**
**	Requires:
**		pipetrrd().
**		readqry().
**
**	Trace Flags:
**		10
*/

d_tree()
{
	extern		pipetrrd();
	QTREE		*readqry();

	Treeroot = readqry(&pipetrrd, TRUE);
	rdpipe(P_SYNC, &Pipe, R_up);
#	ifdef xQTR1
	if (tTf(10, 0))
		treepr(Treeroot, "Treeroot");
#	endif
}
/*
**  PUTTREE -- put tree into 'tree' catalog
**
**	The named tree is inserted into the 'tree' catalog.
**
**	The algorithm is to lock up the entire catalog and try to
**	find the smallest unique id possible for the named relation.
**
**	Parameters:
**		root -- the root of the tree to insert.
**		treerelid -- the relid of the relation for which
**			this tree applies.
**		treeowner -- the owner of the above relation.
**		treetype -- the type of this tree; uses the mdXXX
**			type (as mdPROT, mdINTEG, mdDISTR, etc.).
**
**	Returns:
**		The treeid that was assigned to this tree.
**
**	Side Effects:
**		The tree catalog gets locked, and information is
**		inserted.
**
**	Requires:
**		Treedes -- a relation descriptor for the tree
**			catalog (also needed by gettrseg).
**		insert -- to insert tuples.
**		noclose -- to bring 'tree' up to date after mod.
**		writeqry -- to write the tree.
**		relntrwr -- to do the physical writes to the catalog.
**		getequal -- to test uniqueness of tree id's.
**		setrll -- to set the exclusive relation lock.
**		unlrl -- to unlock same.
**
**	Called By:
**		d_view
**		d_integ
**		d_prot
**
**	Trace Flags:
**		10
**
**	Diagnostics:
**		none
**
*/

puttree(root, treerelid, treeowner, treetype)
QTREE	*root;
int	treetype;
{
	struct tree	treekey;
	struct tree	treetup;
	struct tup_id	treetid;
	register int	i;
	auto int	treeid;

	opencatalog("tree", 2);

	/*
	**  Find a unique tree identifier.
	**	Lock the tree catalog, and scan until we find a
	**	tuple which does not match.
	*/

	setrll(A_SLP, Treedes.reltid, M_EXCL);

	setkey(&Treedes, &treekey, treerelid, TREERELID);
	setkey(&Treedes, &treekey, treeowner, TREEOWNER);
	setkey(&Treedes, &treekey, &treetype, TREETYPE);
	for (treeid = 0;; treeid++)
	{
		setkey(&Treedes, &treekey, &treeid, TREEID);
		i = getequal(&Treedes, &treekey, &treetup, &treetid);
		if (i < 0)
			syserr("d_tree: getequal");
		else if (i > 0)
			break;
	}

	/*
	**  We have a unique tree id.
	**	Insert the new tuple and the tree into the
	**	tree catalog.
	*/

	relntrwr(NULL, 0, treerelid, treeowner, treetype, treeid);
	writeqry(root, &relntrwr);
	relntrwr(NULL, 1);

	/* all inserted -- flush pages and unlock */
	if (noclose(&Treedes) != 0)
		syserr("d_tree: noclose");
	unlrl(Treedes.reltid);

	return(treeid);
}
/*
**  RELNTRWR -- physical tree write to relation
**
**	This is the routine called from writeqry to write trees
**	to the 'tree' relation (rather than the W_down pipe).
**
**	It is assumed that the (treerelid, treeowner, treetype,
**	treeid) combination is unique in the tree catalog, and that
**	the tree catalog is locked.
**
**	Parameters:
**		ptr -- a pointer to the data.  If NULL, this is
**			a control call.
**		len -- the length of the data.  If ptr == NULL, this
**			field is a control code:  zero means
**			initialize (thus taking the next two param-
**			eters); one means flush.
**		treerelid -- the name of the relation for which this
**			tree applies (init only).
**		treeowner -- the owner of this relation (init only).
**		treetype -- on initialization, this tells what the
**			tree is used for.
**		treeid -- on initialization, this is the tree id we
**			want to use.
**
**	Returns:
**		The number of bytes written ('len').
**
**	Side Effects:
**		Well, yes.  Activity occurs in the tree catalog.
**
**	Requires:
**		insert -- to insert tuples.
**		Treedes -- open for read/write.
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
*/

relntrwr(ptr, len, treerelid, treeowner, treetype, treeid)
char	*ptr;
int	len;
char	*treerelid;
char	*treeowner;
int	treetype;
int	treeid;
{
	static struct tree	treetup;
	struct tup_id		treetid;
	register char		*p;
	register int		l;
	static char		*tptr;

	p = ptr;
	l = len;

	/* check for special function */
	if (p == NULL)
	{
		switch (l)
		{
		  case 0:
			clr_tuple(&Treedes, &treetup);
			bmove(treerelid, treetup.treerelid, MAXNAME);
			bmove(treeowner, treetup.treeowner, 2);
			treetup.treetype = treetype;
			treetup.treeid = treeid;
			tptr = treetup.treetree;
			break;

		  case 1:
			if (tptr != treetup.treetree)
			{
				if (insert(&Treedes, &treetid, &treetup, FALSE) < 0)
					syserr("relntrwr: insert 1");
			}
			break;
		
		  default:
			syserr("relntrwr: ctl %d", l);
		}
		return;
	}

	/* output bytes */
	while (l-- > 0)
	{
		*tptr++ = *p++;

		/* check for buffer overflow */
		if (tptr < &treetup.treetree[sizeof treetup.treetree])
			continue;
		
		/* yep, flush buffer to relation */
		if (insert(&Treedes, &treetid, &treetup, FALSE) < 0)
			syserr("relntrwr: insert 2");
		treetup.treeseq++;
		tptr = treetup.treetree;

		/* clear out the rest of the tuple for aesthetic reasons */
		*tptr = ' ';
		bmove(tptr, tptr + 1, sizeof treetup.treetree - 1);
	}

	return (len);
}

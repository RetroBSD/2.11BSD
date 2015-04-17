# include	"../ingres.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"qrymod.h"

/*
**  TRSCAN -- scan trees for various conditions
**
**	These are basically utility routines which do tree scans for
**	one thing or another.  Really, there is nothing exciting at
**	all in this module.
**
**	Defines:
**		aggcheck -- scan tree for aggregates
**		vfind -- find definition for VAR in a view tree
**		qscan -- scan qualification for a variable
**
**	Requires:
**		-
**
**	Required By:
**		MANY
**
**	Compilation Flags:
**		14
**
**	Trace Flags:
**		14
**
**	History:
**		2/14/79 -- version 6.2/0 release.
**		3/22/78 (eric) -- written
*/
/*
**  AGGCHECK -- check for any aggregate in subtree.
**
**	This routine checks to insure that the view algorithm can
**	proceed safely by checking for aggregates in the view tree.
**
**	Parameters:
**		root -- the root of the tree to check.
**
**	Returns:
**		TRUE -- an aggregate found.
**		FALSE -- no aggregates in tree.
**
**	Side Effects:
**		none
**
**	Called By:
**		view
**
**	Trace Flags:
**		14
*/

aggcheck(root)
QTREE	*root;
{
	register QTREE	*t;

	t = root;

	/* check for no pointer */
	while (t != NULL)
	{
		/* check for this node an AGHEAD */
		if (t->sym.type == AGHEAD)
			return (TRUE);

		/* check left subtree recursively */
		if (aggcheck(t->left))
			return (TRUE);
		
		/* check right subtree iteratively */
		t = t->right;
	}

	return (FALSE);
}
/*
**  VFIND -- find definition for attribute in view tree
**
**	The view tree is scanned for a specified RESDOM; a pointer
**	to the value is returned.
**
**	Parameters:
**		vn -- the variable number to dig out.
**		vtree -- a pointer to the target list of the
**			view definition tree.
**
**	Returns:
**		a pointer to the substitution value for the specified
**			'vn'.
**		NULL -- if not found.
**
**	Side Effects:
**		none
**
**	Requires:
**
**	Called By:
**		subsvars
**		vrscan
**
**	Trace Flags:
**		14
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		vfind: bad RESDOM %d -- a node was found which was not
**			a RESDOM node, nor a TREE node, in 'vtree'.
*/

QTREE *
vfind(vn, vtree)
int	vn;
QTREE	*vtree;
{
	register int	n;
	register QTREE	*v;

	n = vn;

	for (v = vtree; v->sym.type == RESDOM; v = v->left)
	{
		if (((struct qt_res *)v)->resno != n)
			continue;

		/* found the correct replacement */
		return (v->right);
	}

	if (v->sym.type != TREE)
		syserr("vfind: bad RESDOM node %d", v->sym.type);
	return (NULL);
}
/*
**  QSCAN -- find specified VAR node in subtree
**
**	Intended for finding a variable in a qualification, this
**	routine just scans a tree recursively looking for a node
**	with the specified VAR (varno.attno) node.
**
**	Parameters:
**		root -- the root of the tree to scan.
**		vn -- the varno to scan for.
**		an -- the attno to scan for.
**
**	Returns:
**		A pointer to the first found VAR node which matches.
**			Scan is prefix.
**		NULL if not found at all.
**
**	Side Effects:
**		none
**
**	Requires:
**		none
**
**	Called By:
**		vrscan
**
**	Trace Flags:
**		15
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

QTREE *
qscan(root, vn, an)
QTREE	*root;
int	vn;
int	an;
{
	register QTREE	*t;
	register QTREE	*u;

	t = root;

	/* check for null node */
	if (t == NULL)
		return (t);

	/* check to see if this node qualifies */
	if (t->sym.type == VAR && ((struct qt_var *)t)->varno == vn && ((struct qt_var *)t)->attno == an)
		return (t);

	/* check other nodes */
	if ((u = qscan(t->left, vn, an)) != NULL)
		return (u);
	return (qscan(t->right, vn, an));
}
/*
**  VARSET -- scan tree and set a bit vector of variables
**
**	The return value is a bit vector representing the set of
**	variables used in that subtree.
**
**	Parameters:
**		root -- the root of the tree to check.
**
**	Returns:
**		A bit vector, such that bit zero (on the low order,
**			right-hand end) means var zero.
**
**	Side Effects:
**		none
**
**	Requires:
**		none
**
**	Called By:
**		vrscan
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

varset(root)
QTREE	*root;
{
	register QTREE	*t;
	register int	s;

	t = root;

	if (t == NULL)
		return (0);

	/* scan left and right branches */
	s = varset(t->left);
	s |= varset(t->right);

	/* check out this node */
	if (t->sym.type == VAR)
	{
		/* or in bit corresponding to this varno */
		s |= 1 << ((struct qt_var *)t)->varno;
	}

	return (s);
}
/*
**  SUBSVARS -- scan query tree and replace VAR nodes
**
**	Scans a tree and finds all VAR nodes for this variable.
**	These nodes are looked up in the translation tree and
**	replaced by the value found there.  If this is for a
**	view, the corresponding node must exist in the translation
**	tree, otherwise, a 'zero' node (of a type appropriate based
**	on the context) is created and inserted.
**
**	This routine is one half of the guts of the whole view
**	algorithm.
**
**	VAR nodes are detached and replaced with the replacement
**	as defined by the view.  Note that there can never be any
**	problems here, since VAR nodes are only used in retrieve
**	contexts.
**
**	It does some extra processing with RESDOM nodes with
**	resno = 0.  These nodes specify a 'tid' domain, and are
**	included by the parser on REPLACE and DELETE commands
**	(for some reason decomp wants them).  Subsvars will allow
**	this construct iff the right hand pointer is a VAR node
**	with attno = 0.  In this case it just changes the varno
**	of the VAR node to be the Resultvar number.  This is be-
**	cause the Resultvar is the variable number of the one and
**	only underlying base relation of the view on an update
**	(which is presumably the only case where this can come
**	up).  Vrscan has already insured that there can only be
**	a single base relation in this case.
**
**	This whole messy thing is only done with view substitutions.
**
**	Parameters:
**		proot -- a pointer to the pointer to the root of the
**			tree to be updated.
**		vn -- the varno of the view variable.  This is the
**			varno which will be scanned for.
**		transtree -- a pointer to the left branch (target list)
**			of the translation tree.
**		vmode -- mdVIEW if called from view processor, mdAPP
**			if called from the integrity processor with
**			an APPEND command, else something else.
**			Mostly, changes the handling of TID type
**			nodes, and forces an error on a view if the
**			VAR node in the scanned tree does not exist
**			in the vtree.
**
**	Returns:
**		none
**		(non-local on error).
**
**	Side Effects:
**		The tree pointed to by *proot is updated in possibly
**			very exciting ways.
**
**	Requires:
**		Newresvar -- in the view case, the new result variable
**			number of the query.
**		treedup -- to dup the tree from vfind.
**		makezero -- to make a zero node when vmode == mdAPP.
**
**	Called By:
**		view
**
**	Trace Flags:
**		32
**
**	Diagnostics:
**		3340: views do not have tids -- the user has tried to
**			reference 'v.tid', where v is the range variable
**			for the view.  This can be determined un-
**			ambiguously if the view ranges over only one
**			relation, but is impossible otherwise; for the
**			time being it is impossible to reference the
**			'tid' field at all.
*/

subsvars(proot, vn, transtree, vmode)
QTREE	**proot;
int	vn;
QTREE	*transtree;
int	vmode;
{
	register QTREE	*t;
	register QTREE	*v;
	register int	i;
	extern QTREE	*vfind(), *makezero(), *treedup();
	extern int	Newresvar;	/* defined in view.c */

	t = *proot;
	v = transtree;

#	ifdef xQTR3
	if (tTf(32, 0))
		printf("subsvars: vn %d root %u transtree %u\n", vn, t, v);
#	endif

	if (t == NULL)
		return;

	/* check left branch of the tree */
	subsvars(&t->left, vn, v, vmode);

	/* check for special 'tid' RESDOM (used by DEL and REPL) */
	if (t->sym.type == RESDOM && ((struct qt_res *)t)->resno == 0)
	{
		/* test for not Resultvar, in which case we ignore leaf */
		if (vn != Resultvar)
			return;

		/* t->right better be VAR node, attno 0 */
		t = t->right;
		if (t->sym.type != VAR || ((struct qt_var *)t)->attno != 0 || ((struct qt_var *)t)->varno != vn)
			syserr("subsvars: RESDOM 0 not VAR 0 %d, %d, %d",
				vn, ((struct qt_var *)t)->attno, t->sym.type);
		
		/* change varno to new Newresvar (set by vrscan) */
#		ifdef xQTR3
		if (tTf(32, 1))
			printf("RESDOM 0: Newresvar %d\n", Newresvar);
#		endif
		((struct qt_var *)t)->varno = Newresvar;
		return;
	}

	/* scan right branch */
	subsvars(&t->right, vn, v, vmode);

	/* check for interesting node */
	if (t->sym.type != VAR || ((struct qt_var *)t)->varno != vn)
		return;

	/* test for special 'tid' attribute case */
	if (((struct qt_var *)t)->attno == 0 && vmode == mdVIEW)
	{
		ferror(3340, Qmode, vn, 0);	/* views do not have tids */
	}

	/* find var in vtree */
	v = vfind(((struct qt_var *)t)->attno, v);
	if (v == NULL)
	{
		if (vmode == mdVIEW)
			syserr("subsvars: attno %d", ((struct qt_var *)t)->attno);
		else if (vmode == mdAPP)
			v = makezero();
	}
	else
		v = treedup(v);

	/* replace VAR node */
	if (v != NULL)
		*proot = v;
}

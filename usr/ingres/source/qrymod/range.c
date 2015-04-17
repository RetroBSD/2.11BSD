# include	"../ingres.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"qrymod.h"


/*
**  RANGE.C -- range table manipulation
**
**	Defines:
**		declare -- declare variable
**		clrrange -- clear range table
**		mapvars -- map varno's in a tree to unique numbers
**		Remap[] -- a mapping from specified (preferred) varno's
**			to actual varno's.  -1 indicates no mapping.
**			Presumably this cannot be set when reading an
**			original query tree, but can happen easily when
**			reading a tree from 'tree' catalog.
**		Rangev[] -- the range table.
**
**	Required By:
**		main
**		treeio
**
**	Files:
**		none
**
**	Trace Flags:
**		11, 12
**
**	History:
**		2/14/79 -- version 6.2 release.
**		2/25/78 (eric) -- written
*/


int		Remap[MAXVAR + 1];
struct rngtab	Rangev[MAXVAR + 1];
/*
**  CLRRANGE -- clear range table(s)
**
**	The range table (Rangev) and range table map (Remap) are
**	initialized in one of two ways.
**
**	Parameters:
**		prim -- if TRUE, the primary range table (Rangev)
**			is cleared and Remap is set to be FULL (for
**			reading in an initial query).  If FALSE,
**			Rangev is untouched, but Remap is cleared.
**
**	Returns:
**		none
**
**	Side Effects:
**		Rangev[i].rused set to FALSE for all entries.
**		Remap[i] set to -1 or MAXVAR + 1 for all entries.
**
**	Requires:
**		Rangev
**		Remap
**
**	Called By:
**		main
**		gettree
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

clrrange(prim)
int	prim;
{
	register int	i;
	register int	p;

	p = prim;

	for (i = 0; i < MAXVAR + 1; i++)
	{
		if (p)
		{
			Rangev[i].rused = FALSE;
			Remap[i] = MAXVAR + 1;
		}
		else
			Remap[i] = -1;
	}
}
/*
**  DECLARE -- declare a range variable
**
**	A range variable is declared.  If possible, the preferred varno
**	stated is used (this is the one already in the tree).  This
**	should always be possible when reading the original tree (and
**	should probably stay this way to make debugging easier).  When
**	not possible, a new varno is chosen, and Remap[oldvarno] is
**	set to newvarno, so that the tree can later be patched up by
**	'mapvars' (below).
**
**	Parameters:
**		varno -- the preferred varno.
**		name -- the relation name.
**		stat -- the 'relstat' of this relation.
**
**	Returns:
**		none
**		(non-local on error)
**
**	Side Effects:
**		Rangev and possible Remap are updated.  No Rangev
**			entry is ever touched if the 'rused' field
**			is set.
**
**	Requires:
**		Rangev
**		Remap
**
**	Called By:
**		readqry
**
**	Trace Flags:
**		11
**
**	Diagnostics:
**		3100 -- too many variables -- we have run out of
**			space in the range table.  This "cannot happen"
**			when reading the original tree, but can happen
**			when reading another tree from the 'tree'
**			catalog.
**
**	Syserrs:
**		%d redec -- the variable stated has been declared
**			twice in the range table.  Actually, if reading
**			the original query, it means declared twice;
**			if reading from the 'tree' catalog, it means
**			declared THREE times (once in the original
**			query and twice in the catalog).
*/

declare(varno, name, owner, stat)
int	varno;
char	*name;
char	owner[2];
int	stat;
{
	register int	i;
	register int	vn;
	register int	s;

	vn = varno;
	s = stat;

	/* check for preferred slot in range table available */
	if (Rangev[vn].rused)
	{
		/* used: check if really redeclared */
		if (Remap[vn] >= 0)
			syserr("declare: %d redec", vn);

		/* try to find another slot */
		for (i = 0; i < MAXVAR + 1; i++)
			if (!Rangev[i].rused)
				break;

		if (i >= MAXVAR + 1)
		{
			ferror(3100, Qmode, -1, name, 0);	/* too many variables */
		}

		Remap[vn] = i;
		vn = i;
	}

	/* declare variable in the guaranteed empty slot */
	bmove(name, Rangev[vn].relid, MAXNAME);
	bmove(owner, Rangev[vn].rowner, 2);
	Rangev[vn].rstat = s;
	Rangev[vn].rused = TRUE;

#	ifdef xQTR2
	if (tTf(11, 0))
		printf("declare(%d, %.12s, %.2s, %o) into slot %d\n",
		    varno, name, owner, s, vn);
#	endif
#	ifdef xQTR3
	if (tTf(11, 1))
		printf("declare: %.12s%.2s 0%o %d\n", Rangev[vn].relid,
		    Rangev[vn].rowner, Rangev[vn].rstat, Rangev[vn].rused);
#	endif
}
/*
**  MAPVARS -- remap varno's to be unique in 'tree' tree
**
**	A tree is scanned for VAR nodes; when found, the
**	mapping defined in Remap[] is applied.  This is done so that
**	varno's as defined in trees in the 'tree' catalog will be
**	unique with respect to varno's in the user's query tree.  For
**	example, if the view definition uses variable 1 and the user's
**	query also uses variable 1, the routine 'declare' will (when
**	called to define the view definition varno 1) allocate a new
**	varno (e.g. 3) in a free slot in the range table, and put
**	the index of the new slot into the corresponding word of Remap;
**	in this example, Remap[1] == 3.  This routine does the actual
**	mapping in the tree.
**
**	Parameters:
**		tree -- pointer to tree to be remapped.
**
**	Returns:
**		none
**
**	Side Effects:
**		the tree pointed to by 'tree' is modified according
**		to Remap[].
**
**	Requires:
**		Remap -- range table mapping.
**
**	Called By:
**		gettree
**
**	Trace Flags:
**		12
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

mapvars(tree)
QTREE	*tree;
{
	register QTREE	*t;
	register int	i;

	t = tree;
#	ifdef xQTR3
	if (tTf(12, 0) && t != NULL && t->sym.type == ROOT)
	{
		treepr(t, "mapvars:");
		for (i = 0; i < MAXVAR + 1; i++)
			if (Rangev[i].rused && Remap[i] >= 0)
				printf("\t%d => %d\n", i, Remap[i]);
	}
#	endif

	while (t != NULL)
	{
		/* map right subtree */
		mapvars(t->right);

		/* check this node */
		if (t->sym.type == VAR)
		{
			if ((i = Remap[((struct qt_var *)t)->varno]) >= 0)
				((struct qt_var *)t)->varno = i;
		}

		/* map left subtree (iteratively) */
		t = t->left;
	}
}

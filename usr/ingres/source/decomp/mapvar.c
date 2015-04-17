# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"

/*
**	MAPVAR -- construct varable maps for ROOT, AND, and AGHEAD nodes.
**	tl is a flag  which indicates if the target list should
**	be included in the mapping.  If tl = 0, it should; else it should not.
*/

mapvar(tree, tl)
struct querytree	*tree;
int			 tl;
{
	register struct querytree	*t;
	register int			rmap, lmap;
	extern struct querytree		*ckvar();

	t = tree;
	if (t == 0)
		return (0);

	switch (t->sym.type)
	{
	  case ROOT:
	  case AND:
	  case AGHEAD:
		/* map the right side */
		((struct qt_root *)t)->rvarm = rmap = mapvar(t->right, tl);

		/* map the left side or else use existing values */
		if (tl == 0)
		{
			((struct qt_root *)t)->lvarm = lmap = mapvar(t->left, tl);
			((struct qt_root *)t)->lvarc = bitcnt(lmap);
		}
		else
			lmap = ((struct qt_root *)t)->lvarm;

		/* form map of both sides */
		rmap |= lmap;

		/* compute total var count */
		((struct qt_root *)t)->tvarc = bitcnt(rmap);

		return (rmap);

	  case VAR:
		if (((struct qt_var *)(t = ckvar(t)))->valptr)
			return (0);	/* var is a constant */
		return (01 << ((struct qt_var *)t)->varno);
	}

	/* node is not a VAR, AND, ROOT, or AGHEAD */
	return (mapvar(t->left, tl) | mapvar(t->right, tl));
}

# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"decomp.h"

/*
** SETVAR -- Routines to convert a VAR to a constant and back to a VAR
**
**	This file contains the routines for tuple substitution.
**
**	Setvar -- Make a VAR node reference its position in the tuple.
**
**	Clrvar -- Make the VAR node refer back to a VAR
*/

setvar(tree1, var, intid, tuple)
struct querytree 	*tree1;
int			var;
long			*intid;
char			*tuple;

/*
** Setvar -- Var's are changed to reference their values in a tuple.
**	ROOT and AND nodes are changed to update the variable maps.
*/

{
	register struct querytree	*tree;
	register int			mask, nvc;
	struct descriptor		*readopen();
	extern struct querytree		*ckvar();

	tree = tree1;
	if (!tree) 
		return;
	switch (tree->sym.type)
	{
	  case VAR:
		if (((struct qt_var *)(tree=ckvar(tree)))->varno == var)
		{
#			ifdef xDTR1
			if (tTf(12, 0))
			{
				printf("setvar:%d;tree:", var);
				writenod(tree);
			}
#			endif
			if (((struct qt_var *)tree)->attno)
				((struct qt_var *)tree)->valptr = tuple +
					readopen(var)->reloff[((struct qt_var *)tree)->attno];
			else
				((struct qt_var *)tree)->valptr = (char *) intid;
		}
		return;

	  case ROOT:
	  case AND:
		mask = 01 << var;
		nvc = ((struct qt_root *)tree)->tvarc;
		if (((struct qt_root *)tree)->lvarm & mask)
		{
			setvar(tree->left, var, intid, tuple);
			((struct qt_root *)tree)->lvarm &=  ~mask;
			--((struct qt_root *)tree)->lvarc;
			nvc = ((struct qt_root *)tree)->tvarc - 1;
		}
		if (((struct qt_root *)tree)->rvarm & mask)
		{
			setvar(tree->right, var, intid, tuple);
			((struct qt_root *)tree)->rvarm &=  ~mask;
			nvc = ((struct qt_root *)tree)->tvarc - 1;
		}
		((struct qt_root *)tree)->tvarc = nvc;
		return;

	  default:
		setvar(tree->left, var, intid, tuple);
		setvar(tree->right, var, intid, tuple);
		return;
	}
}


clearvar(tree1, var1)
struct querytree	*tree1;
int			var1;

/*
**	Clearvar is the opposite of setvar. For
**	each occurence of var1 in the tree, clear
**	the valptr.
*/

{
	register struct querytree	*tree;
	int				var;
	extern struct querytree		*ckvar();

	tree = tree1;
	if (!tree)
		return;

	var = var1;
	if (tree->sym.type == VAR)
	{
		if (((struct qt_var *)(tree = ckvar(tree)))->varno == var)
			((struct qt_var *)tree)->valptr = 0;
		return;
	}
	clearvar(tree->left, var);
	clearvar(tree->right, var);
}

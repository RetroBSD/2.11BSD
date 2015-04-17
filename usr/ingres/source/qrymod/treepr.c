# include	"../ingres.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../aux.h"
# include	"qrymod.h"

/*
**  TREEPR -- print tree for debugging
**
**	This module prints a tree for debugging purposes.  There are
**	two interfaces: treepr prints an entire tree, and nodepr
**	prints a single node.
**
**	Defines:
**		treepr -- tree print interface.
**		rcsvtrpr -- the recursive part to traverse the tree.
**		nodepr -- node print interface.
**
**	Requires:
**		ferror -- to get the Qmdname[] vector.
**		Rangev -- to print the range table.
**
**	Required By:
**		treeio.c
**		maybe others.
**
**	Files:
**		none
**
**	Trace Flags:
**		none
**
**	History:
**		2/14/79 -- version 6.2/0 release.
**		1/15/79 (eric) -- made simple before release.
**		3/27/78 (eric) -- prettied up
**		2/24/78 (eric) -- written
*/




extern char	*Qmdname[];	/* defined in util.c */
/*
**  TREEPR -- print tree for debugging
**
**	This routine prints a tree for debugging.
**
**	Parameters:
**		tree -- root of tree to be printed
**		label -- label to print for identifying the tree.  NULL
**			is OK and produces a default label.
**
**	Returns:
**		none
**
**	Side Effects:
**		output to terminal
**
**	Requires:
**		rcsvtrpr -- tree traversal routine.
**		Rangev.
**
**	History:
**		2/24/78 (eric) -- written
*/

treepr(tree, label)
QTREE	*tree;
char	*label;
{
	register QTREE	*t;
	register char	*l;
	register int	i;

	t = tree;
	l = label;

	/* print label */
	printf("\n");
	if (l != NULL)
		printf("%s ", l);
	printf("Querytree @ %u:\n", t);

	/* print range table */
	for (i = 0; i < MAXVAR + 1; i++)
	{
		if (!Rangev[i].rused)
			continue;
		printf("range of %d is %.12s [O=%.2s, S=%o]\n",
		    i, Rangev[i].relid, Rangev[i].rowner, Rangev[i].rstat);
	}

	/* print query type */
	if (Qmode >= 0)
		printf("%s ", Qmdname[Qmode]);

	/* print result relation if realistic */
	if (Resultvar >= 0)
		printf("Resultvar %d ", Resultvar);
	printf("\n");

	/* print tree */
	rcsvtrpr(t);

	/* print exciting final stuff */
	printf("\n");
}
/*
**  RCSVTRPR -- traverse and print tree
**
**	This function does the real stuff for treepr.  It recursively
**	traverses the tree in postfix order, printing each node.
**
**	Parameters:
**		tree -- the root of the tree to print.
**
**	Returns:
**		none
**
**	Side Effects:
**		none
**
**	Requires:
**		nodepr -- to actually print the node.
**
**	Called By:
**		treepr
**
**	Trace Flags:
**		none
*/

rcsvtrpr(tree)
QTREE	*tree;
{
	register QTREE	*t;

	t = tree;

	while (t != NULL)
	{
		rcsvtrpr(t->left);
		nodepr(t, FALSE);
		t = t->right;
	}
}
/*
**  NODEPR -- print node for debugging
**
**	Parameters:
**		tree -- the node to print.
**
**	Returns:
**		none
**
**	Side Effects:
**		output to terminal
**
**	Requires:
**		none
*/



nodepr(tree)
QTREE	*tree;
{
	register QTREE	*t;
	register int	ty;
	int		l;
	char		*cp;

	t = tree;
	ty = t->sym.type;
	l = t->sym.len & I1MASK;

	printf("%u: %u, %u/ ", t, t->left, t->right);
	printf("%d, %d: ", ty, l);

	switch (ty)
	{
	  case VAR:
		printf("%d.%d [%d/%d]",
			((struct qt_var *)t)->varno,
			((struct qt_var *)t)->attno,
			((struct qt_var *)t)->frmt,
			((struct qt_var *)t)->frml & I1MASK);
		break;

	  case RESDOM:
		printf("%d [%d/%d]",
			((struct qt_res *)t)->resno,
			((struct qt_var *)t)->frmt,
			((struct qt_var *)t)->frml);
		break;

	  case AOP:
		printf("%d [%d/%d] [%d/%d]",
			((struct qt_res *)t)->resno,
			((struct qt_var *)t)->frmt,
			((struct qt_var *)t)->frml,
			((struct qt_ag *)t)->agfrmt,
			((struct qt_ag *)t)->agfrml);
		break;

	  case UOP:
	  case BOP:
	  case COP:
	  case INT:
		switch (t->sym.len)
		{
		  case 1:
		  case 2:
			printf("%d", t->sym.value[0]);
			break;
		
		  case 4:
			printf("%s", locv(i4deref(t->sym.value)));
			break;
		}
		break;

	  case FLOAT:
		printf("%.10f", i4deref(t->sym.value));
		break;

	  case CHAR:
		cp = (char *) t->sym.value;
		while (l--)
			putchar(*cp++);
		break;

	  case TREE:
	  case AND:
	  case OR:
	  case QLEND:
	  case BYHEAD:
	  case ROOT:
	  case AGHEAD:
		break;

	  default:
		syserr("nodepr: ty %d", ty);
	}
	printf("/\n");
}

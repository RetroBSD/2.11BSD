# include	"../ingres.h"
# include	"../aux.h"
# include	"qrymod.h"

/*
**  QRYMOD -- main driver for query modification
**
**	Reads in the query tree, performs the modifications, writes
**	it out, and does process syncronization with below.  The
**	calling routine must sync with the process above.
**
**	Parameters:
**		root1 -- a pointer to the tree to modify.
**
**	Returns:
**		A pointer to the modified tree.
**
**	Side Effects:
**		You've got to be kidding.  The side effects are
**		all of this module.
**
**	Requires:
**		protect -- the protection algorithm.
**		view -- the view processor.
**		integrity -- the integrity processor.
**
**	Required By:
**		main
**
**	Trace Flags:
**		1
**
**	History:
**		2/14/79 -- version 6.2 release.
*/


QTREE *
qrymod(root1)
QTREE	*root1;
{
	register QTREE	*root;
	extern QTREE	*view(), *integrity(), *protect();

	root = root1;

#	ifdef xQTR1
	if (tTf(1, 0))
		treepr(root, "->QRYMOD");
#	endif

	/* view processing */
	root = view(root);

	/* integrity processing */
	root = integrity(root);

	/* protection processing */
	root = protect(root);

	return (root);
}

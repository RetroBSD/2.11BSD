# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"qrymod.h"

/*
**  TREE -- create new tree node.
**
**	This is a stripped down version of the same thing in the
**	parser.
**
**	It only knows about lengths of zero and two.
**
**	Parameters:
**		lptr -- the left pointer.
**		rptr -- the right pointer.
**		typ -- the node type.
**		len -- the node length.
**		value -- the node value.
**
**	Returns:
**		A pointer to the created node.
**
**	Side Effects:
**		Space is taken from Qbuf.
**
**	Requires:
**		need -- to get space from Qbuf.
**
**	History:
**		2/14/79 -- version 6.2/0 release.
*/


QTREE *
tree(lptr, rptr, typ, len, value)
QTREE	*lptr;
QTREE	*rptr;
char	typ;
int	len;
int	value;
{
	register QTREE	*tptr;
	extern char	*need();
	register int	l;

	l = len;

	tptr = (QTREE *) need(Qbuf, l + 6);
	tptr->left = lptr;
	tptr->right = rptr;
	tptr->sym.type = typ;
	tptr->sym.len = l;

	if (l > 0)
		tptr->sym.value[0] = value;
	return (tptr);
}

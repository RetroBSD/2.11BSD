# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"qrymod.h"


/*
**  WRITETREE.C -- query tree output routines
**
**	These routines write out a query tree in internal format.
**
**	Defines:
**		writetree
**		writeqry
**		writesym
**
**	Required By:
**		qrymod.c
**
**	Trace Flags:
**		70
**
**	History:
**		2/14/79 -- version 6.2/0 release.
*/

extern struct pipfrmt	Pipe, Outpipe;
/*
**  WRITETREE -- write a query tree
**
**	A query tree is written to the down pipe.  The parameter is
**	the root of the tree to be written.
**
**	Parameters:
**		q1 -- the root of the tree to write.
**		wrfn -- the function to call to do physical
**			writes.
**
**	Returns:
**		none
**
**	Side Effects:
**		none
**
**	Requires:
**		nodepr -- for debugging node print.
**
**	Called By:
**		writeqry
*/

writetree(q1, wrfn)
QTREE	*q1;
int	(*wrfn)();
{
	register QTREE	*q;
	register int	l;
	register char	t;

	q = q1;

	/* write the subtrees */
	if (q->left != NULL)
		writetree(q->left, wrfn);
	if (q->right != NULL)
		writetree(q->right, wrfn);

	/* write this node */
	t = q->sym.type;
	if (t == AND || t == ROOT || t == AGHEAD)
		q->sym.len = 0;
	l = q->sym.len & I1MASK;
	(*wrfn)(&(q->sym), l + 2);
#	ifdef	xQTR1
	if (tTf(70, 5))
		nodepr(q, TRUE);
#	endif
}
/*
**  WRITEQRY -- write a whole query
**
**	An entire query is written, including range table, and so
**	forth.
**
**	Parameters:
**		root -- the root of the tree to write.
**		wrfn -- the function to do the physical write.
**
**	Returns:
**		none
**
**	Side Effects:
**		none.
**
**	Requires:
**		Rangev -- the range table.
**		Resultvar -- The result variable number.
**		writesym() -- to write symbols out.
**
**	Called By:
**		User.
*/

writeqry(root, wrfn)
QTREE	*root;
int	(*wrfn)();
{
	register int		i;
	struct srcid		sid;

	/* write the query mode */
	if (Qmode >= 0)
		writesym(QMODE, 2, &Qmode, wrfn);

	/* write the range table */
	for (i = 0; i < MAXVAR + 1; i++)
	{
		if (Rangev[i].rused)
		{
			sid.type = SOURCEID;
			sid.len = sizeof sid - 2;
			pmove(Rangev[i].relid, sid.srcname, MAXNAME, ' ');
			sid.srcvar = i;
			sid.srcstat = Rangev[i].rstat;
			bmove(Rangev[i].rowner, sid.srcown, 2);
			(*wrfn)(&sid, sizeof sid);
		}
	}

	/* write a possible result variable */
	if (Resultvar >= 0)
		writesym(RESULTVAR, 2, &Resultvar, wrfn);

	/* write the tree */
	writetree(root, wrfn);
}
/*
**  WRITESYM -- write a symbol block
**
**	A single symbol entry of the is written.
**	a 'value' of zero will not be written.
*/

writesym(typ, len, value, wrfn)
int	typ;
int	len;
char	*value;
int	(*wrfn)();
{
	struct symbol	sym;
	register char	*v;
	register int	l;

	sym.type = typ & I1MASK;
	sym.len = l = len & I1MASK;
	(*wrfn)(&sym, 2);
	v = value;
	if (v != 0)
		(*wrfn)(v, l);
}

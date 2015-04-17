# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"decomp.h"


extern struct querytree		*need();


struct querytree *copytree(root, buf)
struct querytree	*root;
char			*buf;

/*
**	Make a copy of a tree.
*/

{
	register struct querytree	*r, *q;
	register int			length;

	r = root;

	if (!r)
		return (0);

	length = r->sym.len & I1MASK;
	q = need(buf, length + 6);
	bmove(&r->sym, &q->sym, length + 2);

	q->left = copytree(r->left, buf);
	q->right = copytree(r->right, buf);

	return (q);
}
struct querytree *copy_ands(root, buf)
struct querytree	*root;
char		 	*buf;

/*
**	Make a new copy of the root by making
**	new AND nodes and connecting them to the
**	same branches.
*/
{
	register struct querytree	*q;
	register struct querytree	*x, *y;
	struct querytree		*newroot;

#	ifdef xDTR1
	if (tTf(10, -1))
		printf("COPY_ANDS");
#	endif
	newroot = need(buf, 0);
	y = 0;

	for (q=root; q->sym.type != QLEND; q=q->right)
	{
		x = need(buf, q->sym.len + 6);
		x->left = q->left;
		bmove(&q->sym, &x->sym, q->sym.len + 2);
		if (y)
			y->right = x;
		y = x;
	}
	y->right = q;

#	ifdef xDTR1
	if (tTf(10, 0))
		printree(newroot, "Newtree");
#	endif
	return(newroot);
}



struct querytree *makroot(buf)
char 	*buf;
{
	register struct querytree *s;

	s = need(buf, 14);
	s->right = Qle;
	s->left = Tr;
	((struct qt_root *)s)->rootuser = ((struct qt_root *)s)->lvarm = ((struct qt_root *)s)->rvarm = ((struct qt_root *)s)->tvarc = ((struct qt_root *)s)->lvarc = 0;
	s->sym.type = ROOT;
	s->sym.len = 8;
	return (s);
}


struct querytree *makresdom(buf, node)
char			*buf;
struct querytree	*node;
{
	register struct querytree	*res, *n;

	n = node;
	res = need(buf, 10);
	res->sym.type = RESDOM;
	res->sym.len = 4;
	if (n->sym.type == AOP)
	{
		((struct qt_var *)res)->frmt = ((struct qt_ag *)n)->agfrmt;
		((struct qt_var *)res)->frml = ((struct qt_ag *)n)->agfrml;
	}
	else
	{
		((struct qt_var *)res)->frmt = ((struct qt_var *)n)->frmt;
		((struct qt_var *)res)->frml = ((struct qt_var *)n)->frml;
	}
	return (res);
}

struct querytree *makavar(node, varnum, attnum)
struct querytree	*node;
int			varnum, attnum;
{
	register struct querytree *avar, *n;

	n = node;
	avar = need(Qbuf, 12);
	avar->left = avar->right = (struct querytree *) 0;
	((struct qt_var *)avar)->valptr = (char *) 0;
	avar->sym.type = VAR;
	avar->sym.len = 6;
	((struct qt_var *)avar)->frmt = ((struct qt_var *)n)->frmt;
	((struct qt_var *)avar)->frml = ((struct qt_var *)n)->frml;
	((struct qt_var *)avar)->varno = varnum;
	((struct qt_var *)avar)->attno = attnum;
#	ifdef xDTR1
	if (tTf(14, 3))
	{
		printf("makavar: node=%l  ", n);
		writenod(avar);
	}
#	endif
	return(avar);
}

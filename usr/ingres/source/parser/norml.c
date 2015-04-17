# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"../tree.h"
# include	"../symbol.h"

/*
** NORML
**	this routine passes the qualification clause portion of the query
**	tree to the routines for depressing nots and for conjunctive 
**	normalization.  It also calls a routine to place an and with one
**	null branch at the rightmost end of the tree.
*/
struct querytree *
norml(ptr)
struct querytree	*ptr;
{
	extern struct querytree	*nnorm();
	extern struct querytree	*travers();
	extern struct querytree	*tree();

	if (ptr == NULL)
		return (tree(NULL, NULL, QLEND, 0, 0));

	/* push through the 'nots' as far a possible */
	ptr = nnorm(ptr);

	/* make tree into conjunctive normal form */
	ptr = travers(ptr);

	/* terminate the list on the rightmost branch */
	adjust(&ptr);

	/* make 'ands' into a linear list */
	mvands(ptr);
	return (ptr);
}


/*
** NORM
**	this routine takes a tree which has nots only at the lower ends, and
**	places it into conjunctive normal form by repeatedly applying the
**	replacement rule: A or (B and C) ==> (A or B) and (A or C)
*/
struct querytree *
norm(p1)
struct querytree	*p1;
{
	extern struct querytree		*tree();
	register struct querytree	*p;
	register struct querytree	*lptr;
	register struct querytree	*rptr;

	p = p1;
	switch (p->sym.type)
	{
	  case AND:
		p->left = norm(p->left);
		p->right = norm(p->right);
		break;

	  case OR:
		if (p->right->sym.type == AND)
		{
		andright:
			/*
			** combine left subtree with each subtree of the
			** right subtree
			*/
			/*
			** use copy first so that the copy is guaranteed to be
			** the same as the original
			*/
			lptr = tree(treedup(p->left), p->right->left, OR, 0, 0);
			rptr = tree(p->left, p->right->right, OR, 0, 0);
			lptr = norm(lptr);
			rptr = norm(rptr);
			/* change node type to AND from OR */
			p->left = lptr;
			p->right = rptr;
			p->sym.type = AND;	/* length is same */
			break;
		}
		if (p->left->sym.type == AND)
		{
		andleft:
			/*
			** combine right subtree with each subtree of the
			** left subtree
			*/
			/*
			** again, the copy should be used first
			*/
			lptr = tree(p->left->left, treedup(p->right), OR, 0, 0);
			rptr = tree(p->left->right, p->right, OR, 0, 0);
			lptr = norm(lptr);
			rptr = norm(rptr);
			/* change node type to AND from OR */
			p->left = lptr;
			p->right = rptr;
			p->sym.type = AND;
			break;
		}
		/*
		** when TRAVERS is being used to optomize the normalization
		** order there should never be (I think) an OR as a child
		** of the OR in the parent.  Since TRAVERS works bottom up
		** in the tree the second OR level should be gone.
		*/
		if (p->right->sym.type == OR)
		{
			/* skip this (p->sym.type) "or" and normalize the right subtree */
			p->right = norm(p->right);

			/* now re-try the current node */
			if (p->right->sym.type == AND)
				goto andright;
			break;
		}
		if (p->left->sym.type == OR)
		{
			/* skip this "or" and normalize the left subtree */
			p->left = norm(p->left);

			/* now re-try the current node */
			if (p->left->sym.type == AND)
				goto andleft;
			break;
		}
		break;
	}
	return (p);
}

/*
** TRAVERS
**	traverse the tree so that conversion of ORs of ANDs can
**	take place at the innermost clauses first.  IE, normalize
**	before replication rather than after replication.
**
**	This routine need not be used.  The NORM routine will completely
**	traverse the tree and normalize it but...    TRAVERS finds
**	the lowest subtree to normalize first so that sub-trees can
**	be normalized before replication, hence reducing the time required
**	to normalize large trees.  It will also make OR-less trees faster
**	to normalize (since nothing need be done to it).
*/
struct querytree *
travers(p1)
struct querytree	*p1;
{
	register struct querytree	*p;
	extern struct querytree		*norm();

	p = p1;
	if (p->right != NULL)
		p->right = travers(p->right);
	if (p->left != NULL)
		p->left = travers(p->left);
	if (p->sym.type == OR)
		p = norm(p);
	return (p);
}
/*
** NNORM
**	this routine depresses nots in the query tree to the lowest possible
**	nodes.  It may also affect arithmetic operators in this procedure
*/
struct querytree *
nnorm(p1)
struct querytree	*p1;
{
	extern struct querytree		*notpush();
	register struct querytree	*p;

	p = p1;
	if (p->sym.type == AGHEAD)
	{
		/*
		** don't need to continue past an AGHEAD
		** actually, it causes problems if you do
		** because the qualification on the agg
		** has already been normalized and the
		** QLEND needs to stay put
		*/
		return (p);
	}
	if ((p->sym.type == UOP) && (((struct qt_op *)p)->opno == opNOT))
	{
		/* skip not node */
		p = p->right;
		p = notpush(p);
	}
	else
	{
		if (p->left != NULL)
			p->left = nnorm(p->left);
		if (p->right != NULL)
			p->right = nnorm(p->right);
	}
	return (p);
}

/*
** NOTPUSH
**	this routine decides what to do once a not has been found in the
**	query tree
*/
struct querytree *
notpush(p1)
struct querytree	*p1;
{
	extern struct querytree		*nnorm();
	register struct querytree	*p;

	p = p1;
	switch (p->sym.type)
	{
	  case AND:
		p->sym.type = OR;
		p->left = notpush(p->left);
		p->right = notpush(p->right);
		break;

	  case OR:
		p->sym.type = AND;
		p->left = notpush(p->left);
		p->right = notpush(p->right);
		break;

	  case BOP:
		switch (((struct qt_op *)p)->opno)
		{
		  case opEQ:
			((struct qt_op *)p)->opno = opNE;
			break;

		  case opNE:
			((struct qt_op *)p)->opno = opEQ;
			break;

		  case opLT:
			((struct qt_op *)p)->opno = opGE;
			break;

		  case opGE:
			((struct qt_op *)p)->opno = opLT;
			break;

		  case opLE:
			((struct qt_op *)p)->opno = opGT;
			break;

		  case opGT:
			((struct qt_op *)p)->opno = opLE;
			break;

		  default:
			syserr("strange BOP in notpush '%d'", ((struct qt_op *)p)->opno);
		}
		break;

	  case UOP:
		if (((struct qt_op *)p)->opno == opNOT)
		{
			/* skip not node */
			p = p->right;
			p = nnorm(p);
		}
		else
			syserr("strange UOP in notpush '%d'", ((struct qt_op *)p)->opno);
		break;

	  default:
		syserr("unrecognizable node type in notpush '%d'", p->sym.type);
	}
	return (p);
}

/*
** ADJUST
**	terminate qual with an AND and a QLEND at the rightmost branch
*/
adjust(pp)
struct querytree	**pp;
{
	extern struct querytree		*tree();
	register struct querytree	*p;

	p = *pp;
	switch (p->sym.type)
	{
	  case AND: 
		adjust(&(p->right));
		break;

	  case OR:
	  default:
		*pp = tree(p, tree(NULL, NULL, QLEND, 0, NULL), AND, 0, 0);
		break;
	}
}

struct querytree *treedup(p1)
struct querytree	*p1;
{
	register struct querytree	*np;
	register struct querytree	*p;
	extern char			*Qbuf;

	if ((p = p1) == NULL)
		return (p);
	np = (struct querytree *) need(Qbuf, (p->sym.len & I1MASK) + 6);
	bmove(p, np, (p->sym.len & I1MASK) + 6);
	np->left = treedup(p->left);
	np->right = treedup(p->right);
	return (np);
}

/*
**	MVANDS -- pushes all AND's in Qual into linear list
*/
mvands(andp)
struct querytree	*andp;
{
	register struct querytree	*ap, *lp, *rp;

	ap = andp;
	if (ap->sym.type == QLEND)
		return;
	rp = ap->right;
	lp = ap->left;
	if (lp->sym.type == AND)
	{
		ap->left = lp->right;
		ap->right = lp;
		lp->right = rp;
		mvands(ap);
	}
	else
		mvands(rp);
}

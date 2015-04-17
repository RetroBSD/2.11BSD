# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"decomp.h"

/*
** DECOMP1.C
**
**	contains routines associated with setting up 
**	detachable 1-variable sub-queries.
**	ptrs to these sq's are kept in the 
**	array 'sqlist' declared in the main decomp routine.
**
*/

pull_sq(tree1, sqlist, locrang, sqrange, buf)
struct querytree	*tree1;
struct querytree	*sqlist[];
int			locrang[];
int			sqrange[];
char			*buf;
{
	register struct querytree 	*q, *tree, *r;
	struct querytree 		*s;
	int 				anysq, j, badvar;
	extern struct querytree		*makroot();

	tree = tree1;

#	ifdef xDTR1
	if (tTf(10, 0))
		printf("PULL_SQ:tree=%l\n", tree);
#	endif

	anysq = 0;
	for (j = 0; j < MAXRANGE; j++)
		sqlist[j] = 0;

	if (((struct qt_root *)tree)->tvarc == 1)
		return;

	/* detach all one variable clauses except:
	** if the target list is one variable and
	** that variable is disjoint from the other
	** variables, then don't pull it.
	**
	** It will be more efficient to process it
	** all at once in decompy
	*/

	badvar = 0;
	if (((struct qt_root *)tree)->lvarc == 1)
	{
		badvar = ((struct qt_root *)tree)->lvarm; /* get bit position of var */

		/* look for a two variable clause involving badvar */
		for (r = tree->right; r->sym.type != QLEND; r = r->right)
		{
			if (((struct qt_root *)r)->lvarc > 1 && (((struct qt_root *)r)->lvarm & badvar))
			{
				badvar = 0;
				break;
			}
		}
	}
#	ifdef xDTR1
	if (tTf(10, 2))
		printf("Detachable clauses: (badvar=%o)\n", badvar);
#	endif
	for (r=tree; r->right->sym.type!=QLEND; )
	{
#		ifdef xDTR1
		if (tTf(10, 3))
			writenod(r);
#		endif
		q = r;
		r = r->right;
		if (((struct qt_root *)r)->lvarc == 1)
		{
			j = bitpos(((struct qt_root *)r)->lvarm);
#			ifdef xDTR1
			if (tTf(10, 4))
			{
				printf("\nvar=%d,", j);
				printree(r->left, "clause");
			}
#			endif
			if (((struct qt_root *)r)->lvarm == badvar)
			{
#				ifdef xDTR1
				if (tTf(10, 5))
					printf("not detaching \n");
#				endif
				continue;
			}
			anysq++;

			if (!sqlist[j])		/* MAKE ROOT NODE FOR SUBQUERY */
				sqlist[j] = makroot(buf);
			s = sqlist[j];

			/* MODIFY MAIN QUERY */

			q->right = r->right;

			/* MODIFY `AND` NODE OF DETACHED CLAUSE */

			r->right = s->right;
			((struct qt_root *)r)->rvarm = ((struct qt_root *)s)->rvarm;
			((struct qt_root *)r)->tvarc = 1;

			/* ADD CLAUSE TO SUB-QUERY */

			s->right = r;
			((struct qt_root *)s)->rvarm = ((struct qt_root *)r)->lvarm;
			((struct qt_root *)s)->tvarc = 1;

#			ifdef xDTR1
			if (tTf(10, 6))
				printree(s, "SQ");
#			endif

			r = q;
		}
	}

	/* NOW SET UP TARGET LIST FOR EACH SUBQUERY IN SQLIST */

#	ifdef xDTR1
	if (tTf(10, 7))
		printf("# sq clauses=%d\n", anysq);
#	endif
	if (anysq)
	{
#		ifdef xDTR1
		if (tTf(10, 8))
			printf("Dfind--\n");
#		endif
		dfind(tree, buf, sqlist);
		mapvar(tree, 1);

		/* create the result relations */
		for (j = 0; j < MAXRANGE; j++)
		{
			if (q = sqlist[j])
			{
				if (q->left->sym.type != TREE)
				{
					savrang(locrang, j);
					sqrange[j] = mak_t_rel(q, "d", -1);
				}
				else
					sqrange[j] = NORESULT;
			}
		}
	}
}

dfind(tree1, buf, sqlist)
struct querytree *tree1;
char		 *buf;
struct querytree *sqlist[];
{
	register int			varno;
	register struct querytree	*tree, *sq;
	extern struct querytree		*ckvar();

	tree = tree1;
	if (!tree) 
		return;
#	ifdef xDTR1
	if (tTf(10, 9))
		writenod(tree);
#	endif
	if (tree->sym.type == VAR)
	{
		tree = ckvar(tree);
		varno = ((struct qt_var *)tree)->varno;
		if (sq = sqlist[varno])
			maktl(tree, buf, sq, varno);
		return;
	}

	/* IF CURRENT NODE NOT A `VAR` WITH SQ, RECURSE THRU REST OF TREE */

	dfind(tree->left, buf, sqlist);
	dfind(tree->right, buf, sqlist);
	return;
}


maktl(node, buf, sq1, varno)
struct querytree	*node;
char			*buf;
struct querytree	*sq1;
int			varno;
{
	register struct querytree 	*resdom, *tree, *sq;
	int				domno, map;
	extern struct querytree		*makresdom(), *copytree();

	sq = sq1;
	domno = ((struct qt_var *)node)->attno;

#	ifdef xDTR1
	if (tTf(10, 12))
		printf("\tVar=%d,Dom=%d ", varno, domno);
#	endif
	/* CHECK IF NODE ALREADY CREATED FOR THIS DOMAIN */

	for (tree = sq->left; tree->sym.type != TREE; tree = tree->left)
		if (((struct qt_var *)tree->right)->attno == domno)
		{
#			ifdef xDTR1
			if (tTf(10, 13))
				printf("Domain found\n");
#			endif
			return;
		}

	/* create a new resdom for domain */

	resdom = makresdom(buf, node);
	*resdom->sym.value = sq->left->sym.type == TREE? 1:
					*sq->left->sym.value + 1;
	/* resdom->right is a copy of the var node in order to
	** protect against tempvar() changing the var node.
	*/
	resdom->left = sq->left;
	resdom->right = copytree(node, buf);


	/* update ROOT node if necessary */

	sq->left = resdom;
	map = 1 << varno;
	if (!(((struct qt_root *)sq)->lvarm & map))
	{
		/* var not currently in tl */
		((struct qt_root *)sq)->lvarm |= map;
		((struct qt_root *)sq)->lvarc++;

		/* if var is not in qualification then update total count */
		if (!(((struct qt_root *)sq)->rvarm & map))
			((struct qt_root *)sq)->tvarc++;
#		ifdef xDTR1
		if (tTf(10, 15))
		{
			printf("new root ");
			writenod(sq);
		}
#		endif
	}

#	ifdef xDTR1
	if (tTf(10, 14))
	{
		printf("new dom ");
		writenod(resdom);
	}
#	endif
	return;
}

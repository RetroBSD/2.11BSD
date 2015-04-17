# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"

/*
**  TRBUILD -- Rebuild a tree in memory
**
**	Trbuild is called with a pointer to the TREE node of
**	a query already in memory. It rebuilds the pointer
**	structure assuming the querytree is in postfix order.
**
**	Parameters:
**		bufptr - a pointer to the TREE node
**
**
**	Returns:
**		NULL - Internal stack overflow (STACKSIZ)
**		pointer to the ROOT node
**
**	Side Effects:
**		All left & right pointers are rebuilt.
**
**	Called By:
**		readq
**
**	Syserrs:
**		syserr if there are too many leaf nodes or too
**		few child nodes
**
**	History:
**		1/18/79 (rse) - written
*/


struct querytree *trbuild(bufptr)
char	*bufptr;
{
	register struct querytree 	**stackptr;
	register char			*p; /* really struct querytree * */
	register struct symbol 		*s;
	struct querytree		*treestack[STACKSIZ];


	stackptr = treestack;

	for (p = bufptr; ;p += 6 + ((s->len + 1) & 0376))
	{
		s = &(((struct querytree *)p)->sym);
		((struct querytree *)p)->left = ((struct querytree *)p)->right = 0;

		/* reunite p with left and right children on stack, if any*/
		if (!leaf(p))		/* this node has children */
		{
			if (s->type != UOP)
				if (stackptr <= treestack) 
				{
				err:
					syserr("trbuild:too few nodes");
				}
				else
					((struct querytree *)p)->right = *(--stackptr);
			if (s->type != AOP)
				if (stackptr <= treestack) 
					goto err;
				else
					((struct querytree *)p)->left = *(--stackptr);
		}

		/*
		** If this is a ROOT node then the tree is complete.
		** verify that there are no extra nodes in the
		** treestack.
		*/
		if (s->type == ROOT)	 	/* root node */
		{
			if (stackptr != treestack)
				syserr("trbuild:xtra nodes");
			return ((struct querytree *)p);
		}

		/* stack p */
		if (stackptr-treestack >= STACKSIZ) 
			return (NULL);	/* error:stack full */
		*(stackptr++) = (struct querytree *)p;

	}
}


leaf(p)
struct querytree *p;
{
	switch (p->sym.type)
	{
	  case VAR:
	  case TREE:
	  case QLEND:
	  case INT:
	  case FLOAT:
	  case CHAR:
	  case COP:
		return(1);

	  default:
		return(0);
	}
}

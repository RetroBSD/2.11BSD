# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"decomp.h"

struct agglist
{
	struct querytree	**father;	/* addr of pointer to you */
	struct querytree	*agpoint;	/* pointer to aghead */
	struct querytree	*agfather;	/* is your father an aggregate? */
	int			agvarno;	/* var # assigned to aggr fnct */
};

struct hitlist
{
	struct querytree	**trepr;	/* position in tree to be changed */
	int			byno;		/* by-list position */
};

struct agglist	*Aggnext;

aggregate(root)
struct querytree	*root;

/*
**	AGGREGATE - replace aggregates with their values
**
**	Aggregate attempts to optimize aggregate processing
**	wherever possible. It replaces aggregates with their
**	values, and links aggregate functions which have
**	identical by-lists together.
**
**	Note that for the sake of this code, A "prime"
**	aggregate is one in which duplicates are removed.
**	These are COUNTU, SUMU, and AVGU.
**
**	Aggregate first forms a list of all aggregates in
**	the order they should be processed.
**
**	For each aggregate, it looks at all other aggregates
**	to see if there are two simple aggregates
**	or if there is another aggregate funtion with the
**	same by-list.
**
**	An attempt is made to run
**	as many aggregates as possible at once. This can be
**	done only if two or more aggregates have the same
**	qualifications and in the case of aggregate functions,
**	they must have identical by-lists.
**	Even then, certain combinations
**	of aggregates cannot occure together. The list is
**	itemized later in the code.
**
**	Aggregate calls BYEVAL or AGEVAL to actually process
**	aggregate functions or aggregates respectively.
**
*/

{
	struct agglist			alist[MAXAGG + 1];
	struct querytree		*rlist[MAXAGG + 1];
	struct agglist			*al, *al1;
	register struct querytree	*agg, *aop1, *r;
	struct querytree		*aop, *agg1;
	int				i, simple_agg, varmap;
	int				attcnt, anyagg, attoff, twidth;
	struct querytree		*makavar(), *agspace();

	al = alist;
	Aggnext = al;

	findagg(&root, root);	/* generate list of all aggregates */
	Aggnext->agpoint = 0;	/* mark end of the list */
	anyagg = 0;

	varmap = ((struct qt_root *)root)->lvarm | ((struct qt_root *)root)->rvarm;

	/* process each aggregate */
	for (;agg = al->agpoint; al++)
	{

		/* is this still an aggregate? */
		if (agg->sym.type != AGHEAD)
			continue;
		mapvar(agg, 0);	/* map the aggregate tree */
		anyagg++;

		Sourcevar = bitpos(((struct qt_root *)agg)->lvarm | ((struct qt_root *)agg)->rvarm);
#		ifdef xDTR1
		if (tTf(6, 4))
			printf("Sourcevar=%d,rel=%s\n", Sourcevar, rangename(Sourcevar));
#		endif

		simple_agg = (agg->left->sym.type == AOP);	/* TRUE if no bylist */
		aop = agg->left;	/* assume it was TRUE */
#		ifdef xDTR1
		if (tTf(6, 0))
			printf("%s\n", simple_agg ? "agg" : "agg-func");
#		endif
		if (simple_agg)
		{
			/* simple aggregate */
			rlist[0] = agspace(aop);
			twidth = ((struct qt_var *)aop)->frml & I1MASK;	/* init width to the width of the aggregate */
		}
		else
		{
			attoff = ((struct qt_res *)agg->left->left)->resno + 2;
			aop = aop->right;	/* find the AOP node */
			/* assign  new source variable for aggregate */
			al->agvarno = getrange(&varmap);
			/* compute width of bylist + count field */
			twidth = findwid(agg->left) + 4;

			/* make sure the aggregate does not exceed max dimensions */
			if (chkwidth(aop, &twidth, attoff))
				derror(AGFTOBIG);
		}
		attcnt = 1;	/* one aggregate so far */

		/* look for another identical aggregate */
		for (al1 = al + 1; agg1 = al1->agpoint; al1++)
		{

			/* if agg is nested inside agg1 then ignore it */
			if (al->agfather == agg1 || agg1->sym.type != AGHEAD)
				continue;

			/* split aggs and agg-func apart */
			/* check for identical aggregate */
			if (simple_agg)
			{
				aop1 = agg1->left;	/* find AOP node */

				if (aop1->sym.type != AOP)
					continue;	/* not a simple agg */

				/* make sure they can be run together */
				if (checkagg(agg, aop, agg1, aop1) == 0) 
					continue;

				if ((i = sameagg(agg, aop1, attcnt)) >= 0)
				{
					/* found identical aggregate to rlist[i] */
					r = rlist[i];
				}
				else
				{
					/* put this agg in with the others */

					/* first make sure it won't exceed tuple length */
					if (chkwidth(aop1, &twidth, 0))
						continue;	/* can't be included */
					r = rlist[attcnt++] = agspace(aop1);

					/* connect into tree */
					aop1->left = agg->left;
					agg->left = aop1;
				}
			}
			else
			{
				/* aggregate function */
				if (!sameafcn(agg->left->left, agg1->left->left))
					continue;

				aop1 = agg1->left->right;	/* find AOP */


				if (checkagg(agg, aop, agg1, aop1) == 0)
				{
					/* same by-lists but they can't be run together */
					continue;
				}

				if ((i = sameagg(agg, aop1, attcnt)) < 0)
				{
					/* make sure there is room */
					if (chkwidth(aop1, &twidth, attcnt + attoff))
						continue;

					/* add aggregate into tree */
					i = attcnt++;

					aop1->left = agg->left->right;
					agg->left->right = aop1;
				}

				r = makavar(aop1, al->agvarno, i + attoff);
			}
			/* replace aggregate in original tree with its value */
			*(al1->father) = r;

			/* remove aggregate from local list */
			agg1->sym.type = -1;
#			ifdef xDTR1
			if (tTf(6, 3))
				printf("including aghead %l\n", agg1);
#			endif
		}

		/* process aggregate */
		if (simple_agg)
		{
			rlist[attcnt] = 0;
			ageval(agg, rlist);	/* pass tree and result list */
			r = rlist[0];
		}
		else
		{
			opt_bylist(&alist, agg);
			byeval(al->agfather, agg, al->agvarno);
			r = makavar(aop, al->agvarno, attoff);
		}
		/*
		** Link result into tree. al->father hold the address
		** &(tree->left) or &(tree->right).
		*/
		*(al->father) = r;
#		ifdef xDTR1
		if (tTf(6, 4))
			printree(*(al->father), "aggregate value");
#		endif
	}
	if (anyagg)
	{
		opt_bylist(&alist, root);
		mapvar(root, 0);	/* remap main tree */
	}
}

findagg(nodep,  agf)
struct querytree	**nodep;
struct querytree	*agf;

/*
**	findagg builds a list of all aggregates
**	in the tree. It finds them by leftmost
**	innermost first.
**
**	The parameters represent:
**		nodep:	the address of the node pointing to you
**				eg. &(tree->left) or &(tree->right)
**		agf:	the root node. or if we are inside
**			a nested aggregate, the AGHEAD node
*/

{
	register struct querytree	*q, *f;
	register struct agglist		*list;
	int				agg;

	if ((q = *nodep) == NULL)
		return;

	f = agf;
	if (agg = (q->sym.type == AGHEAD))
		f = q;	/* this aggregate is now the father root */

	/* find all aggregates below */
	findagg(&(q->left), f);
	findagg(&(q->right), f);

	/* if this is an aggregate, put it on the list */
	if (agg)
	{
		list = Aggnext;
		list->father = nodep;
		list->agpoint = q;
		list->agfather = agf;
		Aggnext++;
	}
}



struct querytree *agspace(aop)
struct querytree	*aop;

/*
**	Agspace allocates space for the result of
**	a simple aggregate.
*/

{
	register struct querytree	*a, *r;
	register int			length;
	struct querytree		*need();

	a = aop;
	length = ((struct qt_var *)a)->frml & I1MASK;

	r = need(Qbuf, length + 6);
	r->left = r->right = 0;
	r->sym.type = ((struct qt_var *)a)->frmt;
	r->sym.len = length;

	return (r);
}



chkwidth(aop, widthp, domno)
struct querytree	*aop;
int			*widthp;
int			domno;

/*
** Chkwidth -- make sure that the inclusion of another aggregate will
**	not exceed the system limit. This means that the total width
**	cannot exceed MAXTUP and the number of domains cannot exceed MAXDOM-1
*/

{
	register int	width;

	width = *widthp;

#	ifdef xDTR1
	if (tTf(6, 10))
		printf("agg width %d,dom %d\n", width, domno);
#	endif

	width += (((struct qt_var *)aop)->frml & I1MASK);

	if (width > MAXTUP || domno > MAXDOM - 1)
		return (1);

	*widthp = width;
	return (0);
}


cprime(aop)
struct querytree	*aop;

/*
**	Determine whether an aggregate is prime
**	or a don't care aggregate. Returns TRUE
**	if COUNTU,SUMU,AVGU,MIN,MAX,ANY.
**	Returns false if COUNT,SUM,AVG.
*/
{
	register int	i;

	i = TRUE;
	switch (aop->sym.value[0])
	{
	  case opCOUNT:
	  case opSUM:
	  case opAVG:
		i = FALSE;
	}
	return (i);
}


getrange(varmap)
int	*varmap;

/*
**	Getrange find a free slot in the range table
**	for an aggregate function.
**
**	If there are no free slots,derror is called
*/

{
	register int	i, map, bit;

	map = *varmap;

	for (i = 0; i < MAXRANGE; i++)
	{
		/* if slot is used, continue */
		if ((bit = 1 << i) & map)
			continue;

		map |= bit;	/* "or" bit into the map */
		*varmap = map;

#		ifdef xDTR1
		if (tTf(6, 10))
			printf("Assn var %d, map %o\n", i, map);
#		endif

		return (i);
	}
	derror(NODESCAG);
}


checkagg(aghead1, aop1, aghead2, aop2)
struct querytree	*aghead1;
struct querytree	*aop1;
struct querytree	*aghead2;
struct querytree	*aop2;
{
	register struct querytree	*aop_1, *aop_2, *agg1;
	int				ok;

	/* two aggregate functions can be run together
	** according to the following table:
	**
	**		prime	!prime	don't care
	**
	** prime	afcn?	never	afcn?
	** !prime	never	always	always
	** don't care	afcn?	always	always
	**
	** don't care includes: ANY, MIN, MAX
	** afcn? means only if a-fcn's are identical
	*/

	aop_1 = aop1;
	aop_2 = aop2;
	agg1 = aghead1;

	if (!prime(aop_1) && !prime(aop_2))
		ok = TRUE;
	else
		if (sameafcn(aop_1->right, aop_2->right))
			ok = cprime(aop_1) && cprime(aop_2);
		else
			ok = FALSE;
	/* The two aggregates must range over the same variables */
	if ((((struct qt_root *)agg1)->lvarm | ((struct qt_root *)agg1)->rvarm)
		!= (((struct qt_root *)aghead2)->lvarm | ((struct qt_root *)aghead2)->rvarm))
		ok = FALSE;

	/* check the qualifications */
	if (ok)
		ok = sameafcn(agg1->right, aghead2->right);
	return (ok);
}


sameagg(aghead, newaop, agg_depth)
struct querytree	*aghead;
struct querytree	*newaop;
int			agg_depth;
{
	register struct querytree	*agg, *newa;
	register int			i;

	agg = aghead;
	newa = newaop;
	agg = agg->left;
	if (agg->sym.type == BYHEAD)
		agg = agg->right;

	/* agg now points to first aggregate */
	for (i = 1; agg; agg = agg->left, i++)
		if (sameafcn(agg->right, newa->right) && agg->sym.value[0] == newa->sym.value[0])
		{
#			ifdef xDTR1
			if (tTf(6, 6))
				printf("found identical aop %l\n", newa);
#			endif
			return (agg_depth - i);
		}

	/* no match */
	return (-1);
}


struct hitlist	*Hnext, *Hlimit;


opt_bylist(alist, root)
struct agglist		*alist;
struct querytree	*root;
{
	register struct agglist		*al;
	register struct querytree	*agg;
	register struct hitlist		*hl;
	struct querytree		**tpr, *tree, *lnodv[MAXDOM+2];
	struct hitlist			hlist[30];
	int				anyop, i, usedmap, vars, treemap;
	extern struct querytree		*makavar();

	/* compute bitmap of all possible vars in tree (can include xtra vars) */
	treemap = ((struct qt_root *)root)->lvarm | ((struct qt_root *)root)->rvarm;
	anyop = FALSE;

	/* scan the list of aggregates looking for one nested in root */
	for (al = alist; (agg = al->agpoint) && agg != root; al++)
	{
		if (agg->sym.type == AGHEAD && agg->left->sym.type == BYHEAD &&
				al->agfather == root)
		{

			/* this aggregate function is nested in root */

			/* make sure it has some vars of interest */
			if ((treemap & varfind(agg->left->left, NULL)) == 0)
				continue;

#			ifdef xDTR1
			if (tTf(6, 11))
				printree(agg, "nested agg");
#			endif

			/* form list of bydomains */
			lnodv[lnode(agg->left->left, lnodv, 0)] = 0;
			usedmap = 0;

			Hnext = &hlist[0];
			Hlimit = &hlist[30];

			/* find all possible replacements */
			vars = modtree(&root, lnodv, &usedmap);

			/*
			** All references to a variable must be replaced
			** in order to use this aggregates by-domains.
			*/
			if (usedmap && ((vars & usedmap) == 0))
			{
#				ifdef xDTR1
				if (tTf(6, 7))
					printf("Committed\n");
#				endif
				/* success. Committ the tree changes */
				Hnext->trepr = NULL;

				for (hl = &hlist[0]; tpr = hl->trepr; hl++)
				{
					/* get bydomain number */
					i = hl->byno;

					/* get node being replaced */
					tree = *tpr;

					/* if it is already a var, just change it */
					if (tree->sym.type == VAR)
					{
						((struct qt_var *)tree)->varno = al->agvarno;
						((struct qt_var *)tree)->attno = i + 2;
					}
					else
						*tpr = makavar(lnodv[i], al->agvarno, i + 2);

					anyop = TRUE;
#					ifdef xDTR1
					if (tTf(6, 7))
						printree(*tpr, "modified tree");
#					endif
				}
			}
			/* vars is now a map of the variables in the root */
			treemap = vars;
		}
	}

	/* if any changes were made, get rid of the unnecessary links */
	if (anyop)
		chklink(root);
}




modtree(pnode, lnodv, replmap)
struct querytree	**pnode;
struct querytree	*lnodv[];
int			*replmap;
{
	register struct querytree	*tree;
	register int			vars, i;
	struct querytree		*afcn;

	/* point up current node */
	if ((tree = *pnode) == NULL)
		return (0);

	/* examine each by-list for match on this subtree */
	for (i = 0; afcn = lnodv[i]; i++)
	{
		if (sameafcn(tree, afcn->right))
		{
#			ifdef xDTR1
			if (tTf(6, 9))
				printree(tree, "potential Jackpot");
#			endif
			vars = varfind(tree, NULL);
			if (Hnext == Hlimit)
				return (vars);	/* no more room */

			/* record what needs to be done */
			Hnext->trepr = pnode;
			Hnext->byno = i;
			Hnext++;
			*replmap |= vars;
			return (0);
		}
	}
	if (tree->sym.type == VAR)
		return (01 << ((struct qt_var *)tree)->varno);

	/* try the subtrees */
	vars = modtree(&(tree->left), lnodv, replmap);
	if ((vars & *replmap) == 0)
		vars |= modtree(&(tree->right), lnodv, replmap);

	return (vars);
}


chklink(root)
struct querytree	*root;
{
	register struct querytree	*r, *b, *last;

	last = root;

	for (r = last->right; r->sym.type != QLEND; r = r->right)
	{
		/* if this is an EQ node then check for an unnecessary compare */
		if ((b = r->left)->sym.type == BOP && ((struct qt_op *)b)->opno == opEQ)
		{
			if (sameafcn(b->left, b->right))
			{
#				ifdef xDTR1
				if (tTf(6, 5))
					printree(b, "unnec clause");
#				endif
				last->right = r->right;
				continue;
			}
		}
		last = r;
	}
}



sameafcn(q1, q2)
struct querytree *q1, *q2;
{

	register struct querytree	*t1, *t2;
	register int			len;
	int				type;

	t1 = q1;
	t2 = q2;

	if (!(t1 && t2)) 
		return(!(t1 || t2));
	len = (t1->sym.len & 0377) + 2;
	type = t1->sym.type;
	if (type == VAR)
		len = 6;
	if (type == AND)
		len = 2;
	if (!bequal(&t1->sym, &t2->sym, len)) 
		return(0);
	return(sameafcn(t1->left,t2->left) && sameafcn(t1->right,t2->right));
}

varfind(root, aghead)
struct querytree	*root;
struct querytree	*aghead;

/*
**	varfind -- find all variables in the tree pointed to by "root".
**		Examine all parts of the tree except aggregates. For
**		aggregates, ignore simple aggregate and look only
**		at the by-lists of aggregate functions. If the aggregate
**		is "aghead" then ignore it. There is no reason to look
**		at yourself!!!!
**		This routine is called by byeval() to determine
**		whether to link the aggregate to the root tree.
**
**	Curiosity note: since the tree being examined has not been
**	processed by decomp yet, ckvar does not need to be called
**	since the var could not have been altered.
*/

{
	register struct querytree	*tree;
	register int			type;


	if ((tree = root) == NULL)
		return (0);

	if ((type = tree->sym.type) == AGHEAD)
	{
		/* ignore if it matches aghead */
		if (tree == aghead)
			return (0);
		/* if this is an aggregate function, look at bylist */
		tree = tree->left;
		if ((type = tree->sym.type) != BYHEAD)
			return (0);
	}

	if (type == VAR)
		return (1 << ((struct qt_var *)tree)->varno);

	return (varfind(tree->left, aghead) | varfind(tree->right, aghead));
}

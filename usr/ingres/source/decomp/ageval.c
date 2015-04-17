# include	"../pipes.h"
# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"decomp.h"

/*
**	AGEVAL -- evaluate simple aggregate.
**
**	Uses trace flag 5
**
**	Ageval is passed the tree of a simple aggregate,
**	and an array of space to store the results. The
**	amount of space actually allocated is stored in
**	(*result)->sym.len
**
**	If the aggregate is unique (eg. countu, sumu, avgu)
**	or if the aggregate is multi-variable, special
**	processing is done. A temporary relation is formed
**	with a result domain for each single agg in the tree.
**	Decomp is called to retrieve
**	the values to be aggregated into that relation.
**
**	If the aggregate is unique, then duplicates are
**	removed from the temporary relation.
**
**	Next the aggregate is run on either the original relation
**	or on the temporary relation.
**
**	Finally the result is read from OVQP and if a
**	temporary relation was used, it is destroyed.
**
*/


struct querytree *ageval(tree, result)
struct querytree	*tree;		/* root of aggregate */
struct querytree	*result[];	/* space for results */
{
	register struct querytree	*aghead, *resdom, *aop;
	struct querytree		*newtree;
	struct querytree		*lnodv[MAXDOM + 2];
	char				agbuf[AGBUFSIZ];
	int				temp_relnum, i;
	extern int			derror();
	extern struct querytree		*makroot();
	extern struct querytree		*makresdom();
	extern struct querytree		*makavar();

	aghead = tree;
	aop = aghead->left;
	temp_relnum = NORESULT;

	/* if PRIME or multi-var, form aggregate domain in temp relation */
	if (prime(aop) || ((struct qt_root *) aghead)->tvarc > 1)
	{
		initbuf(agbuf, AGBUFSIZ, AGBUFFULL, &derror);

		lnodv[lnode(aop, lnodv, 0)] = 0;

		/* create new tree for retrieve and give it the qualification */
		newtree = makroot(agbuf);
		newtree->right = aghead->right;
		aghead->right = Qle;

		/* put a resdom on new tree for each aop in orig tree */
		/* make each aop in orig tree reference new relation */
		for (i = 0; aop = lnodv[i]; )
		{

			/* create resdom for new tree */
			resdom = makresdom(agbuf, aop);
			((struct qt_res *) resdom)->resno = ++i;
			resdom->right = aop->right;

			/* connect it to newtree */
			resdom->left = newtree->left;
			newtree->left = resdom;

			/* make orig aop reference new relation */
			aop->right = makavar(resdom, FREEVAR, i);
		}

		/* make result relation */
		temp_relnum = mak_t_rel(newtree, "a", -1);

		/* prepare for query */
		mapvar(newtree, 0);
		decomp(newtree, mdRETR, temp_relnum);
		Rangev[FREEVAR].relnum = temp_relnum;
		Sourcevar = FREEVAR;

		/* if prime, remove dups */
		if (prime(aghead->left))
		{
			/* modify to heapsort */
			removedups(FREEVAR);
		}

	}

	Newq = 1;
	Newr = TRUE;

	call_ovqp(aghead, mdRETR, NORESULT);	/* call ovqp with no result relation */
	Newq = 0;

	/* pick up results */
	readagg_result(result);

	/* if temp relation was created, destroy it */
	if (temp_relnum != NORESULT)
		dstr_rel(temp_relnum);

}

prime(aop)
struct querytree	*aop;

/*
**	Determine if an aggregate contains any
**	prime aggregates. Note that there might
**	be more than one aggregate.
*/

{
	register struct querytree	*a;

	a = aop;
	do
	{
		switch (a->sym.value[0])
		{
		  case opCOUNTU:
		  case opSUMU:
		  case opAVGU:
			return (TRUE);
		}
	} while (a = a->left);
	return (FALSE);
}

removedups(var)
int	var;

/*
**	Remove dups from an unopened relation
**	by calling heapsort
*/

{
	register char	*p;
	char		*rangename();

	closer1(var);	/* guarantee that relation has been closed */
	initp();
	p = rangename(var);	/* get name of relation */
#	ifdef xDTR1
	if (tTf(5, 1))
		printf("removing dups from %s\n", p);
#	endif
	setp(p);
	setp("heapsort");
	setp("num");
	call_dbu(mdMODIFY, FALSE);
}

#

/*
**  DISPLAY -- display query corresponding to view, permission, 
**		or intgerity declaration
**
**	Defines:
**		display() -- main function
**		disp() -- routes VIEW, PERMIT, and INTEGRITY 
**		pr_def() -- print view definition
**		pr_integrity() -- print out integrity contstraints on a rel
**
**	Requires:
**		pr_permit() -- to print perissions
**
**	History:
**		11/15/78 (marc) -- written
*/



# include	"../ingres.h"
# include	"../aux.h"
# include	"../catalog.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"../access.h"

# define	QTREE		struct querytree


char		*Resrel;	/* Used in pr_tree() [pr_tree.c] */
char		Mode;		/* 4 - View, 5 - permission, 6 - interity
				 * query print 
				 */

/*
**  DISPLAY -- display query
**
**	Parameters:
**		argc -- number of args in argv
**		argv -- argv [i] == 4 for VIEW, 5 for PERMIT, 6 for INTEGRITY
**				where i % 2 == 0
**		argv [i] -- relation names for which argv[i-1] is Mode
**				where i%2==1
**
**	Returns:
**		0 -- total success
**		err -- error number of last error
**
**	Side Effects:
**		prints out definition of appropriate characteristic of relation
**
**	Requires:
**		disp()
**
**	Called By:
**		dbu/main.c -- DBU controller
**
**	Diagnostics:
**		5401 -- "relation" doesn't exist or
**			does not belong to the user nor the DBA.
**		5403 -- "relation" is not a view [Mode == 4]
**		5404 -- "relation" has no permissions granted [Mode == 5]
**		5405 -- "relation" has no integrity constraints on it [Mode == 6]
**
**	Trace Flags:
**		11, -1
**
**	Syserrs:
**		display: bad openr
**		display: bad argc (odd)
**			bad Mode parameter
**
**	History:
**		11/15/78 (marc) -- written
*/


display(argc, argv)
int		argc;
char		**argv;
{
	register int			ac;
	register char			**av;
	extern struct descriptor	Treedes;
	register			i;
	char				err_array [MAXPARMS]; /* errors are coded
							  * this array for 
							  * printing out
							  * at the end
							  */
	int				err;

#	ifdef xZTR1
	if (tTf(11, -1))
	{
		printf("display: ");
		prargs(argc, argv);
	}
#	endif

	err = 0;
	if (argc % 2 != 0)
		syserr("display: bad argc %d", argc);
	opencatalog("tree", 0);

	for (ac = 0, av = argv; ac < argc; av++, ac++)
	{
		if (atoi(*av, &Mode))
			syserr("display: bad mode \"%s\"", *av);
		Resrel = *++av;
		err_array[ac++] = 0;
		err_array [ac] = disp(*av);
	}
	for (ac = 0, av = argv; ac < argc; av++, ac++)
	{
		if (err_array [ac])
			err = error(5400 + err_array [ac], *av, 0);
	}
	return (err);
}

/* 
** DISP -- display integrity, permit, or define query on a relation
**
**	Finds a relation owned by the user or the DBA and passes
**	the name and owner to the appropritae routine depending on
**	Mode.
**
**	Parameters:
**		relation -- relation on which query is to be printed
**
**	Returns:
**		0 -- success
**		1 -- no such relation, or none seeable by the user.
**		3 -- VIEW mode and relation not a view
**		4 -- PERMIT and no permissions on relation
**		5 -- INTEGRITY mode and no integrity constraints
**
**	Requires:
**		Mode -- for type of query to print
**		pr_def(),
**		pr_prot() -- to print queries
**
**	Called By:
**		display()
**
**	Trace Flags:
**		11, 8
**
**	History:
**		11/15/78 -- (marc) written
**		12/19/78 -- (marc) changed pr_prot call to accomodate
**			S_PROT[12] in relation.relstat
*/

disp(relation)
char	*relation;
{
	struct descriptor		d;
	register			i;

#	ifdef xZTR1
	if (tTf(11, 8))
		printf("disp: relation %s\n", relation);
#	endif

	i = openr(&d, -1, relation);
	if (i > 0)
		return (1);
	else if (i < 0)
		syserr("disp: openr(%s) ret %d", relation, i);
	switch (Mode)
	{
	  case 4:		/* View query */
		if (d.relstat & S_VIEW)
			pr_def(relation, d.relowner);
		else 
			return (3);
		break;

	  case 5:
		if (pr_prot(relation, &d))
			return (4);
		break;

	  case 6:
		if (d.relstat & S_INTEG)
			pr_integrity(relation, d.relowner);
		else
			return (5);
		break;

	  default:
		syserr("disp: Mode == %d", Mode);
	}
	return (0);
}

/*
**  PR_DEF -- Print "define view" query of a view
**
**	Parameters:
**		relation -- relation in question
**		owner -- relowner
**
**	Returns:
**		none
**
**	Side Effects:
**		reads a tree, clears range table
**
**	Requires:
**		pr_tree() [pr_tree.c]
**		Depends on treeid == 0 because there is only on view
**		definition.
**
**	Called By:
**		disp()
**
**	Trace Flags:
**		11, 9
**
**	History:
**		11/15/78 -- (marc) written
*/

pr_def(relation, owner)
char		*relation;
char		*owner;
{
	register QTREE		*t;
	QTREE			*gettree();

#	ifdef xZTR1
	if (tTf(11, 9))
		printf("pr_def(relation=\"%s\", owner=%s)\n", relation, owner);
#	endif

	printf("View %s defined:\n\n", relation);
	clrrange();

	/* Treeid == 0 because views have only one definition */
	t = gettree(relation, owner, mdVIEW, 0);
	pr_range();
	printf("define view ");
	pr_tree(t);
}

/*
**  PR_INTEGRITY -- print out integrity constraints on a relation
**
**	Finds all integrity tuples for this unique relation, and
**	calls pr_int() to print a query from them.
**
**	Parameters:
**		relid -- rel name
**		relowner -- 2 byte owner id
**
**	Returns:
**		none
**
**	Side Effects:
**		file activity, query printing
**
**	Requires:
**		pr_int() -- to print a query given an integrity
**			tuple
**
**	Called By:
**		disp()
**
**	Trace Flags:
**		11, 9
**
**	Syserrs:
**		"pr_integrity: find %d" -- find error
**
**	History:
**		12/17/78 -- (marc) written
*/

pr_integrity(relid, relowner)
char	*relid;
char	*relowner;
{
	extern struct descriptor	Intdes;
	struct tup_id			hitid, lotid;
	struct integrity		key, tuple;
	register			i;


#	ifdef xZTR1
	if (tTf(11, 9))
		printf("pr_integrity(relid =%s, relowner=%s)\n", 
		relid, relowner);
#	endif

	printf("Integrity constraints on %s are:\n\n", relid);
	opencatalog("integrities", 0);

	/* get integrities tuples for relid, relowner */
	clearkeys(&Intdes);
	setkey(&Intdes, &key, relid, INTRELID);
	setkey(&Intdes, &key, relowner, INTRELOWNER);
	if (i = find(&Intdes, EXACTKEY, &lotid, &hitid, &key))
		syserr("pr_integrity: find %d", i);
	for ( ; ; )
	{
		if (i = get(&Intdes, &lotid, &hitid, &tuple, TRUE))
			break;
		if (kcompare(&Intdes, &tuple, &key) == 0)
			pr_int(&tuple);
	}
	if (i != 1)
		syserr("pr_integrity: get %d", i);
}

/*
**  PR_INT -- print an integrity definition given a integrities tuple
**
**	Parameters:
**		g -- integrity tuple
**
**	Returns:
**		none
**
**	Side Effects:
**		prints a query
**		reads a tree
**
**	Requires:
**		gettree(), clrrange(), pr_range()
**
**	Called By:
**		pr_integrity()
**
**	History:
**		12/17/78 (marc) -- written
**		1/9/79   -- (marc) modified to print inttree
*/

pr_int(g)
struct integrity	*g;
{
	register struct integrity	*i;
	register QTREE			*t;
	extern int			Resultvar;
	QTREE				*gettree();

	i = g;
	clrrange();
	t = gettree(i->intrelid, i->intrelowner, mdINTEG, i->inttree);
	printf("Integrity constraint %d -\n\n", i->inttree);
	pr_range();

	printf("define integrity on ");
	pr_rv(Resultvar = i->intresvar);
	printf(" is "); 
	pr_qual(t->right);
	printf("\n\n\n");
}

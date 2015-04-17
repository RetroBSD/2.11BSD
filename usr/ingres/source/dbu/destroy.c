# include	"../ingres.h"
# include	"../aux.h"
# include	"../catalog.h"


/*
**  DESTROY RELATION
**
**	The relations in pv are destroyed.  This involves three steps:
**	1 - remove tuple from relation relation
**	2 - remove tuples from attribute relation
**	3 - unlink physical file
**
**	If the relation is a secondary index, the entry is removed
**	from the index relation, and the primary relation is set to
**	be "not indexed" (unless there is another index on it).
**
**	If the relation has an index, all its indexes are also
**	destroyed.
**
**	If any errors occured while destroying the relations,
**	then the last error number is returned, otherwise
**	0 is returned.
**
**	If any query modification was defined on the relation,
**	the qrymod catalogues are updated.
**
**	If the Standalone variable is non-zero, relation names to
**	destroy include the owner, and you need not own the relation.
**	This is used by purge.
**
**	Uses trace flag 3
**
**	Diagnostics:
**	5201	Attempt to destroy system relation
**	5202	Attempt to destroy nonexistant relation
**
**	History:
**		6/29/79 (eric) (mod 6) -- added Standalone flag.
**		12/15/78 (rse) -- split code into two files. added userdestroy.c
**		12/7/78 (rse) -- replaced call to sys_catalog with inline code
**		11/3/78 (rse) -- flushes tree catalog for views.
*/

int	Standalone;

destroy(pc, pv)
int	pc;
char	**pv;
{
	register int	i, ret;
	register char	*name;

	opencatalog("relation", 2);
	opencatalog("attribute", 2);

	for (ret = 0; (name = *pv++) != -1; )
	{
		if (i = des(name))
			ret = i;
	}
	return (ret);
}


des(name)
char	*name;
{
	register int			i;
	register char			*relname;
	struct tup_id			tid;
	char				newrelname[MAXNAME + 3];
	extern struct descriptor	Reldes, Attdes, Inddes, Treedes;
	struct relation			relt, relk;

	relname = name;
#	ifdef xZTR1
	tTfp(3, -1, "destroy: %s\n", relname);
#	endif

	newrelname[MAXNAME + 2] = 0;

	/* get the tuple from relation relation */
	setkey(&Reldes, &relk, relname, RELID);
	if (Standalone)
		setkey(&Reldes, &relk, &relname[MAXNAME], RELOWNER);
	else
		setkey(&Reldes, &relk, Usercode, RELOWNER);
	if ((i = getequal(&Reldes, &relk, &relt, &tid)) != 0)
	{
		if (i < 0)
			syserr("DESTROY: geteq(rel/%s) %d", relname, i);
		return (error(5202, relname, 0));	/* nonexistant relation */
	}

	/* don't allow a system relation to be destroyed */
	if (relt.relstat & S_CATALOG)
		return (error(5201, relname, 0));	/* attempt to destroy system catalog */

	if ((i = delete(&Reldes, &tid)) != 0)
		syserr("DESTROY: delete(rel) %d", i);

	/*
	** for concurrency reasons, flush the relation-relation page
	** where the tuple was just deleted. This will prevent anyone
	** from being able to "openr" the relation while it is being
	** destroyed. It also allows recovery to finish the destroy
	** it the system crashes during this destroy.
	*/
	if (i = flush_rel(&Reldes, FALSE))
		syserr("destroy:flush rel %d", i);

	purgetup(&Attdes, relt.relid, ATTRELID, relt.relowner, ATTOWNER, 0);

	/*
	**	If this is a user relation, then additional processing
	**	might be needed to clean up indicies, protection constraints
	**	etc.
	*/
	if ((relt.relstat & S_CATALOG) == 0)
		userdestroy(&relt);

	if ((relt.relstat & S_VIEW) == 0)
	{
		if (Standalone)
			bmove(relname, newrelname, MAXNAME + 2);
		else
			ingresname(relname, Usercode, newrelname);
		if (unlink(newrelname) < 0)
			syserr("destroy: unlink(%.14s)", newrelname);
	}
	return (0);
}

# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"qrymod.h"

/*
**  ISSUE -- issue query to rest of system
**
**	This function issues a query to the rest of the INGRES system.
**	It can deal with true queries (that is, a tree), or with a
**	DBU request (which must already be in the pipe).
**
**	The sync from below is read, but not passed up; a pointer
**	to it (or NULL if none) is returned.
**
**	Parameters:
**		execid -- the execid to call.
**		funcid -- the function to call.
**		tree -- NULL -- just sync from below.
**			else -- pointer to tree to issue.
**
**	Returns:
**		The resultcode of the query.
**
**	Side Effects:
**		A query is executed.
**
**	Requires:
**		wrpipe, rdpipe
**		writeqry
**		pipetrwr
**
**	Called By:
**		main
**		qrymod
**
**	Compilation Flags:
**		none
**
**	Trace Flags:
**		13
**
**	History:
**		2/14/79 -- version 6.2 released.
*/

extern struct pipfrmt	Outpipe;

struct retcode *
issue(execid, funcid, tree)
char	execid;
char	funcid;
QTREE	*tree;
{
	register QTREE		*t;
	extern			pipetrwr();
	static struct retcode	rc;
	register int		i;
	register struct retcode	*r;

	t = tree;
	r = &rc;

#	ifdef xQTR2
	if (tTf(13, 1))
		printf("issue:\n");
#	endif
	/* write query tree if given */
	if (t != NULL)
	{
		wrpipe(P_PRIME, &Outpipe, execid, 0, funcid);
		writeqry(t, &pipetrwr);
		wrpipe(P_END, &Outpipe, W_down);
	}

	/* sync with below */
#	ifdef xQTR2
	if (tTf(13, 2))
		printf("issue: response:\n");
#	endif
	rdpipe(P_PRIME, &Outpipe);
	i = rdpipe(P_NORM, &Outpipe, R_down, r, sizeof *r);
	rdpipe(P_SYNC, &Outpipe, R_down);
	if (i == 0)
		return (NULL);
	else
		return (r);
}
/*
**  ISSUEINVERT -- issue a query, but invert the qualification
**
**	This routine is similar to 'issue', except that it issues
**	a query with the qualification inverted.  The inversion
**	(and subsequent tree normalization) is done on a duplicate
**	of the tree.
**
**	Parameters:
**		root -- the root of the tree to issue.
**
**	Returns:
**		pointer to retcode struct.
**
**	Side Effects:
**		'root' is issued.
**
**	Requires:
**		tree -- to create a 'NOT' node.
**		treedup -- to create a copy of the tree.
**		trimqlend, norml -- to normalize after inversion.
**		issue -- to do the actual issue.
**
**	Called By:
**		d_integ
**
**	Trace Flags:
**		none
*/

struct retcode *
issueinvert(root)
QTREE	*root;
{
	register QTREE		*t;
	extern QTREE		*tree(), *treedup();
	extern QTREE		*trimqlend(), *norml();
	register struct retcode	*r;
	extern struct retcode	*issue();

	/* make duplicate of tree */
	t = treedup(root);

	/* prepend NOT node to qualification */
	t->right = tree(NULL, t->right, UOP, 2, opNOT);

	/* normalize and issue */
	t->right = norml(trimqlend(t->right));
	r = issue(EXEC_DECOMP, 0, t);

#	ifdef xQTR3
	/* check for valid return */
	if (r == NULL)
		syserr("issueinvert: NULL rc");
#	endif

	return (r);
}
/*
**  PIPETRWR -- write tree to W_down pipe
**
**	Acts like a 'wrpipe' call, with some arguments included
**	implicitly.
**
**	Parameters:
**		val -- the value to write.
**		len -- the length.
**
**	Returns:
**		same as wrpipe.
**
**	Side Effects:
**		activity on 'Outpipe'.
**
**	Requires:
**		wrpipe
**		Outpipe -- a primed pipe for writing.
**
**	Required By:
**		issue.
*/

pipetrwr(val, len)
char	*val;
int	len;
{
	return (wrpipe(P_NORM, &Outpipe, W_down, val, len));
}
/*
**  NULLSYNC -- send null query to sync Equel program
**
**	On a RETRIEVE to terminal, Equel is left reading the data
**	pipe by the time we get to this point; on an error (which
**	will prevent the running of the original query) we must
**	somehow unhang it.  The solution is to send a completely
**	null query, as so:
**
**			ROOT
**		       /    \
**		    TREE   QLEND
**
**	This will cause OVQP to send a sync back to the EQUEL program,
**	incuring an incredible amount of overhead in the process
**	(but it was his own fault!).
**
**	Parameters:
**		none
**
**	Returns:
**		none
**
**	Side Effects:
**		Space is grabbed from Qbuf.
**		A query is sent.
**
**	Requires:
**		tree() -- to create the relevant nodes.
**		issue() -- to send the query.
**
**	Called By:
**		ferror()
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

struct retcode *
nullsync()
{
	register QTREE		*t;
	extern struct retcode	*issue();
	register struct retcode	*rc;
	QTREE			*tree();

	t = tree(tree(NULL, NULL, TREE, 0), tree(NULL, NULL, QLEND, 0), ROOT, 0);
	rc = issue(EXEC_DECOMP, '0', t);
	return (rc);
}

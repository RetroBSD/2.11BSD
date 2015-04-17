# include	"../ingres.h"
# include	"../catalog.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"../pipes.h"
# include	"qrymod.h"

/*
**  READTREE.C -- query tree input procedures
**
**	This file contains routines to read trees from pipes and
**	catalogs.
**
**	Defines:
**		readqry
**		readsym
**		readtree
**		pipetrrd
**		relntrrd
**		gettree
**		Qmode
**		Resultvar
**
**	Requires:
**		prtree.c
**
**	Required By:
**		main.c
**		view.c
**		protect.c
**		integ.c
**
**	Trace Flags:
**		70
**
**	History:
**		2/14/79 -- version 6.2/0 release.
*/


extern struct pipfrmt	Pipe, Outpipe;

int	Qmode;		/* Query mode */
int	Resultvar;	/* Result variable number */
/*
** READQRY
**
** 	Reads in query symbols from input pipe into core
**	locations and sets up information needed for later 
**	processing.
**
**	Returns ptr to root of querytree
**
**	Locbuf is a 'struct srcid' since that is the largest node of
**	a QMODE, SOURCEID, or RESULTVAR node.
*/

QTREE *
readqry(rdfn, initialize)
int	(*rdfn)();	/* tree read function */
int	initialize;	/* if set, initialize Qbuf */
{
	register struct symbol 	*s;
	struct srcid		locbuf;
	register QTREE		*rtval;
	register char		*p;
	extern			xerror();
	char			*readtree();
	QTREE			*trbuild();

#	ifdef xQTR1
	tTfp(70, 0, "READQRY:\n");
#	endif

	if (initialize)
	{
		/* initialize for new query block */
		initbuf(Qbuf, QBUFSIZ, QBUFFULL, &xerror);
		clrrange(TRUE);
		Resultvar = -1;
		Qmode = -1;
	}
	else
		clrrange(FALSE);

	s = (struct symbol *) &locbuf;

	/* read symbols from input */
	for (;;)
	{
		readsym(s, rdfn);
#		ifdef xQTR1
		if (tTf(70, 1))
		{
			printf("%d, %d, %d/", s->type, s->len, s->value[0] & 0377);
			if (s->type == SOURCEID)
				printf("%.12s", ((struct srcid *)s)->srcname);
			printf("\n");
		}
#		endif
		switch (s->type)
		{
		  case QMODE:
			if (Qmode != -1)
				syserr("readqry: two Qmodes");
			Qmode = s->value[0];
			break;

		  case RESULTVAR:
			if (Resultvar != -1)
				syserr("readqry: two Resultvars");
			Resultvar = s->value[0];
			break;

		  case SOURCEID:
			declare(((struct srcid *)s)->srcvar,
				((struct srcid *)s)->srcname,
				((struct srcid *)s)->srcown,
				((struct srcid *)s)->srcstat);
			break;

		  case TREE:	/* beginning of tree, no more other stuff */
			p = readtree(s, rdfn);
			rtval = trbuild(p);
			if (rtval == NULL)
				ferror(STACKFULL, -1, -1, 0);
			return (rtval);

		  default:
			syserr("readq: bad symbol %d", s->type);
		}
	}
}
/*
** readsym
**	reads in one symbol from pipe into symbol struct.
*/
readsym(dest, rdfn)
char	*dest;		/* if non-zero, pts to allocated space */
int	(*rdfn)();	/* tree read function */
{
	register int		len, t;
	register struct symbol	*p;
	char			*need();

	/* check if enough space for type and len of sym */
	p = (struct symbol *) ((dest != NULL) ? dest : need(Qbuf, 2));
	if ((*rdfn)(p, 2) < 2) 
		goto err3;
	len = p->len & I1MASK;
	t = p->type;
	if (len)
	{
		if (dest == NULL) 
		{
			/* this will be contiguous with above need call */
			need(Qbuf, len);
		}
		if ((*rdfn)(p->value, len) < len) 
			goto err3;
	}

	return;

err3:
	syserr("readsym: read");
}
/*
** readtree
**
** 	reads in tree symbols into a buffer up to a root (end) symbol
**
*/

char *readtree(tresym, rdfn)
struct symbol	*tresym;
int		(*rdfn)();
{
	register QTREE	*nod;
	register char	*rtval;
	char		*need();

	rtval = need(Qbuf, 6);
	bmove(tresym, &(((struct querytree *)rtval)->sym), 2);	/* insert type and len of TREE node */
	for(;;)
	{
		/* space for left & right pointers */
		nod = (QTREE *) need(Qbuf, 4);
		readsym(NULL, rdfn);
#		ifdef xQTR1
		if (tTf(70, 2))
			nodepr(nod, TRUE);
#		endif
		if (nod->sym.type == ROOT)
			return (rtval);
	}
}
/*
**  PIPETRRD -- read tree from R_up pipe
**
**	This routine looks like a rdpipe with only the last two
**	parameters.
**
**	Parameters:
**		ptr -- pointer to data area
**		cnt -- byte count
**
**	Returns:
**		actual number of bytes read
**
**	Side Effects:
**		as with rdpipe()
**
**	Requires:
**		rdpipe()
**
**	Called By:
**		readsym (indirectly via main)
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

pipetrrd(ptr, cnt)
char	*ptr;
int	cnt;
{
	return (rdpipe(P_NORM, &Pipe, R_up, ptr, cnt));
}
/*
**  RELNTRRD -- read tree from 'tree' relation
**
**	This looks exactly like the 'pipetrrd' call, except that info
**	comes from the 'tree' catalog instead of from the pipe.  It
**	must be initialized by calling it with a NULL pointer and
**	the segment name wanted as 'treeid'.
**
**	Parameters:
**		ptr -- NULL -- "initialize".
**			else -- pointer to read area.
**		cnt -- count of number of bytes to read.
**		treeid -- if ptr == NULL, this is the tree id,
**			otherwise this parameter is not supplied.
**
**	Returns:
**		count of actual number of bytes read.
**
**	Side Effects:
**		activity in database.
**		static variables are adjusted correctly.  Note that
**			this routine can be used on only one tree
**			at one time.
**
**	Requires:
**		Treedes -- a relation descriptor for the "tree" catalog
**			open for read.
**		clearkeys, setkey, getequal
**
**	Called By:
**		readsym (indirectly via main)
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		several access method error returns
**		on initialization if the specified treeid is not
**			in the catalog.
*/

relntrrd(ptr, cnt, treerelid, treeowner, treetype, treeid)
char	*ptr;
int	cnt;
char	*treerelid;
char	*treeowner;
char	treetype;
int	treeid;
{
	static struct tree	trseg;
	static char		*trp;
	static int		seqno;
	register char		*p;
	register int		n;
	register int		i;
	struct tree		trkey;
	struct tup_id		tid;

	p = ptr;
	n = cnt;

	if (p == NULL)
	{
		/* initialize -- make buffer appear empty */
		trp = &trseg.treetree[sizeof trseg.treetree];
		bmove(treerelid, trseg.treerelid, MAXNAME);
		bmove(treeowner, trseg.treeowner, 2);
		trseg.treetype = treetype;
		trseg.treeid = treeid;
		seqno = 0;
		opencatalog("tree", 0);

#		ifdef xQTR2
		if (tTf(70, 6))
			printf("relntrrd: n=%.12s o=%.2s t=%d i=%d\n",
			    treerelid, treeowner, treetype, treeid);
#		endif

		return (0);
	}

	/* fetch characters */
	while (n-- > 0)
	{
		/* check for segment empty */
		if (trp >= &trseg.treetree[sizeof trseg.treetree])
		{
			/* then read new segment */
			clearkeys(&Treedes);
			setkey(&Treedes, &trkey, &trseg.treerelid, TREERELID);
			setkey(&Treedes, &trkey, &trseg.treeowner, TREEOWNER);
			setkey(&Treedes, &trkey, &trseg.treetype, TREETYPE);
			setkey(&Treedes, &trkey, &trseg.treeid, TREEID);
			setkey(&Treedes, &trkey, &seqno, TREESEQ);
			seqno++;
			if ((i = getequal(&Treedes, &trkey, &trseg, &tid)) != 0)
				syserr("relnrdtr: getequal %d", i);
			trp = &trseg.treetree[0];
		}

		/* do actual character fetch */
		*p++ = *trp++;
	}

	return (cnt);
}
/*
**  GETTREE -- get tree from 'tree' catalog
**
**	This function, given an internal treeid, fetches and builds
**	that tree from the 'tree' catalog.  There is nothing exciting
**	except the mapping of variables, done by mapvars().
**
**	Parameters:
**		treeid -- internal id of tree to fetch and build.
**		init -- passed to 'readqry' to tell whether or not
**			to initialize the query buffer.
**
**	Returns:
**		Pointer to root of tree.
**
**	Side Effects:
**		file activity.  Space in Qbuf is used up.
**
**	Requires:
**		relntrrd -- to initialize for readqry.
**		readqry -- to read and build the tree.
**		mapvars -- to change the varno's in the tree after
**			built to avoid conflicts with variables in
**			the original query.
**
**	Called By:
**		view
**		integrity (???)
**		protect (???)
**		d_view
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
*/

QTREE *
gettree(treerelid, treeowner, treetype, treeid, init)
char	*treerelid;
char	*treeowner;
char	treetype;
int	treeid;
int	init;
{
	register QTREE	*t;
	extern int	relntrrd();
	register int	i;

	/* initialize relntrrd() for this treeid */
	relntrrd(NULL, 0, treerelid, treeowner, treetype, treeid);

	/* read and build query tree */
	t = readqry(&relntrrd, init);

	/* remap varno's to be unique */
	if (!init)
		mapvars(t);

	return (t);
}

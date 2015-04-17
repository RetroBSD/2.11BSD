#
# include	"../ingres.h"
# include	"../catalog.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"../pipes.h"

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
**		relntrrd
**		gettree
**		declare
**		clrrange
**
**	Required By:
**		display.c
**
**	Errors:
**		5410 -- Query buffer full
**		5411 -- Tree stack overflow
**
**	History:
**		8/23/78 (eric) -- release 6.2/0
**		11/15/78 (marc) -- modified for DBU
**		1/18/79 (rse) -- moved trbuild to iutil lib
*/


extern struct pipfrmt	Pipe, Outpipe;

/* if DBU's other than DISPLAY choose to use readtree.c,
 * QBUFFULL should be passed to gettree() as a parameter 
 */

# define	QBUFFULL	5410
# define	STACKFULL	5411
# define	QTREE		struct querytree

/* this structure should be identical to the one in pr_prot.c */
struct rngtab
{
	char	relid [MAXNAME];
	char	rowner [2];
	char	rused;
};

extern struct rngtab	Rangev [];
extern int		Resultvar;


/* check out match for these two in decomp.h */
# define	QBUFSIZ		2000		/* query buffer */



char		Qbuf[QBUFSIZ];		/* tree buffer space */
int		Qmode;			/* type of query */


extern struct descriptor	Treedes;


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
**
**	Trace Flags:
**		12, 0
**
*/

QTREE *
readqry(rdfn)
int	(*rdfn)();	/* tree read function */
{
	register struct symbol 	*s;
	struct srcid		locbuf;
	register QTREE		*rtval;
	register char		*p;
	int			berror();
	char			*readtree();
	QTREE			*trbuild();

	/* initialize for new query block */
	initbuf(Qbuf, QBUFSIZ, QBUFFULL, berror);
	clrrange();
	Resultvar = -1;
	Qmode = -1;

	s = (struct symbol *) &locbuf;

	/* read symbols from input */
	for (;;)
	{
		readsym(s, rdfn);
#		ifdef xZTR1
		if (tTf(12, 0))
			printf("readqry symbol: type %o==%d\n", s->type);
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
				((struct srcid *)s)->srcown);
			break;

		  case TREE:	/* beginning of tree, no more other stuff */
			p = readtree(s, rdfn);
			rtval = trbuild(p);
			if (rtval == NULL)
				berror(STACKFULL);
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
	char *need();

	rtval = need(Qbuf, 6);
	bmove(tresym, &(((struct querytree *)rtval)->sym), 2);	/* insert type and len of TREE node */
	for(;;)
	{
		/* space for left & right pointers */
		nod = (QTREE *) need(Qbuf, 4);
		readsym(NULL, rdfn);
		if (nod->sym.type == ROOT)
			return (rtval);
	}
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
**		readsym (indirectly via pr_def, pr_prot)
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		several access method error returns
**		on initialization if the specified treeid is not
**			in the catalog.
**
**	History:
**		2/23/78 (eric) -- specified (not written)
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
**	that tree from the 'tree' catalog.  
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
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		none
**
**	History:
**		3/2/78 (eric) -- 'init' parameter added (defaulted to
**			false before).
**		2/25/78 (eric) -- written
*/

QTREE *
gettree(treerelid, treeowner, treetype, treeid)
char	*treerelid;
char	*treeowner;
char	treetype;
int	treeid;
{
	register QTREE	*t;
	extern int	relntrrd();
	register int	i;

	/* initialize relntrrd() for this treeid */
	relntrrd(NULL, 0, treerelid, treeowner, treetype, treeid);

	/* read and build query tree */
	t = readqry(&relntrrd);

	return (t);
}



/*
** BERROR -- Buffer overflow routine.
**
**	Called on Tree buffer overflow; returns via reset()
**	to DBU controller after sending back an error to process 1.
**
**	Trace flag 12, 1
*/

berror(err)
int	err;
{
#	ifdef xZTR1
	if (tTf(12, 1))
		printf("berror(%d)\n", err);
#	endif
	error(err, 0);
	reset();
}

/*
** declare -- declare a range table entry
**
**	Trace flag 12, 2
**
*/

declare(var, name, owner)
int		var;
char		*name;
char		*owner;
{
	register struct rngtab	*r;
	register		v;

	v = var;
	if (v > MAXNAME)
		syserr("declare: bad var number %d", v);

#	ifdef xZTR1
	if (tTf(12, 2))
		printf("declare(var=%d, name=\"%s\", owner=\"%s\")\n",
		var, name, owner);
#	endif

	r = &Rangev [v];
	r->rused++;
	bmove(name, r->relid, sizeof r->relid);
	bmove(owner, r->rowner, sizeof r->rowner);
}

/*
** CLRRANGE -- clear range table entries
**
** 	Trace flag 12, 3
*/

clrrange()
{
	register		i;
	register struct rngtab	*r;

#	ifdef xZTR1
	if (tTf(12, 3))
		printf("clrrange()\n");
#	endif

	r = Rangev;
	for (i = 0; i <= MAXVAR; )
		r [i++].rused = 0;
}

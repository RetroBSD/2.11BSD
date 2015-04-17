# include	"../ingres.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"decomp.h"
# include	"../pipes.h"


/*
** READQUERY
**
** 	Reads in query symbols from input pipe into core
**	locations and sets up information needed for later 
**	processing.
**
**	Returns:
**		pointer to start of tree
**
**	Side Effects:
**
**		Sets Qmode to mode of query
**		Resultvar gets result variable number or -1
**		Range table gets initialized.
*/

struct pipfrmt Inpipe;

struct querytree *readqry()
{
	register struct symbol 		*s;
	register struct querytree	*q;
	register int			mark;
	int				i;
	struct querytree		*readnode();

#	ifdef xDTR1
	if (tTf(1, -1))
		printf("READQUERY:\n");
#	endif

	Resultvar = -1;
	Qmode = -1;
	mark = markbuf(Qbuf);
	for (;;)
	{
		freebuf(Qbuf, mark);
		q = readnode(Qbuf);
		s = &(q->sym);
#		ifdef xDTR1
		if (tTf(1, 1))
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
			Qmode = s->value[0];
			break;

		  case RESULTVAR:
			Resultvar = s->value[0];
			break;

		  case SOURCEID:
			i = ((struct srcid *)s)->srcvar;
			Rangev[i].relnum = rnum_assign(((struct srcid *)s)->srcname);
			break;

		  case TREE:
			readtree(Qbuf);
			return (q);

		  default:
			syserr("readq: bad symbol %d", s->type);
		}
	}
}

struct querytree *readnode(buf)
char	*buf;
{
	register int			len, user;
	register struct querytree	*q;
	extern struct querytree		*need();

	q = need(buf, 6);	/* space for left,right ptrs + type & len */

	/* read in type and length */
	if (rdpipe(P_NORM, &Inpipe, R_up, &(q->sym), 2) < 2) 
		goto err3;
	len = q->sym.len & I1MASK;

	if (len)
	{
		need(buf, len);
		if (rdpipe(P_NORM, &Inpipe, R_up, q->sym.value, len) < len) 
			goto err3;
		return (q);
	}

	/* if type is and, root, or aghead, allocate 6 or 8 byte for var maps */
	user = FALSE;
	switch (q->sym.type)
	{

	  case ROOT:
		user = TRUE;
	  case AGHEAD:
		len = 8;
		need(buf, len);
		((struct qt_root *)q)->rootuser = user;
		break;

	  case AND:
		len = 6;
		need(buf, len);
	}
	q->sym.len = len;
	return (q);
err3:
	syserr("readsym: read error on R_up");
}


/*
 * readtree
 *
 * 	reads in tree symbols into a buffer up to an end root symbol
 *
 */
readtree(buf)
char	*buf;
{
	register struct querytree 	*nod;

	do
	{
		nod = readnode(buf);
#		ifdef xDTR1
		if (tTf(1, 2))
		{
			printf("\t");
			writenod(nod);
		}
#		endif
	} while (nod->sym.type != ROOT);
	clearpipe();	/* This is needed in case query was multiple of 120 bytes */
}

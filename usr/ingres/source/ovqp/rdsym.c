# include	"../ingres.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"ovqp.h"

extern struct pipfrmt	Inpipe, Outpipe;

char *rdsym()
/*
**	Reads one symbol from the pipe
**	and inserts it in the next slot
**	on the Ovqpbuf.
**
**	returns address of list element.
*/
{
	register char	*next;
	register int	i;
	register int	j;
	extern char	*Ovqpbuf;
	char		*need();

	next = need(Ovqpbuf, 2);	/* get two bytes for type and length */

	if (j = rdpipe(P_NORM, &Inpipe, R_decomp, next, 2) !=2)	/* get type and length */
		syserr("rdsym:bad rdpipe %d", j);
	i = *(next + 1) & 0377;	/* get length of symbol */
#	ifdef xOTR1
	if (tTf(29, 0))
		printf("RDSYM: sym %2.d  len=%3.d\t",*next, i);
#	endif

	if (i)
	{
		/* if i is odd round up and allocate enought space. */
		/* alloc will guarantee an even byte adress */

		need(Ovqpbuf, i);		/* get space for value */
		if (j = rdpipe(P_NORM, &Inpipe, R_decomp, next+2, i) != i)
			syserr("rdsym:bad rdpipe of %d", j);
	}

#	ifdef xOTR1
	if (tTf(29, 1))
		if (((struct symbol *)next)->type != VAR)
			prsym(next);
#	endif
	
	if (Qvpointer >= MAXNODES)
		ov_err(NODOVFLOW);
	Qvect [Qvpointer++] = (struct symbol *) next;
	return (next);
}

/*
**  Sym_ad -- reasonable way of getting the address
**	of the last symbol read in.
**
*/

struct symbol **sym_ad()
{
	return (&Qvect [Qvpointer - 1]);
}



putvar(sym, desc, tup)
struct symbol	*sym;
char		tup[];
struct descriptor	*desc;

/*
**	putvar is called to insert a tuple
**	pointer into the list. Desc is a
**	descriptor struc of an open relation.
**	Tup is the tuple buffer for the relation.
*/

{
	register struct descriptor	*d;
	register char			*next;
	register int			attnum;
	extern char			*Ovqpbuf;
	char				*need();

	next = (char *) sym;
	d = desc;


	attnum = ((struct stacksym *)next)->value[0] & 0377;
	next = need(Ovqpbuf, 4);	/* get four more bytes */

	if (attnum)
	{
		/* attnum is a real attribute number */
		if (attnum > d->relatts)
			syserr("rdsym:bad att %d in %.12s", attnum, d->relid);
		((struct stacksym *)next)->type = d->relfrmt[attnum];
		((struct stacksym *)next)->len = d->relfrml[attnum];
		((struct stacksym *)next)->value[0] = (int) &tup[0] + d->reloff[attnum];	/* address within tuple buffer location */
	}
	else
	{
		/* attnum refers to the tuple id */
		((struct stacksym *)next)->type = TIDTYPE;
		((struct stacksym *)next)->len = TIDLEN;	/* tids are longs */
		((struct stacksym *)next)->value[0] = (int) &Intid;	/* address of tid */
	}
#	ifdef xOTR1
	if (tTf(29, 3))
		prsym(next - 4);
#	endif
	return;
}

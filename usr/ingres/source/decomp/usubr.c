# include	"../ingres.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"../access.h"
# include	"decomp.h"


/**
 **	usubr.c
 **
 **	utility routines to handle setting up params, etc for DBU calls
 **/



/*
 *	generate domain names, formats
 */
domnam(lnp, pre)
struct querytree 	**lnp;
char			*pre;
{

	register char 	suf, *n, **p;
	char		name[MAXNAME];

	suf = '1';
	for (n=name; *n++= *pre++;);
	*n = '\0';
	n--;
	for (p=(char **)lnp; *p; p++)
	{
		*n = suf++;
		setp(name);
		setp(format(*p));
	}
	return;
}


/*
**	Parameter buffer for use in calling dbu routines.
**	The buffer is manipulated by setp() and initp().
**	It is interrogated by call_dbu().
*/

char	*Pv[PARGSIZE];		/* array of pointers to parameters */
char	Pbuffer[PBUFSIZE];	/* space for actual parameters */

int	Pcount;			/* next free char in Pbuffer */
int	Pc;			/* number of parameters */

initp()

/*
**	Init parameter buffer to have no params.
*/

{
	Pc = 0;
	Pcount = 0;
}


setp(s)
char	*s;

/*
**	Copy the paramer s into the parameter buffer.
*/

{
	register char	*r, *p;
	register int	i;

	r = s;
	i = Pcount;
	p = &Pbuffer[i];

	/* copy parameter */
	Pv[Pc++] = p;
	while (*p++ = *r++)
		i++;

	Pcount = ++i;

	/* check for overflow */
	if (i > PBUFSIZE || Pc >= PARGSIZE)
		syserr("setp overflow chars=%d,args=%d", i, Pc);
}

/*
 *	gets format in ascii from RESDOM or AOP node
 */
char *
format(p)
struct querytree *p;
{

	static char	buf[10];
	register char	*b;

	b = buf;

	*b++ = ((struct qt_var *)p)->frmt;
	itoa(((struct qt_var *)p)->frml & I1MASK, b);
	return(buf);
}


/*
 *	makes list of nodes (depth first)
 */
lnode(nod, lnodv, count)
struct querytree 	*nod, *lnodv[];
int			count;
{
	register struct querytree	*q;
	register int			i;

	i = count;
	q = nod;

	if (q && q->sym.type != TREE)
	{
		i = lnode(q->left, lnodv, i);
		lnodv[i++] = q;
	}
	return(i);
}




dstr_rel(relnum)
int	relnum;

/*
**	Immediately destroys the relation if it is an _SYS
*/

{
	initp();
	dstr_mark(relnum);
	dstr_flush(0);
}


dstr_mark(relnum)
int	relnum;

/*
**	Put relation on list of relations to be
**	destroyed. A call to initp() must be
**	made before any calls to dstr_mark().
**
**	A call to dstr_flush() will actually have
**	the relations exterminated
*/

{
	register char	*p;
	char		*rnum_convert();

	if (rnum_temp(relnum))
	{
		p = rnum_convert(relnum);
#		ifdef xDTR1
		if (tTf(3, 4))
			printf("destroying %s\n", p);
#		endif
		setp(p);
		specclose(relnum);	/* guarantee that relation is closed and descriptor destroyed */
		rnum_remove(relnum);
	}
}


dstr_flush(errflag)
int	errflag;

/*
**	call destroy if any relations are
**	in the parameter vector
*/

{
	if (Pc)
		call_dbu(mdDESTROY, errflag);
}


mak_t_rel(tree, prefix, rnum)
struct querytree	*tree;
char			*prefix;
int			rnum;

/*
**	Make a temporary relation to match
**	the target list of tree.
**
**	If rnum is positive, use it as the relation number,
**	Otherwise allocate a new one.
*/

{
	char		*lnodv[MAXDOM + 1];
	register int	i, relnum;

	initp();
	setp("0");	/* initial relstat field */
	relnum = rnum < 0 ? rnum_alloc() : rnum;
	setp(rnum_convert(relnum));
	lnodv[lnode(tree->left, lnodv, 0)] = 0;
	domnam(lnodv, prefix);

	call_dbu(mdCREATE, FALSE);
	return (relnum);
}


struct querytree **mksqlist(tree, var)
struct querytree	*tree;
int			var;
{
	register struct querytree	**sq;
	register int			i;
	static struct querytree		*sqlist[MAXRANGE];

	sq = sqlist;
	for (i = 0; i < MAXRANGE; i++)
		*sq++ = 0;

	sqlist[var] = tree;
	return (sqlist);
}




long rel_pages(tupcnt, width)
long	tupcnt;
int	width;
{
	register int	tups_p_page;

	tups_p_page = (PGSIZE - 12) / (width + 2);
	return ((tupcnt + tups_p_page - 1) / tups_p_page);
}

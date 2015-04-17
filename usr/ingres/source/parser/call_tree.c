# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"parser.h"

struct pipfrmt	Pipebuf;	/* define pipe buffer structure
				also referenced in rngsend and treepr */
/*
** CALL_TREE -- send a tree to decomp (or qrymod)
*/
call_tree(qmode1, execid)
int	qmode1;
int	execid;
{
	register int	qmode;
	register int	retr;

	qmode = qmode1;
#	ifdef	xPTR2
	tTfp(26, 0, "call_tree: qm=%d\tex=%d\n", qmode, execid);
#	endif
	wrpipe(P_PRIME, &Pipebuf, execid, 0, qmode);
	writesym(QMODE, 2, &qmode1);
	if (Resrng)
	{
#		ifdef	xPTR2
		tTfp(26, 1, "resvarno:%d\n", Resrng->rentno);
#		endif
		writesym(RESULTVAR, 2, &Resrng->rentno);
	}
	rngsend();
	treepr(Lastree);
	wrpipe(P_END, &Pipebuf, W_down);

	/* optomize test for these two conditions */
	retr = (qmode == mdRETR || qmode == mdRET_UNI);

	/* output sync block to EQUEL if necessary */
	if (Equel && retr && Resrng == 0)
		syncup();
	syncdn();
	if (retr)
	{
		if (Resrng)
		{
			if (Relspec)
			{
				setp(Resrng->relnm);
				setp(Relspec);
				call_p(mdMODIFY, EXEC_DBU);
			}
		}
		else if (!Equel)
			printeh();
	}
}

/*
** WRITESYM -- does the physical write stuff, etc.
**	for a normal tree symbol.  (i.e. other
**	than a SOURCEID--range table entry
*/
writesym(typ, len, value)
int	typ;
int	len;
char	*value;
{
	struct symbol	sym;

	sym.type = typ & I1MASK;
	sym.len = len & I1MASK;
	wrpipe(P_NORM, &Pipebuf, W_down, &sym, 2);
	if (value != NULL)
		wrpipe(P_NORM, &Pipebuf, W_down, value, len & I1MASK);
}

/*
**  RNGWRITE -- does the physical write operation, etc.
**	for SOURCEID symbol (i.e. a range table entry).
*/
rngwrite(r)
struct rngtab	*r;
{
	register struct rngtab	*rptr;
	struct srcid		s;
	register struct srcid	*sp;

	rptr = r;
	sp = &s;
	sp->type = SOURCEID;
	sp->len = sizeof(*sp) - 2;
	sp->srcvar = rptr->rentno;
	pmove(rptr->relnm, sp->srcname, MAXNAME, ' ');
	bmove(rptr->relnowner, sp->srcown, 2);
	sp->srcstat = rptr->rstat;
	wrpipe(P_NORM, &Pipebuf, W_down, sp, sizeof(*sp));
#	ifdef	xPTR3
	if (tTf(26, 2))
		printf("srcvar:%d\tsrcname:%.12s\tsrcown:%.2s\tsrcstat:%o\n", sp->srcvar, sp->srcname, sp->srcown, sp->srcstat);
#	endif
}

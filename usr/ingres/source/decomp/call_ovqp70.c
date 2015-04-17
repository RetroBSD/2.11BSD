# include	"../ingres.h"
# include	"../aux.h"
# include	"../unix.h"
# include	"../access.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"decomp.h"
# include	"../ovqp/ovqp.h"


/*
** CALL_OVQP -- Routines which interface to the One Variable Query Processor.
**
**	This file contains the routines associated with sending queries
**	and receiving results from OVQP. The interface to these routines is
**	still messy. Call_ovqp is given the query, mode, and result relation
**	as parameters and gets the source relation, and two flags
**	(Newq, Newr) as globals. The routines include:
**
**	Call_ovqp -- Sends a One-var query to ovqp and flushes the pipe.
**
**	Readresult -- Reads the result from a one-var query.
**
**	Endovqp    -- Informs ovqp that the query is over. Helps to synchronize
**			the batch file (if any).
**
*/


int		Qvptr;		/* index into available Qvect space in ovqpnod() */
struct symbol	*Qvect[MAXNODES];
char		*Ovqpbuf;

char		D_ovqp70 = 1;	/* used for loading only. forces call_ovqp70 to be
			** loaded instead of call_ovqp
			*/



call_ovqp(qtree, mode, resultnum)
struct querytree 	*qtree;
int			mode;
int			resultnum;

/*
** Call_ovqp -- send query down pipe to ovqp and flush pipe.
**	Inputs are:
**		mode		retrieve, append, etc.
**		resultnum	result relation id
**		qtree		the query
**		Sourcevar	(global) if >= 0 then source var
**		Newq		send NEWQ symbol
**		Newr		send NEWR symbol
*/

{
	register struct querytree	*tree;
	register int			i;
	char				*rangename();
	struct symbol			s;
	extern int			derror();
	extern int			Batchupd;
	extern struct descriptor	Inddes;
	char				ovqpbuf[LBUFSIZE];
	struct descriptor		*readopen(), *specopen();

#	ifdef xOTM
	if (tTf(76, 1))
		timtrace(7, 0);
#	endif

	tree = qtree;
#	ifdef xDTR1
	if (tTf(8, -1))
	{
		if (tTf(8, 0))
			printf("CALL_OVQP-\n");
		if (tTf(8, 1))
		{
			if (Newq)
				printree(tree, "new query to ovqp");
			else
				printf("query same as previous\n");
		}
		if (tTf(8, 2))
		{
			printf("Sourcevar=%d\t", Sourcevar);
			if (Sourcevar >= 0)
				printf("relid=%s\t", rangename(Sourcevar));
			if (resultnum >= 0)
				printf("Resultname=%s", rnum_convert(resultnum));
			if (((struct qt_root *)tree)->rootuser)
				printf(", userqry");
			printf("\n");
		}
	}
#	endif



	/* assign mode of this query */
	Ov_qmode = mode;

	if (Newr)
	{
		Newr = FALSE;
	}

	if (resultnum >= 0)
		Result = specopen(resultnum);
	else
		Result = NULL;

	if (Sourcevar >= 0)
		Source = readopen(Sourcevar);
	else
		Source = NULL;

	/* assume this will be direct update */
	Userqry = Buflag = FALSE;

	if (((struct qt_root *)tree)->rootuser)
	{
		Userqry = TRUE;
		/* handle batch file */
		if (Result && Ov_qmode != mdRETR)
		{
			if (Batchupd || Result->relindxd > 0)
			{
				if (Bopen == 0)
				{
					if (Result->relindxd > 0)
						opencatalog("indexes", 0);
					if (i = openbatch(Result, &Inddes, Ov_qmode))
						syserr("call_ovqp:opn batch %d", i);
					Bopen = TRUE;
				}
				Buflag = TRUE;
			}
		}
	}

	/*  now write the query list itself  */
	if (Newq)
	{
		Ovqpbuf = ovqpbuf;
		initbuf(Ovqpbuf, LBUFSIZE, LISTFULL, &derror);
		Qvptr = 0;
		Alist = Bylist = Qlist = Tlist = NULL;
		Targvc = ((struct qt_root *)tree)->lvarc;
		Qualvc = bitcnt(((struct qt_root *)tree)->rvarm);
		Agcount = 0;

		if (tree->sym.type == AGHEAD)
		{
			Alist = &Qvect[0];
			if (tree->left->sym.type == BYHEAD)
			{
				mklist(tree->left->right);
				ovqpnod(tree->left);	/* BYHEAD node */
				Bylist = &Qvect[Qvptr];
				mklist(tree->left->left);
			}
			else
				mklist(tree->left);
		}
		else
		{
			if (tree->left->sym.type != TREE)
			{
				Tlist = &Qvect[0];
				mklist(tree->left);
			}
		}

		/* now for the qualification */
		ovqpnod(tree);	/* ROOT node */

		if (tree->right->sym.type != QLEND)
		{
			Qlist = &Qvect[Qvptr];
			mklist(tree->right);
		}
		ovqpnod(Qle);	/* QLEND node */
	}

	/* Now call ovqp */
	if (strategy())
	{

#		ifdef xOTM
		if (tTf(76, 2))
			timtrace(9, 0);
#		endif

		i = scan();	/* scan the relation */

#		ifdef xOTM
		if (tTf(76, 2))
			timtrace(10, 0);
#		endif

	}
	else
		i = EMPTY;

#	ifdef xOTM
	if (tTf(76, 1))
		timtrace(8, 0);
#	endif


	/* return result of query */
	return (i == NONEMPTY);	/* TRUE if tuple satisfied */
}



struct retcode	Retcode;	/* return code structure */

endovqp(ack)
int	ack;

/*
** Endovqp -- Inform ovqp that processing is complete. "Ack" indicates
**	whether to wait for an acknowledgement from ovqp. The overall
**	mode of the query is sent followed by an EXIT command.
**
**	Ovqp decides whether to use batch update or not. If ack == ACK
**	then endovqp will read a RETVAL symbol from ovqp and return
**	a token which specifies whether to call the update processor or not.
*/

{
	register int		i, j;
	extern int		Qry_mode;

	if (ack != RUBACK)
	{
		if (Equel && Qry_mode == mdRETTERM)
			equeleol(EXIT);	/* signal end of retrieve to equel process */
	}

	i = NOUPDATE;

	if (ack == ACK)
	{
		if (Bopen)
		{
			closebatch();
			Bopen = FALSE;
			i = UPDATE;
		}
	}
	else
	{
		if (Bopen)
		{
			rmbatch();
			Bopen = FALSE;
		}
	}

	Retcode.rc_tupcount = Tupsfound;

	closecatalog(FALSE);

	return (i);
}



ovqpnod(n)
struct querytree	*n;

/*
**	Add node n to ovqp's list
*/

{
	register struct symbol		*s;
	register struct querytree	*q;
	register int			i;
	extern struct querytree		*ckvar(), *need();

	q = n;
	s = &q->sym;

	/* VAR nodes must be specially processed */
	if (s->type == VAR)
	{
		/* locate currently active VAR */
		q = ckvar(q);

		/* Allocate an ovqp var node for the VAR */
		s = (struct symbol *) need(Ovqpbuf, 8);
		s->len = 6;
		s->value[0] = ((struct qt_var *)q)->attno;
		((struct qt_v *)s)->vtype = ((struct qt_var *)q)->frmt;
		((struct qt_v *)s)->vlen = ((struct qt_var *)q)->frml;

		/* If VAR has been substituted for, get value */
		if (((struct qt_var *)q)->valptr)
		{

			/* This is a substituted variable */
			if (((struct qt_var *)q)->varno == Sourcevar)
				syserr("ovqpnod:bd sub %d,%d", ((struct qt_var *)q)->varno, Sourcevar);

			s->type = S_VAR;
			((struct qt_v *)s)->vpoint = (int *) ((struct qt_var *)q)->valptr;
		}
		else
		{
			/* Var for one variable query */
			if (((struct qt_var *)q)->varno != Sourcevar)
				syserr("ovqpnod:src var %d,%d", ((struct qt_var *)q)->varno, Sourcevar);
			s->type = VAR;
			if (((struct qt_var *)q)->attno)
				((struct qt_v *)s)->vpoint = (int *) (Intup + Source->reloff[((struct qt_var *)q)->attno]);
			else
				((struct qt_v *)s)->vpoint = (int *) &Intid;
		}

	}
	if (s->type == AOP)
		Agcount++;

	/* add symbol to list */
	if (Qvptr > MAXNODES - 1)
		ov_err(NODOVFLOW);
	Qvect[Qvptr++] = s;
}


readagg_result(result)
struct querytree	*result[];

/*
**
*/

{
	register struct querytree	**r, *aop;
	register int			i;


	Tend = Outtup;
	r = result;

	while (aop = *r++)
	{
		i = aop->sym.len & I1MASK;

		if (aop->sym.type == CHAR)
			pad(Tend, i);

		bmove(Tend, aop->sym.value, i);

		Tend += i;
#		ifdef xDTR1
		if (tTf(8, 3))
			writenod(aop);
#		endif
	}
}


ov_err(code)
int	code;
{
	derror(code);
}


struct descriptor *openindex(name)
char	*name;
{
	register struct descriptor	*d;
	register int			varno;
	struct descriptor		*readopen();

	varno = SECINDVAR;
	Rangev[varno].relnum = rnum_assign(name);
	d = readopen(varno);
	return (d);
}


/*
**	Use "closer()" for closing relations. See
**	desc_close in openrs.c for details.
*/
extern int	closer();
int		(*Des_closefunc)()	= closer;

init_decomp()
{
	static struct accbuf	xtrabufs[12];

	acc_addbuf(xtrabufs, 12);
}


startdecomp()
{
	/* called at the start of each user query */
	Retcode.rc_tupcount = 0;
	initrange();
	rnum_init();
	startovqp();
}



/*
** Files_avail -- returns how many file descriptors are available
**	for decomposition. For decomp combined with ovqp, the fixed
**	overhead for files is:
**
**		4 -- pipes
**		1 -- relation relation
**		1 -- attribute relation
**		1 -- standard output
**		1 -- concurrency
**		1 -- indexes relation
**
**	Optional overhead is:
**
**		1 -- for Equel data pipe
**		1 -- for batch file on updates
*/

files_avail(mode)
int	mode;
{
	register int	i;

	i = MAXFILES - 9;
	if (Equel)
		i--;
	if (mode != mdRETR)
		i--;
	return (i);
}

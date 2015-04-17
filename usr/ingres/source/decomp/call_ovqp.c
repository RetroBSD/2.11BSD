# include	"../pipes.h"
# include	"../ingres.h"
# include	"../aux.h"
# include	"../unix.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"decomp.h"


/*
** CALL_OVQP -- Routines which interface to the One Variable Query Processor.
**
**	This file contains the routines associated with sending queries
**	and receiving results from OVQP. The interface to these routines is
**	still messy. Call_ovqp is given the query, mode, and result relation
**	as parameters and gets the source relation, and two flags
**	(Newq, Newr) as globals. The routines include:
**
**	Call_ovqp -- Sends a One-var query to ovqp, flushes the pipe,
**			and reads result from ovqp.
**
**	Endovqp    -- Informs ovqp that the query is over. Helps to synchronize
**			the batch file (if any).
**
**	Pipesym    -- Writes a token symbol to ovqp.
*/

struct pipfrmt Outpipe, Inpipe;
struct retcode	Retcode;	/* return code structure */



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
	struct retcode			ret;
	struct symbol			s;
	int				j;

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
			if (((struct qt_root *) tree)->rootuser)
				printf(", userquery");
			printf("\n");
		}
	}
#	endif



	/*  header information needed by OVQP for each query  */
	wrpipe(P_PRIME, &Outpipe, EXEC_OVQP, 0, 0);
	pipesym(QMODE, 1, mode);

	if (Newq)
		pipesym(CHANGESTRAT, 0);

	if (Newr)
	{
		pipesym(REOPENRES, 0);
		Newr = FALSE;
	}

	if (Sourcevar >= 0)
	{
		s.type = SOURCEID;
		s.len = MAXNAME;
		wrpipe(P_NORM, &Outpipe, W_ovqp, &s, 2);
		wrpipe(P_NORM, &Outpipe, W_ovqp, rangename(Sourcevar), MAXNAME);
	}

	if (resultnum >= 0)
	{
		s.type = RESULTID;
		s.len = MAXNAME;
		wrpipe(P_NORM, &Outpipe, W_ovqp, &s, 2);
		wrpipe(P_NORM, &Outpipe, W_ovqp, rnum_convert(resultnum), MAXNAME);
	}

	/*	this symbol, USERQRY, should be sent last because OVQP
	 *	requires the query mode and result relation when it
	 *	encounters USERQRY.
	 */

	if (((struct qt_root *) tree)->rootuser)
	{
		pipesym(USERQRY, 0);
	}

	/*  now write the query list itself  */

	pipesym(TREE, 0);

	if (tree->sym.type == AGHEAD)
	{
		pipesym(AGHEAD, 0);
		if (tree->left->sym.type == BYHEAD)
		{
			mklist(tree->left->right);
			pipesym(BYHEAD, 0);
			mklist(tree->left->left);
		}
		else
			mklist(tree->left);
	}
	else
		mklist(tree->left);

	pipesym(ROOT, 0);
	mklist(tree->right);
	pipesym(QLEND, 0);

	/* now flush the query to ovqp */
	wrpipe(P_FLUSH, &Outpipe, W_ovqp);

	/*
	** Read the result of the query from ovqp. A Retcode
	** struct is expected.
	** The possble values of Retcode.rt_status are:
	**	EMPTY     -- no tuples satisfied
	**	NONEMPTY  -- at least one tuple satisfied
	**	error num -- ovqp user error
	*/


	rdpipe(P_PRIME, &Inpipe);
	if (rdpipe(P_NORM, &Inpipe, R_ovqp, &ret, sizeof (ret)) != sizeof (ret))
		syserr("readresult: read error on R_ovqp");
	j = ret.rc_status;
	switch (j)
	{
	  case EMPTY:
		i = FALSE;
		break;

	  case NONEMPTY:
		i = TRUE;
		if (((struct qt_root *) tree)->rootuser)
			Retcode.rc_tupcount = ret.rc_tupcount;
		break;

	  default:
		derror(j);

	}
	return (i);
}



readagg_result(result)
struct querytree	*result[];

/*
** Readagg_result -- read results from ovqp from a simple aggregate.
*/

{
	register struct querytree	**r, *aop;
	register int			vallen;
	int				i;

	r = result;
	while (aop = *r++)
	{
		vallen = aop->sym.len & I1MASK;
		if (rdpipe(P_NORM, &Inpipe, R_ovqp, &aop->sym, 2) != 2)
			syserr("ageval:rd err (1)");
		if (vallen != (aop->sym.len & I1MASK))
			syserr("ageval:len %d,%d", vallen, aop->sym.len);
		if ((i = rdpipe(P_NORM, &Inpipe, R_ovqp, aop->sym.value, vallen)) != vallen)
			syserr("ageval:rd err %d,%d", i, vallen);
#		ifdef xDTR1
		if (tTf(8, 3))
			writenod(aop);
#		endif
	}
}




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
	register int		done, j;
	struct retcode		ret;
	extern int		Qry_mode;
	extern struct descriptor Inddes;

	if (ack != RUBACK)
	{
		wrpipe(P_PRIME, &Outpipe, EXEC_OVQP, 0, 0);
	
		/* send mode of the query */
		pipesym(QMODE, 1, Qry_mode);
	
		/* send exit code */
		pipesym(EXIT, 1, ack);
	
		wrpipe(P_END, &Outpipe, W_ovqp);
	}

	j = NOUPDATE;
	if (ack == ACK)
	{
		/* read acknowledgement from ovqp */
		rdpipe(P_PRIME, &Inpipe);
		if (rdpipe(P_NORM, &Inpipe, R_ovqp, &ret, sizeof (ret)) != sizeof (ret))
			syserr("endovqp no retcode");

		j = ret.rc_status;
		if (j != UPDATE && j != NOUPDATE)
			syserr("endovqp:%d", j);

		rdpipe(P_SYNC, &Inpipe, R_ovqp);
	}

	cleanrel(&Inddes);	/* could be closecatalog(FALSE) except for
				** text space limitations */
	return (j);
}






pipesym(token, length, val)
int	token;
int	length;
int	val;
{
	register struct symbol	*s;
	char			temp[4];
	register int		i;

	s = (struct symbol *) temp;
	i = length;

	s->type = token;
	if (s->len = i)
		s->value[0] = val;

	wrpipe(P_NORM, &Outpipe, W_ovqp, s, i + 2);
}

ovqpnod(node)
struct querytree	*node;
{

	register int			typ;
	register struct querytree	*q;
	register struct symbol		*s;
	int				i;
	char				temp[4];
	struct querytree		*ckvar();

	q = node;
	s = (struct symbol *) temp;

	typ = q->sym.type;
	s->type = typ;

	switch (typ)
	{
	  case VAR:
		q = ckvar(q);

		/*  check to see if this variable has been substituted for.
		**  if so, pass the constant value to OVQP		      */

		if (((struct qt_var *)q)->valptr)
		{
			s->type = ((struct qt_var *)q)->frmt;
			s->len = ((struct qt_var *)q)->frml;
			wrpipe(P_NORM, &Outpipe, W_ovqp, s, 2);
			wrpipe(P_NORM, &Outpipe, W_ovqp, ((struct qt_var *)q)->valptr, s->len & 0377);
			return;
		}
		else
		{
			/* double check that variable is sourcevar */
			if (((struct qt_var *)q)->varno != Sourcevar)
				syserr("mklist:src=%d,var=%d", Sourcevar, ((struct qt_var *)q)->varno);
			s->len = 1;
			s->value[0] = ((struct qt_var *)q)->attno;
			break;
		}

	  case TREE:
		return;

	  case AND:
	  case OR:
		s->len = 0;
		break;

	  case BOP:
	  case RESDOM:
	  case UOP:
		s->len = 1;
		s->value[0] = *(q->sym.value);
		break;

	  default:
		i = (q->sym.len & 0377) + 2;
		wrpipe(P_NORM, &Outpipe, W_ovqp, &q->sym, i);
		return;
	}

	wrpipe(P_NORM, &Outpipe, W_ovqp, s, s->len + 2);
}

/*
**	Use "inpcloser()" for closing relations. See
**	desc_close in openrs.c for details.
*/
extern int	inpcloser();
int		(*Des_closefunc)()	= inpcloser;


init_decomp()
{
	/* this is a null routine called from main() */
}


startdecomp()
{
	/* called at the start of each user query */
	Retcode.rc_tupcount = 0;
	initrange();
	rnum_init();
}


/*
** Files_avail -- returns how many file descriptors are available
**	for decomposition. For decomp running by itself the fixed
**	overhead for files is:
**
**		4 -- pipes
**		1 -- relation relation
**		1 -- attribute relation
**		1 -- indexes relation
**		1 -- standard output
**		1 -- concurrency
*/

files_avail(mode)
int	mode;
{
	return (MAXFILES - 9);
}

# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"ovqp.h"

extern struct pipfrmt		Inpipe, Outpipe;
extern struct descriptor	Reldesc, Srcdesc;
struct descriptor		Indesc;

endqry(code)
int	code;

/*
**	Endqry is called at the end of a query, either
**	in response to getqry receiving an EXIT or as a
**	result of a user error
*/
{
	register int	i;

	i = code;

	if (Equel && Retrieve)
		equeleol(EXIT);	/* signal end of query to equel process */
	if (i == ACK)
	{
		if (Bopen)
			i = UPDATE;
		else
			i = NOUPDATE;

		closeall();
		retdecomp(i);
	}
	else
	{
		if (i == NOACK)
		{
			if (Bopen)
			{
				/* remove the batch file */
				rmbatch();
				Bopen = FALSE;
			}
		}
		else
			syserr("endqry:bad ack %d",code);
		closeall();
	}
}




ov_err(code)
int	code;
/*
**	Usererr is called when a user generated
**	error is detected. The error number is
**	sent to DECOMP for processing. The read pipe
**	is flushed. A non-local goto (reset) returns
**	to main.c (see setexit()).
*/
{
	retdecomp(code);	/* send error message to decomp */
	endqry(NOACK);		/* end the query with no ack to decomp */
				/* and close all relations */
	rdpipe(P_SYNC, &Inpipe, R_decomp);	/* flush the pipe */

	/* copy the error message from decomp to the parser */
	copyreturn();
	reset();	/* return to main.c */
	syserr("ov_err:impossible return from reset");
}



closeall()
{
	closerel(&Srcdesc);
	closerel(&Reldesc);
	closecatalog(FALSE);
	closerel(&Indesc);
	if (Bopen)
	{
		closebatch();
		Bopen = FALSE;
	}
}

retdecomp(code)
int	code;
/*
**	writes a RETVAL symbol with value "code".
**	if code = EMPTY then retdecomp will wait
**	for an EOP acknowledgement from DECOM before
**	returning.
**	If the query was a simple
**	aggregate, then the aggregate values are also written.
*/

{
	struct retcode	ret;
	register	i, t;
	struct stacksym	ressym;
	struct symbol	**alist;
	struct al_sym
	{
		char	type;
		char	len;
		int	aoper;
		char	atype;
		char	alen;
	};

#	ifdef xOTR1
	if (tTf(33, 1))
		printf("RETDECOMP: returning %d\n", code);
#	endif
	ret.rc_status = code;
	ret.rc_tupcount = Tupsfound;
	wrpipe(P_NORM, &Outpipe, W_decomp, &ret, sizeof (ret));

	if (Alist && !Bylist)
	{
		alist = Alist;
		i = Agcount;
		Tend = Outtup;

		while (i--)
		{

			/* find next AOP */
			while ((t = (*++alist)->type) != AOP)
				if (t == ROOT)
					syserr("retdecomp:missing aop %d", i);

			ressym.type = ((struct al_sym *) *alist)->atype;
			t = ressym.len = ((struct al_sym *) *alist)->alen;
			t &= I1MASK;
			if (ressym.type == CHAR)
			{
				cpderef(ressym.value) = Tend;
				/* make sure it is blank padded */
				pad(Tend, t);
			}
			else
				bmove(Tend, ressym.value, t);
			Tend += t;

#			ifdef xOTR1
			if (tTf(33, 2))
			{
				printf("returning aggregate:");
				prstack(&ressym);
			}
#			endif
			pwritesym(&Outpipe, W_decomp, &ressym);	/* write top of stack to decomp */
		}
	}

	wrpipe(P_END, &Outpipe, W_decomp);
}
openrel(desc, relname, mode)
struct descriptor	*desc;
char			*relname;
int			mode;

/*
**	openrel checks to see if the descriptor already
**	holds the relation to be opened. If so it returns.
**	Otherwise it closes the relation and opens the new one
*/

{
	register struct descriptor	*d;
	register char			*name;
	register int			i;

	d = desc;
	name = relname;
#	ifdef xOTR1
	if (tTf(33, 4))
		printf("openrel: open=%d current=%.12s, new=%.12s\n", d->relopn, d->relid, name);
#	endif

	if (d->relopn)
	{
		if (bequal(d->relid, name, MAXNAME))
			return;
#		ifdef xOTR1
		if (tTf(33, 5))
			printf("openrel:closing the old\n");
#		endif
		closerel(d);
	}
	if (i = openr(d, mode, name))
		syserr("can't open %.12s %d", name, i);
}


closerel(desc)
struct descriptor	*desc;

/*
**	close the relation
*/

{
	register struct descriptor	*d;
	register int			i;

	d = desc;
#	ifdef xOTR1
	if (tTf(33, 6))
		printf("closerel open=%d rel=%.12s\n", d->relopn, d->relid);
#	endif
	if (d->relopn)
		if (i = closer(d))
		{
		printf("closerel open=%d rel=%.12s\n", d->relopn, d->relid);
			syserr("can't close %.12s %d", d->relid, i);
		}
}
struct descriptor *openindex(relid)
char		*relid;
{
	openrel(&Indesc, relid, 0);
	return (&Indesc);
}

/*
**	SYSTEM RELATION DESCRIPTOR CACHE DEFINITION
**
*/

extern struct descriptor	Inddes;



struct desxx	Desxx[] =
{
	"indexes",	&Inddes,	0,
	0
};

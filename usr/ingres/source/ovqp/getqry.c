# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"ovqp.h"

/*
**	getqry is called to read a logical query
**	from decomp. The query always comes is a
**	standard sequence:
**
**	Query info
**		ie. source, result, mode etc.
**
**	TREE
**
**	optional target list
**
**	optional AGHEAD
**
**		optional BYLIST for AG
**
**	ROOT
**
**	optional qualification list
**
**	QLEND
**
**	return 0 if query received
**	return ACK or NOACK if query processing done
*/

extern struct pipfrmt	Inpipe, Outpipe;
struct descriptor	Reldesc, Srcdesc, Inddes;


getqry()
{
	register struct symbol	*sym;
	extern int		Batchupd;
	register int		i;
	register int		qb_mark;
	extern			ov_err();
	extern char		*Ovqpbuf;
	char			*rdsym();
	struct symbol		**sym_ad();

	/* initialize query buffer, and prepare for priming it later 
	 * in this routine
	 */

	initbuf(Ovqpbuf, LBUFSIZE, LISTFULL, ov_err);
	qb_mark = markbuf(Ovqpbuf);
	wrpipe(P_PRIME, &Outpipe, EXEC_DECOMP, 0, 0);

	Source = Result = 0;	/* assume no source or result relations */
	Userqry = Buflag = Newq = FALSE;		/* assume batch not needed */
	/* read in initial info about query */

	while ((sym = (struct symbol *) rdsym())->type != TREE)	/* process until TREE */
	{
		switch (sym->type)
		{
		  case SOURCEID:
			Source = &Srcdesc;
			openrel(&Srcdesc, sym->value, 0);
			break;

		  case RESULTID:
			Result = &Reldesc;
			openrel(&Reldesc, sym->value, 1);
			break;

		  case USERQRY:
			Userqry = TRUE;
			/* if this is an update, consider using batch */
			if (Result && (Ov_qmode != mdRETR))
			{
				/* if the batchupd flag is set or the rel has a secondary index */
				/* batch the updates */
#				ifdef xOTR1
				if (tTf(22, 9))
					printf("Batchupd=%d,relindx=%d\n", Batchupd, Result->relindxd);
#				endif
				if (Batchupd || Result->relindxd)
				{
					/* if the batch hasn't been opened yet, open it */
#					ifdef xOTR1
					if (tTf(22, 10))
						printf("USING BATCH\n");
#					endif
					if (!Bopen)
					{
						opencatalog("indexes", 0);
						if (i = openbatch(Result, &Inddes, Ov_qmode))
							syserr("getqry:openbatch ret %d", i);
						Bopen = TRUE;
					}
					Buflag = TRUE;
				}
			}
			if (!Result)	/* true if this is a retrieve to equel or the terminal */
				Retrieve = TRUE;
			break;

		  case CHANGESTRAT:
			/* indicates to strategy that a new strategy must be determined */
			Newq = TRUE;
			break;

		  case REOPENRES:
			/* guarantee that result relation will be opened fresh */
			closerel(&Reldesc);
			break;

		  case QMODE:
			Ov_qmode = *sym->value & 0377;
			/*
			** A mode of RETINTO comes only at the
			** end of a query. It's use is for signaling
			** the Retrieve flag for equel. It is
			** actually only needed when an equel
			** retrieve is made on an empty relation.
			** If you don't understand this don't worry.
			*/
			if (Ov_qmode == mdRETTERM)
			{
				Retrieve = TRUE;
				/* change the mode just in case */
				Ov_qmode = mdRETR;
			}
			break;

		  case EXIT:
			rdpipe(P_SYNC, &Inpipe, R_decomp);	/* read the end of pipe */
			return (*sym->value & 0377);	/* return mode of query end */

		  default:
			syserr("getqry:bad sym %d", sym->type);
		}
	}

	/* init the various list head as null */
	Tlist = Alist = Bylist = Qlist = 0;
	Qvpointer = Targvc = Qualvc = 0;

	/* prime the buffer,
	 * this throws away the header info from above
	 */
	freebuf(Ovqpbuf, qb_mark);

	/* read in (possibly null) target list */
	while ((sym = (struct symbol *) rdsym())->type != AGHEAD)	/* read until an AGHEAD */
	{
		switch (sym->type)
		{
		  case ROOT:
			goto qread;

		  case VAR:
			/* variable in target list */
			Targvc++;
			putvar(sym, &Srcdesc, Intup);
			/* fall through for Tlist check */

		  default:
			/* the catch all INT, CHAR, etc. */
			if (!Tlist)
				Tlist = sym_ad();	/* top of target list */
			break;
		}
	}
	/* next possibility is aggregate */

	Agcount = 0;
	while ((sym = (struct symbol *) rdsym())->type != ROOT)	/* process until ROOT */
	{
		switch (sym->type)
		{
		  case VAR:
			/* variable in aggregate list */
			Targvc++;
			putvar(sym, &Srcdesc, Intup);
			/*  fall through for Alist check */

		  default:
			if (!Alist)
				Alist = sym_ad();
			if (sym->type == AOP)
				Agcount++;
			break;

		  case BYHEAD:
			/* aggregate function */
			while ((sym = (struct symbol *) rdsym())->type != ROOT)	/* read until ROOT */
			{
				if (!Bylist)
					Bylist = sym_ad();
				if (sym->type == VAR)
					putvar(sym, &Srcdesc, Intup);
			}
			goto qread;
		}
	}

qread:

	/* read in the qualification */
	while ((sym = (struct symbol *) rdsym())->type != QLEND)	/* read until end of qualification */
	{
		if (!Qlist)
			Qlist = sym_ad();
		if (sym->type == VAR)
		{
			Qualvc++;
			putvar(sym, &Srcdesc, Intup);
		}
	}
	return (0);	/* successful return */
}

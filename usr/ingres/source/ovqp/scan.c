# include	"../ingres.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"ovqp.h"


/*
**	SCAN
**
**	performs tuple by tuple scan of source reln or index reln
**	within limits found by strategy routine. 
**	When the source reln tuple is obtained the interpreter is invoked
**	to continue further processing
**
*/


scan()
{
	register	j, mode, domno;
	char		*rlist;	/* "result" list of query */
	long		count;
	long		tid, temptid;
	char		agtup[MAXTUP], outtup1[MAXTUP];
	int		qualfound, ok;
	struct symbol	*interpret();

#	ifdef xOTR1
	if (tTf(30, -1))
	{
		printf("SCAN\tScanr=%.12s\n", Scanr ? Scanr->relid : "(none)");
		if (tTf(30, 4))
			printf(" Alist=%l, Bylist=%l, Tlist=%l, Qlist=%l\n", Alist, Bylist, Tlist, Qlist);
	}
#	endif

	if (Result || Alist)
	{
		if (Result)
			clr_tuple(Result, Outtup);
		else
		{
			j = MAXTUP;
			while (j--)
				Outtup[j] = 0;
		}
	}

	count = 0;
	qualfound = EMPTY;
	mode = Ov_qmode;

	/*
	** Check for identical source and result relations.
	** For modes mdREPL and mdDEL, Origtup must point
	** to the original (unmodified result tuple).
	**
	** If there is no Source or Result relations then
	** the code has no effect.
	*/
	if (!bequal(Source->relid, Result->relid, MAXNAME))
	{
		Diffrel = TRUE;
		Origtup = outtup1;
	}
	else
	{
		Diffrel = FALSE;
		Origtup = Intup;
	}

	/*  determine type of result list */
	/* the only valid combinations are:
	**
	** Tlist=no	Alist=no	Bylist=no
	** Tlist=yes	Alist=no	Bylist=no
	** Tlist=no	Alist=yes	Bylist=no
	** Tlist=no	Alist=yes	Bylist=yes
	*/
	rlist = (char *) (Tlist? Tlist: Alist);
	if (Bylist)
		rlist = 0;

	Counter= &count;
	if (Bylist)
	{
		/*
		** For aggregate functions the result relation
		** is in the format:
		** domain 1 = I4 (used as a counter)
		** domain 2 through relatts - Agcount (by-domains)
		** remaining domains (the actual aggregate values)
		*/

		/* set up keys for the getequal */
		/* domno must end with the domain number of the first aggregate */
		for (domno = 2; domno <= Result->relatts - Agcount; domno++)
			Result->relgiven[domno] = 1;

		Counter = (long *) Outtup;	/* first four bytes of Outtup is counter for Bylist */
	}

	/*
	** check for constant qualification.
	** If the constant qual is true then remove
	** the qual to save reprocessing it.
	** If it is false then block further processing.
	*/

	ok = TRUE;
	if (Qlist && Qualvc == 0)
		if (*interpret(Qlist)->value)
			Qlist = 0;	/* qual always true */
		else
			ok = FALSE;	/* qual always false */


	/* if no source relation, interpret target list */
	if (!Scanr && ok)
	{
		/* there is no source relation and the qual is true */
		qualfound = NONEMPTY;
		Tend = Outtup;
		/* if there is a rlist then process it. (There should always be one) */
		if (rlist)
		{
			(*Counter)++;
			interpret(rlist);
		}
		if (Tlist)
			dispose(mode);
		else
			if (Userqry)
				Tupsfound++;
	}

	if (Scanr && ok)
	{
		/* There is a source relation. Iterate through each tuple */
		while (!(j = get(Scanr, &Lotid, &Hitid, Intup, NXTTUP)))
		{
#			ifdef xOTR1
			if (tTf(30, 5))
			{
				if (Scanr != Source)
					printf("Sec Index:");
				else
					printf("Intup:");
				printup(Scanr, Intup);
			}
#			endif
			Intid = Lotid;
			if (Scanr != Source)
			{
				/* make sure index tuple is part of the solution */
				if (!indexcheck())
					/* index keys don't match what we want */
					continue;
				bmove(Intup + Scanr->relwid - TIDLEN, &tid, TIDLEN);
				if (j = get(Source, &tid, &temptid, &Intup, CURTUP))
					syserr("scan:indx get %d %.12s", j, Scanr->relid);
#				ifdef xOTR1
				if (tTf(30, 6))
				{
					printf("Intup:");
					printup(Source, Intup);
				}
#				endif
				Intid = tid;
			}

			if ( !Qlist || *interpret(Qlist)->value)
			{
				qualfound = NONEMPTY;
				Tend = Outtup;
				if (rlist)
				{
					(*Counter)++;
					interpret(rlist);
				}
				if (Tlist)
					dispose(mode);
				else
					if (Userqry)
						Tupsfound++;

				if (!Targvc)	/* constant Target list */
					break;

				/* process Bylist if any */
				if (Bylist)
				{
					interpret(Bylist);
					if ((j = getequal(Result, Outtup, agtup, &Uptid)) < 0)
						syserr("scan:getequal %d,%.12s", j, Result->relid);
	
					if (!j)
					{
						/* match on bylist */
						bmove(agtup, Outtup, Result->relwid);
						mode = mdREPL;
						(*Counter)++;
					}
					else
					{
						/* first of this bylist */
						mode = mdAPP;
						*Counter = 1;
					}
	
					Tend = Outtup + Result->reloff[domno];
					interpret(Alist);
					dispose(mode);
				}
			}
		}

		if (j < 0)
			syserr("scan:get prim %d %.12s", j, Source->relid);
	}
	if (Result)
	{
		if (j = noclose(Result))
			syserr("scan:noclose %d %.12s", j, Result->relid);
	}
	return (qualfound);
}

dispose(mode)
{
	register int	i;

	i = 0;

	if (!Result)
	{
		if (Equel)
			equeleol(EOTUP);
		else
			printeol();
	}
	else
	{
#		ifdef xOTR1
		if (tTf(30, -1))
		{
			if (tTf(30, 1))
				printf("mode=%d,",mode);
			if (tTf(30, 2) && (mode == mdREPL || mode == mdDEL))
				printf("Uptid:%s, ",locv(Uptid));
			if (tTf(30, 3))
				if (mode == mdDEL)
					printup(Source, Intup);
				else
					printup(Result, Outtup);
		}
#		endif

		/* SPOOL UPDATES OF EXISTING USER RELNS TO BATCH PROCESSOR */
		if (Buflag)
		{
			addbatch(&Uptid, Outtup, Origtup);
			return;
		}

		/* PERFORM ALL OTHER OPERATIONS DIRECTLY */
		switch (mode)
		{
		  case mdRETR:
		  case mdAPP:
			if ((i = insert(Result, &Uptid, Outtup, NODUPS)) < 0)
				syserr("dispose:insert %d %.12s", i, Result->relid);
			break;

		  case mdREPL:
			if ((i = replace(Result, &Uptid, Outtup, NODUPS)) < 0)
				syserr("dispose:replace %d %.12s", i, Result->relid);
			break;

		  case mdDEL:
			if ((i = delete(Result, &Uptid)) < 0)
				syserr("dispose:delete %d %.12s", i, Result->relid);
			break;

		  default:
			syserr("dispose:bad mode %d", mode);
		}
	}

	if (Userqry && i == 0)
		Tupsfound++;
}

# include	"../ingres.h"
# include	"../aux.h"
# include	"../catalog.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"ovqp.h"
# include	"strategy.h"

/**
** STRATEGY
**
**	Attempts to limit access scan to less than the entire Source
**	relation by finding a key which can be used for associative
**	access to the Source reln or an index thereon.  The key is
**	constructed from domain-value specifications found in the
**	clauses of the qualification list using sub-routine findsimp
**	in findsimp.c and other subroutines in file key.c
**/



strategy()
{
	register int			i, allexact;
	struct accessparam		sourceparam, indexparam;
	struct index			itup, rtup;
	struct key			lowikey[MAXKEYS+1], highikey[MAXKEYS+1];
	register struct descriptor	*d;
	extern struct descriptor	Inddes;
	struct descriptor		*openindex();

#	ifdef xOTR1
	if (tTf(31, 0))
		printf("STRATEGY\tSource=%.12s\tNewq = %d\n", Source ? Source->relid : "(none)", Newq);
#	endif

	while (Newq)	/* if Newq=TRUE then compute a new strategy */
			/* NOTE: This while loop is executed only once */
	{
		Scanr = Source;
	
		if (!Scanr)
			return (1);	/* return immediately if there is no source relation */
	
		Fmode = NOKEY;	/* assume a find mode with no key */
	
		if (!Qlist)
			break;	/* if no qualification then you must scan entire rel */
	
		/* copy structure of source relation into sourceparam */
		paramd(Source, &sourceparam);
	
		/* if source is unkeyed and has no sec index then give up */
		if (sourceparam.mode == NOKEY && Source->relindxd <= 0)
			break;

		/* find all simple clauses if any */
		if (!findsimps())
			break;	/* break if there are no simple clauses */
	
		/* Four steps are now performed to try and find a key.
		** First if the relation is hashed then an exact key is search for
		**
		** Second if there are secondary indexes, then a search is made
		** for an exact key. If that fails then a  check is made for
		** a range key. The result of the rangekey check is saved.
		**
		** Third if the relation is an ISAM a check is  made for
		** an exact key or a range key.
		**
		** Fourth if there is a secondary index, then if step two
		** found a key, that key is used.
		**
		**  Lastly, give up and scan the  entire relation
		*/
	
		/* step one. Try to find exact key on primary */
		if (exactkey(&sourceparam, &Lkey_struct))
		{
			Fmode = EXACTKEY;
			break;
		}
	
		/* step two. If there is an index, try to find an exactkey on one of them */
		if (Source->relindxd)
		{
	
			opencatalog("indexes", 0);
			setkey(&Inddes, &itup, Source->relid, IRELIDP);
			setkey(&Inddes, &itup, Source->relowner, IOWNERP);
			if (i = find(&Inddes, EXACTKEY, &Lotid, &Hitid, &itup))
				syserr("strategy:find indexes %d", i);
	
			while (!(i = get(&Inddes, &Lotid, &Hitid, &itup, NXTTUP)))
			{
#				ifdef xOTR1
				if (tTf(31, 3))
					printup(&Inddes, &itup);
#				endif
				if (!bequal(itup.irelidp, Source->relid, MAXNAME) ||
				    !bequal(itup.iownerp, Source->relowner, 2))
					continue;
				parami(&itup, &indexparam);
				if (exactkey(&indexparam, &Lkey_struct))
				{
					Fmode = EXACTKEY;
					d = openindex(itup.irelidi);
					/* temp check for 6.0 index */
					if (d->relindxd == -1)
						ov_err(BADSECINDX);
					Scanr = d;
					break;
				}
				if (Fmode == LRANGEKEY)
					continue;	/* a range key on a s.i. has already been found */
				if (allexact = rangekey(&indexparam, &lowikey, &highikey))
				{
					bmove(&itup, &rtup, sizeof itup);	/* save tuple */
					Fmode = LRANGEKEY;
				}
			}
			if (i < 0)
				syserr("stragery:bad get from index-rel %d", i);
			/* If an exactkey on a secondary index was found, look no more. */
			if (Fmode == EXACTKEY)
				break;
		}
	

		/* step three. Look for a range key on primary */
		if (i = rangekey(&sourceparam, &Lkey_struct, &Hkey_struct))
		{
			if (i < 0)
				Fmode = EXACTKEY;
			else
				Fmode = LRANGEKEY;
			break;
		}
	
		/* last step. If a secondary index range key was found, use it */
		if (Fmode == LRANGEKEY)
		{
			if (allexact < 0)
				Fmode = EXACTKEY;
			d = openindex(rtup.irelidi);
			/* temp check for 6.0 index */
			if (d->relindxd == -1)
				ov_err(BADSECINDX);
			Scanr = d;
			bmove(&lowikey, &Lkey_struct, sizeof lowikey);
			bmove(&highikey, &Hkey_struct, sizeof highikey);
			break;
		}

		/* nothing will work. give up! */
		break;
	
	}

	/* check for Newq = FALSE and no source relation */
	if (!Scanr)
		return (1);
	/*
	** At this point the strategy is determined.
	**
	** If Fmode is EXACTKEY then Lkey_struct contains
	** the pointers to the keys.
	**
	** If Fmode is LRANGEKEY then Lkey_struct contains
	** the pointers to the low keys and Hkey_struct
	** contains pointers to the high keys.
	**
	** If Fmode is NOKEY, then a full scan will be performed
	*/
#	ifdef xOTR1
	if (tTf(31, -1))
		printf("Fmode= %d\n",Fmode);
#	endif

	/* set up the key tuples */
	if (Fmode != NOKEY)
	{
		if (setallkey(&Lkey_struct, Keyl))
			return (0);	/* query false. There is a simple
					** clause which can never be satisfied.
					** These simple clauses can be choosey!
					*/
	}

	if (i = find(Scanr, Fmode, &Lotid, &Hitid, Keyl))
		syserr("strategy:find1 %.12s, %d", Scanr->relid, i);

	if (Fmode == LRANGEKEY)
	{
		setallkey(&Hkey_struct, Keyh);
		if (i = find(Scanr, HRANGEKEY, &Lotid, &Hitid, Keyh))
			syserr("strategy:find2 %.12s, %d", Scanr->relid, i);
	}

#	ifdef xOTR1
	if (tTf(31, 1))
	{
		printf("Lo");
		dumptid(&Lotid);
		printf("Hi");
		dumptid(&Hitid);
	}
#	endif

	return (1);
}

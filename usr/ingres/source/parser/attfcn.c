# include	"../ingres.h"
# include	"../aux.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"../symbol.h"
# include	"parser.h"

/*
** fake attribute stash entries for tid nodes (and sid in dist version)
**	these are provided by the system to a program only for debugging
**	the system source and do not have well defined (over time) meanings.
*/
struct atstash	Faketid =
{
	0,	INT,	4,	"tid",	0
};
#ifdef	DISTRIB
struct atstash	Fakesid =
{
	0,	INT,	4,	"sid",	0
};
#endif

/*
**  ATTLOOKUP
**	routine to look up atribute 'attrib' in the att stash
**	for range table entry 'rptr'.  If the attrib is not
**	in the stash it is entered.
*/
struct atstash *
attlookup(rptr1, attrib)
struct rngtab	*rptr1;
char		*attrib;
{
	register struct rngtab		*rptr;
	register struct atstash		*current, *sp;
	int				ik;
	struct attribute		tuple;
	register struct attribute	*ktuple;
	struct attribute		ktup;
	struct tup_id			tid;
	extern struct atstash		*attfind();
	extern struct atstash		*attadd();

	rptr = rptr1;
	ktuple = &ktup;
#	ifdef	xPTR2
	tTfp(10, 1, "attlookup: att = %s and rel= %s\n", attrib, rptr->relnm);
#	endif

	/* attribute called "tid" is phantom attribute, use fake */
	if (sequal("tid", attrib))
		return (&Faketid);
#	ifdef	DISTRIB
	if (sequal("sid", attrib))
		return (&Fakesid);
#	endif

	/* check to see if attrib is in stash */
	if ((sp = attfind(rptr, attrib)) != 0)
		return (sp);

#	ifdef	xPTR2
	tTfp(10, 2, "getting att info from relation\n");
#	endif

	/* rel name, owner, attname is unique ident */
	clearkeys(&Desc);
	setkey(&Desc, ktuple, rptr->relnm, ATTRELID);
	setkey(&Desc, ktuple, rptr->relnowner, ATTOWNER);
	setkey(&Desc, ktuple, attrib, ATTNAME);
	if (!(ik=getequal(&Desc, ktuple, &tuple, &tid)))
	{
		/* put attrib stuff into att stash */
		current = attadd(rptr, &tuple);
		return (current);
	}
	if (ik == 1)
		/* attribute not in relation */
		yyerror(NOATTRIN, attrib, rptr->relnm, 0);
	syserr("fatal error in getequal, ret: %d", ik);
}

/*
** ATTADD
**	add an attrib to the list
*/
struct atstash *
attadd(rptr, tuple)
struct rngtab		*rptr;
struct attribute	*tuple;
{
	register struct atstash	*current;
	register struct atstash	*aptr;
	register struct atstash	*bptr;
	int			i;
	extern struct atstash	*attalloc();

	current = attalloc();
	current->atbid = tuple->attid;
	current->atbfrmt = tuple->attfrmt;
	current->atbfrml = tuple->attfrml;
	bmove(tuple->attname, current->atbname, MAXNAME);
	for (i = 0; i < MAXNAME; i++)
		if (current->atbname[i] == ' ')
			current->atbname[i] = '\0';

	aptr = rptr->attlist;
	bptr = 0;
	while (aptr != 0)
	{
		if (aptr->atbid > current->atbid)
			break;
		bptr = aptr;
		aptr = aptr->atbnext;
	}
	if (bptr == 0)
		rptr->attlist = current;
	else
		bptr->atbnext = current;
	current->atbnext = aptr;
	return (current);
}

/*
** ATTFIND
**	look in attribute stash to see if attrib info already there
**	return pointer to attribute in attribute table else zero
*/
struct atstash *
attfind(rptr, attrib1)
struct rngtab	*rptr;
char		*attrib1;
{
	register struct atstash	*aptr;
	register char		*attrib;

	attrib = attrib1;
	aptr = rptr->attlist;
	while (aptr != 0)
	{
		if (!scompare(attrib, MAXNAME, aptr->atbname, MAXNAME))
			return (aptr);
		aptr = aptr->atbnext;
	}
	return (0);
}

/*
** ATTCHECK
**	checks for type conflicts in the current query domain and
**	attable entry
*/
attcheck(aptr1)
struct atstash	*aptr1;
{
	register struct atstash	*aptr;

	aptr = aptr1;
	if ((Trfrmt == CHAR) != (aptr->atbfrmt == CHAR))
		/* function type does not match attrib */
		yyerror(RESTYPE, aptr->atbname, 0);
}

/*
** ATTFREE
**	puts a list of attrib space back on the free list
*/
attfree(aptr)
struct atstash	*aptr;
{
	register struct atstash	*att;

	if ((att = aptr) == NULL)
		return;
	while (att->atbnext != NULL)
		att = att->atbnext;
	att->atbnext = Freeatt;
	Freeatt = aptr;
}

/*
** ATTALLOC
**	returns a pointer to a atstash type structure
*/
struct atstash *
attalloc()
{
	register struct atstash	*aptr;
	register struct rngtab	*rptr;
	register int		i;
	extern struct rngtab	*rngatndx();

	i = MAXVAR - 1;
	while (Freeatt == NULL)
	{
		/*
		** search least recently used vbles for attrib stash space
		** until at least one entry is found
		*/
		rptr = rngatndx(i);
		Freeatt = rptr->attlist;
		rptr->attlist = NULL;
		i = rptr->rposit - 1;	/* try one newer than the current */
	}
	aptr = Freeatt;
	Freeatt = Freeatt->atbnext;
	aptr->atbnext = NULL;
}

/*
** ATTCOUNT
**	returns a count fof the number of attributes already in the
**	attrib stash
*/
attcount(rptr)
struct rngtab	*rptr;
{
	register int		cntr;
	register struct atstash	*aptr;

	cntr = 0;
	aptr = rptr->attlist;
	while (aptr != NULL)
	{
		cntr++;
		aptr = aptr->atbnext;
	}
	return (cntr);
}

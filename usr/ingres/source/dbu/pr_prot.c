#

/*
**  PR_PROT.C -- Print out a protection query
**
**	Defines:
**		pr_prot() -- given a relation owner pair, prints protection
**			queries issued on the relation
**
**	Required By:
**		disp() -- [display.c]
**
**	Conditional Compilation:
**		xZTR1 -- for trace flags
**
**	Trace Flags:
**		11
**
**	History:
**		11/15/78 -- (marc) written
*/




# include	"../ingres.h"
# include	"../tree.h"
# include	"../aux.h"
# include	"../unix.h"
# include	"../catalog.h"
# include	"../symbol.h"


# define 	QTREE		struct querytree


QTREE	*gettree();	/* gets a tree from "tree" catalog [readtree.c] */

char	*Days [] =
{
	"sunday",
	"monday",
	"tuesday",
	"wednesday",
	"thursday",
	"friday",
	"saturday",
};

# define	QTREE		struct querytree

struct rngtab
{
	char	relid [MAXNAME];
	char	rowner [2];
	char	rused;
}; 

struct rngtab	Rangev[MAXVAR + 1];



/*
**  PR_PROT -- print out protection info on a relation
**
**	Prints out a "define permit" statement for 
**	each permission on a relation.
**	First calls pr_spec_permit() to print permissions
**	indicated in the relation.relstat bits. Lower level pr_??
**	routines look for these bits, so in the calls to pr_permit
**	for tuples actually gotten from the "protect" catalog,
**	pr_prot sets the relstat bits, thereby suppressing their special
**	meaning (they are inverse bits: 0 means on).
**
**	Parameters:
**		relid -- non-unique relation name used by user in DBU call
**		rel -- relation tuple
**
**	Returns:
**		0 -- some permissions on rel
**		1 -- no permissions on rel
**
**	Side Effects:
**		reads trees from protection catalog
**
**	Requires:
**		pr_permit() -- to generate query from protection tuple
**		pr_spec_permit() -- to print out relstat generated permissions
**
**	Called By:
**		disp() [display.c]
**
**	Trace Flags:
**		11, 0
**
**	Syserrs:
**		on bad returns from find(), get(), and openr()
**
**	History:
**		11/15/78 -- (marc) written
**		12/19/78 -- (marc) modified to take into account S_PROT[12]
**		1/9/79 -- (marc)    "		"	    "	S_PROT[ALL,RET]
*/

pr_prot(relid, rel)
char			*relid;
struct relation		*rel;
{
	extern struct descriptor	Prodes;
	struct tup_id			hitid, lotid;
	struct protect			key, tuple;
	register			i;
	int				flag;	/* indicates wether a special
						 * case occurred
						 */
	register struct relation	*r;

#	ifdef xZTR1
	if (tTf(11, 0))
		printf("pr_prot: relation \"%s\" owner \"%s\"relstat 0%o\n",
		rel->relid, rel->relowner, rel->relstat);
#	endif

	r = rel;
	flag = 0;
	if (r->relstat & S_PROTUPS || !(r->relstat & S_PROTALL)
	   || !(r->relstat & S_PROTRET))
		printf("Permissions on %s are:\n\n", relid);
	/* print out special permissions, if any */
	flag += pr_spec_permit(r, S_PROTALL);
	flag += pr_spec_permit(r, S_PROTRET);

	if (!(r->relstat & S_PROTUPS))
		if (flag)
			return (0);
		else
			return (1);
	opencatalog("protect", 0);
	
	/* get protect catalog tuples for "r", "owner" */
	clearkeys(&Prodes);
	setkey(&Prodes, &key, r->relid, PRORELID);
	setkey(&Prodes, &key, r->relowner, PRORELOWN);
	if (i = find(&Prodes, EXACTKEY, &lotid, &hitid, &key))
		syserr("pr_prot: find %d", i);
	/* ready for pr_user call to getuser() */
	getuser(-1);
	for ( ; ; )
	{
		if (i = get(&Prodes, &lotid, &hitid, &tuple, TRUE))
			break;
		/* print out protection info */
		if (kcompare(&Prodes, &tuple, &key) == 0)
			/* permission from real protect tuple, concoct
			 * neutral relstat
			 */
			pr_permit(&tuple, r->relstat | S_PROTALL | S_PROTRET);
	}
	if (i != 1)
		syserr("pr_prot: get %d", i);

	/* close user file opened by pr_user call to getuser */
	getuser(0);
}

/*
**  PR_SPEC_PERMIT -- Print out special permissions
**	Prints out permissios indicated by the relation.relstat field bits.
**	Concocts a protection tuple for the permission and assigns a 
**	propermid-like number to it for printing. Passes to pr_permit()
**	the concocted tuple, together with a relstat where the appropriate
**	bit is 0, so that the special printing at the lower level pr_??? 
**	routines takes place.
**
**	Parameters:
**		rel -- relation relation tuple for the permitted relation
**		relst_bit -- if this bit is 0 inthe relstat, prints the query
**				{S_PROTALL, S_PROTRET}
**
**	Returns:
**		1 -- if prints
**		0 -- otherwise
**
**	Requires:
**		pr_permit()
**
**	Called By:
**		pr_prot()
**
**	Syserrs:
**		relst_bit ! in {S_PROTALL, S_PROTRET}
**
**	History:
**		1/9/79 -- (marc) written to handle S_PROTALL and S_PROTRET
*/


pr_spec_permit(rel, relst_bit)
struct relation		*rel;
int			relst_bit;
{
	register struct relation	*r;
	register struct protect		*p;
	struct protect			prot;

	r = rel;
	if (r->relstat & relst_bit)
		return (0);
	p = &prot;
	p->protree = -1;
	if (relst_bit == S_PROTALL)
		p->propermid = 0;
	else if (relst_bit == S_PROTRET)
		p->propermid = 1;
	else
		syserr("pr_spec_permit(relst_bit == 0%o)", relst_bit);

	bmove(r->relid, p->prorelid, MAXNAME);
	bmove("  ", p->prouser, 2);
	bmove(" ", &p->proterm, 1);
	pr_permit(p, (r->relstat | S_PROTRET | S_PROTALL) & ~relst_bit);
	return (1);
}


/*
**  PR_PERMIT -- print out a DEFINE PERMIT query for a protection tuple
**
**	Parameters:
**		prot -- protection tuple
**		relstat -- relstat from relation
**
**	Returns:
**		none
**
**	Side Effects:
**		reads in a tree from the "tree" catalog
**		prints out a query
**
**	Requires:
**		clrrange(), pr_range() -- to initialize and print range table
**		gettree() -- to read in "tree" tree and fill range table
**		pr_ops(),
**		pr_doms(),
**		pr_user(),
**		pr_term(),
**		pr_time(),
**		pr_day() -- to print out parts of query
**		pr_qual() -- [pr_tree.c] to print out qualification
**
**	Called By:
**		pr_prot()
**		pr_spec_permit()
**
**	Trace Flags:
**		11, 1
**
**	History:
**		11/15/78 -- (marc) written
**		12/19/78 -- (marc) changed to deal with relstat
**		1/9/79 -- (marc)    "		"	  "
*/


pr_permit(prot, relstat)
struct protect	*prot;
int		relstat;
{
	register struct protect		*p;
	register QTREE			*t;
	extern int			Resultvar;
	extern struct descriptor	Prodes;

	p = prot;
	/* 
	 * if there is a qualification then
	 * clear range table, then read in protect tree, 
	 * the print out range statements
	 * else create single entry range table.
	 */
	clrrange();
	if (p->protree >= 0)
		t = gettree(p->prorelid, p->prorelown, mdPROT, p->protree);
	else
	{
		t = 0;
		declare(0, p->prorelid, p->prorelown);
		p->proresvar = 0;
	}
	printf("Permission %d -\n\n", p->propermid);
	pr_range();

#	ifdef xZTR1
	if (tTf(11, 1))
	{
		printf("pr_permit: prot=");
		printup(&Prodes, p);
		printf(", Resultvar=%d\n", Resultvar);
	}
#	endif

	/* print out query */
	printf("define permit ");
	pr_ops(p->proopset, relstat);
	printf("on ");
	pr_rv(Resultvar = p->proresvar);
	putchar(' ');
	pr_doms(p->prodomset, relstat);
	printf("\n\t");
	pr_user(p->prouser);
	pr_term(p->proterm);
	if ((relstat & S_PROTRET) && (relstat & S_PROTALL))
	{
		/* not special case */
		pr_time(p->protodbgn, p->protodend);
		pr_day(p->prodowbgn, p->prodowend);
	}
	if (t && t->right != QLEND)
	{
		printf("\nwhere ");
		pr_qual(t->right);
	}
	printf("\n\n\n");
}

/*
**  PR_OPS -- Prints the the operation list defined by a protection opset
**
**	Eliminates the appropriate bits from a copy of the opset while printing
**	out the appropriate operation list.
**
**	Parameters:
**		opset -- protection.opset for the relation
**		relstat
**
**	Returns:
**		none
**
**	Side Effects:
**		printing of permitted op list
**
**	Called By:
**		pr_permit()
**
**	Trace Flags:
**		11, 2
**
**
**	History:
**		11/15/78 -- (marc) written
**		12/19/78 -- (marc) relstat added
**		1/9/79 -- (marc) S_PROTALL and S_PROTRET handled
*/

pr_ops(opset, relstat)
int		opset;
int		relstat;
{
	register	op, j;

#	ifdef xZTR1
	if (tTf(11, 2))
		printf("pr_ops(0%o)\n", opset);
#	endif

	if (!(relstat & S_PROTALL) || opset == -1)
	{
		printf("all ");
		return;
	}
	if (!(relstat & S_PROTRET))
	{
		printf("retrieve ");
		return;
	}

	op = (opset & ~PRO_AGGR & ~PRO_TEST) & 077;
	for ( ; ; )
	{
		if (op & (j = PRO_RETR))
			printf("retrieve");
		else if (op & (j = PRO_REPL))
			printf("replace");
		else if (op & (j = PRO_DEL))
			printf("delete");
		else if (op & (j = PRO_APP))
			printf("append");
		op ^= j;
		if (op)
			printf(", ");
		else
			break;
	}
	putchar(' ');
}

/*
**  PR_DOMS -- Print domains in permit target list
**
**	Parameters:
**		doms -- an 8 byte integer array; a bit map of the domains
**			if all 8 integers are -1, then "all" is printed fo
**			for the target list
**		relstat
**
**	Returns:
**		none
**
**	Side Effects:
**		prints out target list
**
**	Requires:
**		pr_attname() -- to print out names given relation 
**				and domain number
**
**	Called By:
**		pr_permit()
**
**	Trace Flags:
**		11, 3
**
**	History:
**		11/15/78 -- (marc) written
**		1/9/79 -- (marc) modified so will assume all domains if
**			special permission
**		5/22/79 -- (marc) changed so bit of first word is unused
*/

pr_doms(doms, relstat)
int	doms [8];
int	relstat;
{
	register int		*d;
	register int		flag, shift;
	int			word;
	char			*rel;
	extern int		Resultvar;

	word = shift = 0;
	d = doms;
	rel = Rangev [Resultvar].relid;

#	ifdef xZTR1
	if (tTf(11, 3))
	{
		printf("pr_doms: rel=\"%s\" ", rel);
		for (word = 0; word < 8; )
			printf("0%o ", d [word++]);
		word = 0;
		putchar('\n');
	}
#	endif
	if (!(relstat & S_PROTALL) || !(relstat & S_PROTRET))
		return;
	flag = 1;
	for (word = 0; word < 8; word++)
		if (*d++ != -1)
		{
			flag = 0;
			break;
		}

	if (!flag)
	{
		putchar('(');
		for (d = doms, word = 0; word < 8; word++, d++)
		{
			for (shift = 0; shift < 16; shift++, *d >>= 1)
			{
				if (*d & 01)
				{
					if (flag++)
						printf(", ");
					pr_attname(rel, word * 16 + shift);
				}
			}
		}
		putchar(')');
	}
}

/*
**  PR_USER -- prints out permitted user's name
**
**	Parameters:
**		user -- 2 char array, user's usercode as in
**			users file
**
**	Returns:
**		none
**
**	Side Effects:
**		prints users name or "all" if user was "  "
**
**	Requires:
**		getuser(UTIL) -- to get user login name.
**
**	Called By:
**		pr_permit()
**
**	Trace Flags:
**		11, 4
**
**	History:
**		11/15/78 -- (marc) written
*/

pr_user(user)
char		user [2];
{
	register		i;
	char			buf [512];
	register char		*c, *u;
	
#	ifdef xZTR1
	if (tTf(11, 4))
		printf("pr_user(\"%c%c\")\n", user [0], user [1]);
#	endif

	c = buf;
	u = user;
	printf("to ");
	if (bequal(u, "  ", 2))
		printf("all ");
	else
	{
		if (getuser(u, c))
		{
			printf("%c%c ", u[0], u[1]);
			return;
		}
		while (*c != ':' && *c != '\n')
			putchar(*c++);
		putchar(' ');
	}
}

/*
**  PR_TIME -- Prints out clock time range access is allowed
**
**	Parameters:
**		bgn, end -- begin end times in seconds (if all day, returns)
**
**	Returns:
**		none
**
**	Side Effects:
**		prints out time
**
**	Called By:
**		pr_permit()
**
**	Trace Flags:
**		11, 5
**
**	History:
**		11/15/78 -- (marc) written
*/

pr_time(bgn, end)
int		bgn, end;
{
	char		time [3];
	register char	*t;
	register int	b, e;

	t = time;
	b = bgn;
	e = end;
#	ifdef xZTR1
	if (tTf(11, 5))
		printf("pr_time(bgn=%d, end=%d)\n", b, e);
#	endif
	if (b == 0 && e == 24 * 60)
		return;
	printf("from %d:", b / 60);
	itoa(b % 60, t);
	if (!t [1])
		putchar('0');
	printf("%s to %d:", t, e / 60);
	itoa(e % 60, t);
	if (!t [1])
		putchar('0');
	printf("%s ", t);
}

/*
**  PR_DAY -- Prints day range permitted
**
**	Parameters:
**		bgn, end -- bgn end days [0..6] (if all week returns)
**
**	Returns:
**		none
**
**	Side Effects:
**		prints days or nothing
**
**	Requires:
**		Days [0..6] -- string array of day names
**
**	Called By:
**		pr_permit()
**
**	Trace Flags:
**		11, 6
**
**	History:
**		11/15/78 -- (marc) written
*/

pr_day(bgn, end)
int		bgn, end;
{
#	ifdef xZTR1
	if (tTf(11, 6))
		printf("pr_day(bgn=%d, end=%d)\n", bgn, end);
#	endif
	if (bgn == 0 && end >= 6)
		return;
	printf("on %s to %s ", Days [bgn], Days [end]);
}

/*
**  PR_TERM -- Print terminal from which access permitted
**
**	Parameters:
**		term -- 1 char terminal id as in /etc/tty* (if ' ' the returns)
**
**	Returns:
**		none
**
**	Side Effects:
**		prints terminal or nothing
**
**	Called By:
**		pr_permit()
**
**	Trace Flags:
**		11, 7
**
**	History:
**		11/15/78 -- (marc) written
*/

pr_term(term)
# ifdef	xV7_UNIX
char		*term;
# endif
# ifndef xV7_UNIX
char		term;
# endif
{
#	ifdef xZTR1
	if (tTf(11, 7))
# ifdef xV7_UNIX
		printf("pr_term(term='%2s')\n", term);
# endif
# ifndef xV7_UNIX
		printf("pr_term(term='%c')\n", term);
# endif
#	endif

# ifndef xV7_UNIX
	if (term != ' ')
		printf("at tty%c ", term);
# endif
# ifdef xV7_UNIX
	if (*term != ' ')
		printf("at tty%2s ", term);
# endif
}

# include	"../ingres.h"
# include	"../aux.h"
# include	"../pipes.h"
# include	"../tree.h"
# include	"parser.h"
# include	"../symbol.h"


/*
** TREE
**  	FUNCTION TO ADD NODE TO QUERY TREE
**	RETURN VALUE IS POINTER TO NODE JUST CREATED		
*/
struct querytree *
tree(lptr, rptr, typ, len, valu1, attnum1)
struct querytree	*lptr;
struct querytree	*rptr;
char			typ;
int			len;
int			valu1;
struct atstash		*attnum1;
{
	register struct atstash		*attnum;
	register struct querytree	*tptr;
	register int			valu;
	extern char			*need();
	extern struct querytree		*norm();
#	ifdef xPTM
	if (tTf(76, 2))
		timtrace(5);
#	endif

	attnum = attnum1;
	valu = valu1;
	tptr = (struct querytree *) need(Qbuf, len + 6);
	tptr->left = lptr;
	tptr->right = rptr;
	tptr->sym.type = typ;
	tptr->sym.len = len;
	switch (typ)
	{
	  case VAR:
		((struct qt_var *)tptr)->varno = valu & I1MASK;
		((struct qt_var *)tptr)->attno = attnum->atbid;
		((struct qt_var *)tptr)->frmt = attnum->atbfrmt;
		((struct qt_var *)tptr)->frml = attnum->atbfrml;
		((struct qt_var *)tptr)->valptr = 0;
		break;

	  case ROOT:
	  case AGHEAD:
	  case BYHEAD:
	  case AND:
	  case OR:
	  case QLEND:
		break;

	  case UOP:
	  case BOP:
		((struct qt_op *)tptr)->opno = valu;
		format(tptr);
		break;

	  case COP:
		if ((((struct qt_op *)tptr)->opno = getcop(valu)) == BADCOP)
			/* bad const operator */
			yyerror(BADCONSTOP, valu, 0);
		break;

	  case AOP:
		format(tptr->right);
		((struct qt_ag *)tptr)->agfrmt = Trfrmt;
		((struct qt_ag *)tptr)->agfrml = Trfrml;

	  case RESDOM:
		((struct qt_res *)tptr)->resno = valu;
		format(tptr);
		((struct qt_var *)tptr)->frmt = Trfrmt;
		((struct qt_var *)tptr)->frml = Trfrml;
		break;

	  default:
		/* INT, FLOAT, CHAR */
		bmove(valu, tptr->sym.value, len & I1MASK);
		break;
	}
#	ifdef xPTM
	if (tTf(76, 2))
		timtrace(6);
#	endif
	return (tptr);
}

/*
** TREEPR
**  	TREE PRINT ROUTINE
**	CREATES STRING WHERE EACH TARGET LIST ELEMENT AND QUALIFICATION
**	CLAUSE IS IN POLISH POSTFIX FORM BUT OVERALL LIST IS IN INFIX
**	FORM
*/
treepr(p1)
struct querytree	*p1;
{
	register struct querytree	*p;
	register int			l;
	extern struct pipfrmt		Pipebuf;

	p = p1;
	if (p->sym.type == ROOT || p->sym.type == BYHEAD)
	{
		writesym(TREE, 0, 0);
#		ifdef	xPTR2
		tTfp(26, 8, "TREE node\n");
#		endif
	}
	if (p->left)
		treepr(p->left);
	if (p->right)
		treepr(p->right);

	if (p->sym.type <= CHAR)
	{
#		ifdef	xPTR2
		if (tTf(26, 9))
			nodewr(p);
#		endif
		l = p->sym.len & I1MASK;
		wrpipe(P_NORM, &Pipebuf, W_down, &(p->sym), l+2);
		return;
	}
	syserr("Unidentified token in treeprint");
}

# ifdef	xPTR1
/*
** NODEWR
**	printf a tree node for debugging purposes
*/
nodewr(p1)
struct querytree	*p1;
{
	register struct querytree	*p;
	char				str[257];
	register char			*ptr;
	char				*bmove();

	p = p1;
	printf("addr=%l, l=%l, r=%l, typ=%d, len=%d:\n", p, p->left, p->right, p->sym.type, p->sym.len);
	switch (p->sym.type)
	{
	  case VAR:
		printf("\t\tvarno=%d, attno=%d, frmt=%d, frml=%d\n",
			((struct qt_var *)p)->varno,
			((struct qt_var *)p)->attno,
			((struct qt_var *)p)->frmt,
			((struct qt_var *)p)->frml);
		break;

	  case AOP:
		printf("\t\taop=%d, frmt=%d, frml=%d\n",
			((struct qt_res *)p)->resno,
			((struct qt_var *)p)->frmt,
			((struct qt_var *)p)->frml);
		break;

	  case RESDOM:
		printf("\t\trsdmno=%d, frmt=%d, frml=%d\n",
			((struct qt_res *)p)->resno,
			((struct qt_var *)p)->frmt,
			((struct qt_var *)p)->frml);
		break;

	  case CHAR:
		ptr = bmove(p->sym.value, str, p->sym.len & I1MASK);
		*ptr = 0;
		printf("\t\tstring=%s\n", str);
		break;

	  case INT:
		switch(p->sym.len)
		{
		  case 2:
			printf("\t\tvalue=%d\n", i2deref(p->sym.value));
			break;


		  case 4:
			printf("\t\tvalue=%s\n", locv(i4deref(p->sym.value)));
			break;
		}
		break;

	  case FLOAT:
		switch(p->sym.len)
		{
		  case 4:
			printf("\t\tvalue=%f\n", f4deref(p->sym.value));
			break;

		  case 8:
			printf("\t\tvalue=%f\n", f8deref(p->sym.value));
			break;
		}
		break;

	  case UOP:
	  case BOP:
	  case COP:
		printf("\t\top=%d\n", ((struct qt_op *)p)->opno);
		break;
	}
}
# endif

/*
** WINDUP
**	assign resno's to resdoms of an agg fcn
*/
windup(ptr)
struct querytree	*ptr;
{
	register int			tot;
	register int			kk;
	register struct querytree	*t;

	/* COUNT THE RESDOM'S OF THIS TARGET LIST */
	kk = 1;
	for (t = ptr; t; t = t->left)
		kk++;
	tot = 1;
	for (t=ptr; t;t = t->left)
		((struct qt_res *)t)->resno = kk - tot++;
}

/*
** ADDRESDOM - makes a new entry for the target list
**
**	Trname must contain the name of the resdom to
**	use for the header, create and Rsdmno for append, replace
**
**	the parameters are pointers to the subtrees to be
**	suspended from the node
*/
struct querytree *
addresdom(lptr, rptr)
struct querytree	*lptr, *rptr;
{
	extern struct querytree		*tree();
	register struct querytree	*rtval;
	register struct atstash		*aptr;
	char				buf[10];	/* buffer type and length in ascii for dbu */
	struct atstash			*attlookup();

	switch (Opflag)
	{
	  case mdRETR:
	  case mdRET_UNI:
	  case mdVIEW:
		Rsdmno++;
		if (Rsdmno >= MAXDOM)
			/* too many resdoms */
			yyerror(RESXTRA, 0);
		rtval = tree(lptr, rptr, RESDOM, 4, Rsdmno);
		if (!Equel || Resrng)
		{
			/* buffer info for header or CREATE */
			setp(Trname);
			buf[0] = Trfrmt & I1MASK;
			smove(iocv(Trfrml & I1MASK), &buf[1]);
			setp(buf);
		}
		break;

	  default:
		/*
		** for append and replace, the result domain
		** number is determined by the location of
		** the attribute in the result relation
		*/
		if (sequal(Trname, "tid"))
			/* attrib not found */
			yyerror(NOATTRIN, Trname, Resrng->relnm, 0);
#		ifdef	DISTRIB
		if (sequal(Trname, "sid"))
			/* attrib not found */
			yyerror(NOATTRIN, Trname, Resrng->relnm, 0);
#		endif
		aptr = attlookup(Resrng, Trname);
		Rsdmno = aptr->atbid;
		rtval = tree(lptr, rptr, RESDOM, 4, Rsdmno);
		if (Opflag != mdPROT)	/* INTEGRITY not possible here */
			attcheck(aptr);
		break;
	}
	return (rtval);
}

/*
** GETCOP
**	routine to lookup 'string' in constant operators table
**	constant table is declared in tables.y
**	structure is defined in ../parser.h
*/
getcop(string)
char	*string;
{
	register struct constop	*cpt;
	register char		*sptr;
	extern struct constop	Coptab[];

	sptr = string;
	for (cpt = Coptab; cpt->copname; cpt++)
		if (sequal(sptr, cpt->copname))
			return (cpt->copnum);
	return (BADCOP);
}

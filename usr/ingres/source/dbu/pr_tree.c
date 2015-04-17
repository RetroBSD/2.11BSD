#

/*
**  PR_TREE.C -- Query tree printing routines
**
**	Defines:
**		pr_tree() -- print out query tree
**		pr_qual() -- print out qualofocation tree
**		pr_attname() -- print out attribute name of domain
**
**	Required By:
**		display DBU --[display.c]
**
**	Trace Flags:
**		10
**
**	History:
**		11/15/78 -- (marc) written
*/



# include	"../symbol.h"
# include	"../ingres.h"
# include	"../tree.h"
# include	"../aux.h"

char	*pr_trim();
char	*resultres();

struct tab
{
	char	t_opcode;
	char	*t_string;
};


struct tab	Uop_tab[] =
{
	opPLUS,		"+ ",
	opMINUS,	"- ",
	opNOT,		"not[ERROR]",
	opATAN,		"atan",
	opCOS,		"cos",
	opGAMMA,	"gamma",
	opLOG,		"log",
	opASCII,	"ascii",
	opSIN,		"sin",
	opSQRT,		"sqrt",
	opABS,		"abs",
	opEXP,		"exp",
	opINT1,		"int1",
	opINT2,		"int2",
	opINT4,		"int4",
	opFLOAT4,	"float4",
	opFLOAT8,	"float8",
};
struct tab	Bop_tab[] =
{
	opADD,		"+",
	opSUB,		"-",
	opMUL,		"*",
	opDIV,		"/",
	opPOW,		"**",
	opEQ,		"=",
	opNE,		"!=",
	opLT,		"<",
	opLE,		"<=",
	opGT,		">",
	opGE,		">=",
	opMOD,		"%",
};

struct tab	Cop_tab[] =
{
	opDBA,		"dba",
	opUSERCODE,	"usercode",
	opDATE,		"date",
	opTIME,		"time",
};

struct tab	Aop_tab[] =
{
	opCOUNT,	"count",
	opCOUNTU,	"countu",
	opSUM,		"sum",
	opSUMU,		"sumu",
	opAVG,		"avg",
	opAVGU,		"avgu",
	opMIN,		"min",
	opMAX,		"max",
	opANY,		"any",
};

/*
 * This structure must be IDENTICAL to that in readtree.c
 */
struct rngtab
{
	char	relid[MAXNAME];
	char	rowner[2];
	char	rused;
};

extern struct rngtab	Rangev[];
int			Resultvar;
struct descriptor	Attdes;
int			Tl_elm;
int			Dom_num;
char			*Resrel;


/* 
** tree:	tl_clause ROOT tl_clause
**
**	prints out a tree assuming a mdVIEW-like mode
**
*/

pr_tree(root)
struct querytree	*root;
{

#	ifdef xZTR1
	if (tTf(10, -1))
		printf("pr_tree: root %u Resultvar %d Resrel %s\n",
		root, Resultvar, Resrel);
#	endif

	printf("%s ", pr_trim(resultres(),
		MAXNAME));

	pr_dom_init();
	Tl_elm = 0;

	/* print target list */
	printf("(\n");
	pr_tl_clause(root->left, TRUE);
	putchar(')');

	/* print qualification */
	if (root->right->sym.type != QLEND)
	{
                printf("\nwhere ");
		pr_qual(root->right);
	}
	putchar('\n');
}

/*
** tl_clause:	TREE
**	|	tl_clause RESDOM expr
**	
** target_flag = "in a target list (as opposed to in a by list)"
*/

pr_tl_clause(t_l, target_flag)
struct querytree	*t_l;
int			target_flag;
{
	register		fl;

	fl = target_flag;

#	ifdef xZTR1
	if (tTf(10, 1))
		printf("tl_clause target %d Tl_elm %d\n", fl, Tl_elm);
#	endif

	if (t_l->sym.type != TREE)
	{
		pr_tl_clause(t_l->left, fl);
		if (Tl_elm)
		{
			printf(", ");
			if (fl)
				putchar('\n');
		}
		/* print out info on result variable */
		pr_resdom(t_l, fl);
		pr_expr(t_l->right);
		Tl_elm++;
	}
}

/*
** print out info on a result attribute.
** this will be done only if the RESDOM node
** is inside a target_list and if the Resultvar >= 0.
** Resultvar == -1 inside a target list indicates that this is
** a retrieve to terminal.
*/

pr_resdom(resdom, target_flag)
struct querytree	*resdom;
int			target_flag;
{

#	ifdef xZTR1
	if (tTf(10, 2))
		printf("pr_resdom: target_flag %d\n", target_flag);
#	endif

	if (target_flag)
	{
		printf("\t");
		pr_attname(resultres(), ((struct qt_res *)resdom)->resno);
		printf(" = ");
	}
}

/* 
** give a relation name, and the attribute number of that
** relation, looks in the attribute relation for the name of the
** attribute.
*/


pr_attname(rel, attno)
char		*rel;
int		attno;
{
	struct tup_id		tid;
	struct attribute	key, tuple;
	register		i;

#	ifdef xZTR1
	if (tTf(10, 3))
		printf("pr_attname: rel %s attno %d\n",
		rel, attno);
#	endif

	if (attno == 0)
	{
		printf("tid");
		return;
	}
	opencatalog("attribute", 0);
	clearkeys(&Attdes);
	setkey(&Attdes, &key, rel, ATTRELID);
	setkey(&Attdes, &key, &attno, ATTID);
	i = getequal(&Attdes, &key, &tuple, &tid);
	if (i)
		syserr("pr_attname: bad getequal %d rel %s attno %d",
		i, rel, attno);
	printf("%s", pr_trim(tuple.attname, MAXNAME));
}

/*
** expr:	VAR
**	|	expr BOP expr
**	|	expr UOP
**	|	AOP AGHEAD qual
**		  \
**		   expr
**	|	BYHEAD	  AGHEAD qual
**	        /    \
**	tl_clause     AOP
**			\
**			 expr
**	|	INT
**	|	FLOAT
**	| 	CHAR
**	|	COP
**
*/

pr_expr(ex)
struct querytree	*ex;
{
	register			op;
	register			tl_elm;
	register struct querytree	*e;

	e = ex;
	switch (e->sym.type)
	{
	  case VAR:
		pr_var(e);
		break;

	  case BOP:
		if (((struct qt_op *)e)->opno == opCONCAT)
		{
			printf("concat(");
			pr_expr(e->left);
			printf(", ");
			pr_expr(e->right);
			putchar(')');
		}
		else
		{
			putchar('(');
			pr_expr(e->left);
			pr_op(BOP, ((struct qt_op *)e)->opno);
			pr_expr(e->right);
			putchar(')');
		}
		break;

	  case UOP:
		if ((op = ((struct qt_op *)e)->opno) == opMINUS || op == opPLUS || op == opNOT)
		{
			pr_op(UOP, ((struct qt_op *)e)->opno);
			pr_expr(e->left);
			putchar(')');
		}
		else
		{
			/* functional operators */
			pr_op(UOP, ((struct qt_op *)e)->opno);
			pr_expr(e->left);
			putchar(')');
		}
		break;

	  case AGHEAD:
		if (e->left->sym.type == AOP)
		{
			/* simple aggregate */
			pr_op(AOP, ((struct qt_op *)e->left)->opno);
			pr_expr(e->left->right);
			if (e->right->sym.type != QLEND)
			{
				printf("\where ");
				pr_qual(e->right);
			}
			putchar(')');
		}
		else
		{
			/* aggregate function */
			pr_op(AOP, ((struct qt_op *)e->left->right)->opno);
			pr_expr(e->left->right->right);
			printf(" by ");
			/* avoid counting target list elements
			 * in determining wether to put out
			 * commas after list's elements
			 */
			tl_elm = Tl_elm;
			Tl_elm = 0;
			pr_tl_clause(e->left->left, FALSE);
			Tl_elm = tl_elm;
			if (e->right->sym.type != QLEND)
			{
				printf("\n\t\twhere ");
				pr_qual(e->right);
			}
			putchar(')');
		}
		break;
	  
	  case INT:
	  case FLOAT:
	  case CHAR:
		pr_const(e);
		break;

	  case COP:
		pr_op(COP, ((struct qt_op *)e)->opno);
		break;

	  default:
		syserr("expr %d", e->sym.type);
	}
}
pr_const(ct)
struct querytree	*ct;
{
	register struct querytree	*c;
	register char			*cp;
	register			i;
	char				ch;
	double				d;

	c = ct;
	switch (c->sym.type)
	{
	  case INT:
		if (c->sym.len == 1)
			printf("%d", i1deref(c->sym.value));
		else if (c->sym.len == 2)
			printf("%d", i2deref(c->sym.value));
		else	/* i4 */
			printf("%d", i4deref(c->sym.value));
		break;

	  case FLOAT:
		if (c->sym.len == 4)
			d = f4deref(c->sym.value);
		else
			d = f8deref(c->sym.value);
		printf("%-10.3f", f8deref(c->sym.value));
		break;

	  case CHAR:
		printf("\"");
		cp = (char *) c->sym.value;
		for (i = c->sym.len; i--; cp++)
		{
			if (any(*cp, "\"\\[]*?") == TRUE)
				putchar('\\');

			if (*cp >= ' ')
			{
				putchar(*cp);
				continue;
			}
			/* perform pattern matching character replacement */
			switch (*cp)
			{
			  case PAT_ANY:
				ch = '*';
				break;
			
			  case PAT_ONE:
				ch = '?';
				break;
			
			  case PAT_LBRAC:
				ch = '[';
				break;

			  case PAT_RBRAC:
				ch = ']';
				break;

			  default:
				ch = *cp;
			}
			putchar(ch);
		}
		putchar('"');
		break;

	  default:
		syserr("bad constant %d", c->sym.type);
	}
}


/*
** pr_op: print out operator of a certain type
*/

pr_op(op_type, op_code)
int		op_type, op_code;
{
	register struct tab	*s;
	register int		opc;

	opc = op_code;
	switch (op_type)
	{
	  case UOP:
		s = &Uop_tab[opc];
		printf("%s(", s->t_string);
		break;

	  case BOP:
		s = &Bop_tab[opc];
		printf(" %s ", s->t_string);
		break;

	  case AOP:
		s = &Aop_tab[opc];
		printf("%s(", s->t_string);
		break;

	  case COP:
		s = &Cop_tab[opc];
		printf("%s", s->t_string);
		break;
	}
	if (opc != s->t_opcode)
		syserr("pr_op: op in wrong place type %d, code %d", op_type,
		s->t_opcode);
}

/*
** print a VAR node: that is, a var.attno pair
** at present the var part is the relation name over which var
** ranges.
*/

pr_var(var)
struct querytree	*var;
{
	register struct querytree	*v;

#	ifdef xZTR1
	if (tTf(10, 4))
		printf("pr_var(var=%d)\n", var);
#	endif
	v = var;
	pr_rv(((struct qt_var *)v)->varno);
	putchar('.');
	pr_attname(Rangev[((struct qt_var *)v)->varno].relid, ((struct qt_var *)v)->attno);
}

/*
** qual:	QLEND
**	|	q_clause AND qual
**
*/

pr_qual(ql)
struct querytree	*ql;
{
	register struct querytree	*q;


	q = ql;
	pr_q_clause(q->left);
	if (q->right->sym.type != QLEND)
	{
		printf(" and ");
		pr_qual(q->right);
	}
}

/*
** q_clause:	q_clause OR q_clause
**	|	expr
*/

pr_q_clause(qc)
struct querytree	*qc;
{
	register struct querytree	*q;


	q = qc;
	if (q->sym.type == OR)
	{
		pr_q_clause(q->left);
		printf(" or ");
		pr_q_clause(q->right);
	}
	else
		pr_expr(q);
}


char *pr_trim(string, len)
char	*string;
int	len;
{
	static char	buf[30];
	register	l;
	register char	*s;

	s = string;
	l = len;
	bmove(s, buf, l);
	for (s = buf; l && *s != ' ' && *s; --l)
		s++;
	*s = '\0';
	return (buf);
}

pr_dom_init()
{
	Dom_num = 0;
}

pr_ddom()
{
	printf("d%d = ", Dom_num++);
}

pr_range()
{
	register int	i;

	for (i = 0; i <= MAXVAR; i++)
	{
		if (Rangev[i].rused)
		{
			printf("range of ");
			pr_rv(i);
			printf(" is %s\n", 
			  pr_trim(Rangev[i].relid, MAXNAME));
		}
	}
}

pr_rv(re)
int	re;
{
	register	i, j;
	register char	ch;

	i = re;
	ch = Rangev[i].relid[0];

#	ifdef xZTR1
	if (tTf(10, 6))
		printf("pr_rv(%d) ch '%c'\n", i, ch);
#	endif
	
	for (j = 0; j <= MAXVAR; j++)
	{
		if (!Rangev[j].rused)
			continue;
		if (ch == Rangev[j].relid[0])
			break;
	}
	if (j < i)
		printf("rv%d", i);
	else
		printf("%c", ch);
}


char	*resultres()
{
	extern char	*Resrel;

#	ifdef xZTR1
	if (tTf(10, 5))
		printf("resultres : Resultvar %d, Resrel %s\n",
		Resultvar, Resrel);
#	endif
	if (Resultvar > 0)
		return (Rangev[Resultvar].relid);
	if (Resrel == 0)
		syserr("resultres: Resrel == 0");

	return (Resrel);
}

any(ch, st)
char	ch;
char	*st;
{
	register char	*s;
	register char	c;

	for (s = st, c = ch; *s; )
		if (*s++ == c)
			return (TRUE);
	return (FALSE);
}

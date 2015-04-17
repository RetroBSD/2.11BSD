# include	"../ingres.h"
# include	"../aux.h"
# include	"../tree.h"
# include	"../symbol.h"
# include	"ovqp.h"
/*
** TYPECHECK
**
**	Performs typechecking and conversions where appropriate .
**	Prohibits mixed type expressions.
*/

typecheck(pp1, pp2, opval)
struct symbol	*pp1, *pp2;
int		opval;
{
	register struct symbol	*p1, *p2;
	register int		i;

	p1 = pp1;
	p2 = pp2;

	i = (p1->type == CHAR & p2->type == CHAR);	/* c is true only if both are chars */
	switch (opval)
	{

	  case opCONCAT:
		if (!i)
			ov_err(NUMERIC);	/* numeric in a char operator */
		return;	/* else no further checking is needed */

	  case opADD:
	  case opSUB:
	  case opMUL:
	  case opDIV:
	  case opPOW:
	  case opMOD:
		if (i)
			ov_err(BADCHAR);	/* arithmetic operation on two character fields */
	}

	/* first check for identical types of symbols */
	if (p1->type == p2->type)
	{
		if (p1->len == p2->len)
			return;
		/* lengths are different. make p2 point to the smaller one */
		if (p1->len < p2->len)
		{
			p2 = p1;
			p1 = pp2;
		}

		switch (p2->type)
		{

		  case INT:
			if (p1->len == 4)
				i2toi4(p2);

		  case CHAR:
			return;	/* done if char or int */

		  case FLOAT:
			f8tof4(p2);
			return;
		}
	}

	/* at least one symbol is an INT or FLOAT. The other can't be a CHAR */
	if (p1->type == CHAR || p2->type == CHAR)
		ov_err(BADMIX);	/* attempting binary operation on one CHAR field with a numeric */

	/* one symbol is an INT and the other a FLOAT */
	if (p2->type == INT)
	{
		/* exchange so that p1 is an INT and p2 is a FLOAT */
		p1 = p2;
		p2 = pp1;
	}

	/* p1 is an INT and p2 a FLOAT */
	itof(p1);
	if (p2->len == 4)
		f8tof4(p2);
}


typecoerce(tosx, ntype, nlen)
struct stacksym	*tosx;
int		ntype;
int		nlen;

/*
**	Coerce the top of stack symbol to the
**	specified type and length. If the current
**	value is a character then it must be converted
**	to numeric. A user error will occure is the
**	char is not syntaxtically correct.
*/

{
	register struct stacksym	*tos;
	register char			*cp;
	register int			*val;
	int				ret;
	char				temp[256];

	tos = tosx;

	if (tos->type == CHAR)
	{
		val = tos->value;
		cp = temp;
		bmove(cpderef(tos->value), cp, tos->len & I1MASK);
		cp[tos->len & I1MASK] = '\0';
		if (ntype == FLOAT)
			ret = atof(cp, val);
		else
		{
			if (nlen == 4)
				ret = atol(cp, val);
			else
				ret = atoi(cp, val);
		}
		if (ret < 0)
			ov_err(CHARCONVERT);
		tos->type = ntype;
		tos->len = nlen;
	}
	else
		rcvt(tos, ntype, nlen);
}


i2toi4(pp)
struct symbol *pp;
{
	register struct symbol	*p;

	p = pp;

	i4deref(p->value) = i2deref(p->value);
	p->len = 4;
}


i4toi2(pp)
struct symbol *pp;
{
	register struct symbol	*p;

	p = pp;

	i2deref(p->value) = i4deref(p->value);
	p->len = 2;
}


itof(pp)
struct symbol *pp;
{
	register struct symbol	*p;

	p  = pp;

	if  (p->len == 4)
		f8deref(p->value) = i4deref(p->value);
	else
		f8deref(p->value) = i2deref(p->value);
	p->type = FLOAT;
	p->len= 8;
}


ftoi2(pp)
struct symbol *pp;
{
	register struct symbol	*p;

	p = pp;

	i2deref(p->value) = f8deref(p->value);
	p->type = INT;
	p->len = 2;
}


ftoi4(pp)
struct symbol *pp;
{
	register struct symbol	*p;

	p = pp;
	i4deref(p->value) = f8deref(p->value);
	p->type = INT;
	p->len = 4;
}


f8tof4(pp)
struct symbol	*pp;
{
	register struct symbol	*p;

	p = pp;
	f4deref(p->value) = f8deref(p->value);
	p->len = 4;
}

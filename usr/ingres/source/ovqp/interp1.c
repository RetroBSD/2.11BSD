# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../pipes.h"
# include	"ovqp.h"
/*
**	INTERP1.C
**
**	symbol I/O utility routines for OVQP interpreter.
**
*/

/*
** GETSYMBOL
**
**	Gets (type, len, value) symbols from list
**	A ptr to the list is advanced after
**	call.  Symbols are moved to a target location
**	(typically a slot on the interpreter's Stack).
**	Legality of symbol type and length is checked.
**	Returns		1  if no more symbols in list
**			0 otherwise
**
*/

getsymbol(ts, p)
struct symbol	*ts;		/* target location (on stack) */
struct symbol	***p;		/* pointer to list */
{
	int				len;	/*length of target symbol*/
	register char			*d;	/* ptr to data for target symbol */
	register char			*cp;	/* the item in the list */
	register struct stacksym	*tops;	/* target location on stack */

	tops = (struct stacksym *) ts;	/* copy stack pointer */
	cp = (char *) **p;		/* get list pointer */
#	ifdef xOTR1
	if(tTf(24, 0))
	{
		printf("GETSYM:");
		prsym(cp);
	}
#	endif

	if (((struct stacksym *)cp)->type == VAR || 
	    ((struct stacksym *)cp)->type == S_VAR)
	{
		tops->type = ((struct qt_v *)cp)->vtype;
		len = tops->len = ((struct qt_v *)cp)->vlen;
		d = (char *) ((struct qt_v *)cp)->vpoint;
	}
	else
	{
		tops->type = ((struct stacksym *)cp)->type;
		len = tops->len = ((struct stacksym *)cp)->len;
		len &= 0377;
		d = (char *) ((struct stacksym *)cp)->value;
	}
	/* advance Qvect sequencing pointer p */
	*p += 1;

	switch(tops->type)
	{
	  case INT:
		switch (len)
		{
		  case 1:
			*tops->value = *d;
			break;
		  case 2:
		  case 4:
			bmove(d, tops->value, len);
			break;

		  default:
			syserr("getsym:bad int len %d",len);
		}
		break;

	  case FLOAT:
		bmove(d, tops->value, len);
		if (len == 4)
			f8deref(tops->value) = f4deref(tops->value);	/* convert to double precision */
		else
			if (len != 8)
				syserr("getsym:bad FLOAT len %d",len);
		break;

	  case CHAR:
		cpderef(tops->value) = d;
		break;

	  case AOP:
	  case BOP:
	  case UOP:
	  case COP:
	  case RESDOM:
		/* all except aop are of length 1. aop is
		** length 6 but the first byte is the aop value
		*/
		*tops->value = *d & 0377;
		break;

	  case AND:
	  case OR:
		break;

	  case AGHEAD:
	  case BYHEAD:
	  case ROOT:
	  case QLEND:
		return (1);	/* all these are delimitors between lists */

	  default:
		syserr("getsym:bad type %d", tops->type);
	}
	return(0);
}



/*
*  TOUT
*
*	Copies a symbol value into the Output tuple buffer.
* 	Used to write target
*	list elements or aggregate values into the output tuple.
*/

tout(sp, offp, rlen)
struct symbol	*sp;
char		*offp;
{
	register struct symbol	*s;
	register int		i;
	register char		*p;
	int			slen;
	extern char		*bmove();

	s = sp;	/* copy pointer */

#	ifdef xOTR1
	if (tTf(24, 3))
	{
		printf("TOUT: s=");
		prstack(s);
		printf("  offset=%d, rlen=%d\n", offp-Outtup, rlen);
	}
#	endif
	if (s->type == CHAR)
	{
		slen = s->len & 0377;
		rlen &= 0377;
		i = rlen - slen;	/* compute difference between sizes */
		if (i <= 0)
			bmove(cpderef(s->value), offp, rlen);
		else
		{
			p = bmove(cpderef(s->value), offp, slen);
			while (i--)
				*p++ = ' ';	/* blank out remainder */
		}
	}
	else
	{
		bmove(s->value, offp, rlen);
	}
}



rcvt(tosp, restype, reslen)
struct symbol	*tosp;
int		restype, reslen;
{
	register struct symbol	*tos;
	register int		rtype, rlen;
	int			stype, slen;

	tos = tosp;
	rtype = restype;
	rlen = reslen;
	stype = tos->type;
	slen= tos->len;
#	ifdef xOTR1
	if (tTf(24, 6))
	{
		printf("RCVT:rt=%d, rl=%d, tos=", rtype, rlen);
		prstack(tos);
	}
#	endif

	if (rtype != stype)
	{
		if (rtype == CHAR || stype == CHAR)
			ov_err(BADCONV);	/* bad char to numeric conversion requested */
		if (rtype == FLOAT)
			itof(tos);
		else
		{
			if (rlen == 4)
				ftoi4(tos);
			else
				ftoi2(tos);
		}
		tos->len = rlen;	/* handles conversion to i1 or f4 */
	}

	else
		if (rtype != CHAR && rlen != slen)
		{
			if (rtype == INT)
			{
				if (rlen == 4)
					i2toi4(tos);
				else
					if (slen == 4)
						i4toi2(tos);
			}
			tos->len = rlen;	/* handles conversion to i1 or f4 */
		}
}

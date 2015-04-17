# include	"../ingres.h"
# include	"../aux.h"
# include	"../symbol.h"
# include	"../tree.h"
# include	"../access.h"
# include	"../pipes.h"
# include	"ovqp.h"



/*
**
** INTERPRET
**
**	 Processes the retrieved tuple from the Source relation
**	 according to the symbols in the list.  Recognition
**	 of characteristic delimiters and separators initiates
**	 action appropriate to a target list or qualification list
**	 as the case may be.
**	
**	 Between delimiters, the symbol list is expected to be in
**	 Polish postfix form.  A qualification list is further
**	 expected to be in conjunctive normal form, with Boolean
**	 operators infixed.
**
*/


double	pow();
double	sqrt();
double	log();
double	exp();
double	sin();
double	cos();
double	atan();
double	gamma();

struct symbol *interpret(list)

struct symbol	**list;	/* ptr to list of sym pointers */

{
	register struct stacksym	*tos;
	struct symbol			*op1,*op2;	/*operands popped off stack*/
	register int			*val1,*val2;		/*ptrs to values of operands*/
	int				opval, optype, l1;
	char				*s1;
	int				byflag;
	long				hitid;
	extern char			*Usercode;
	extern				ov_err();
	int				cb_mark;
	extern char			*Ovqpbuf;


#	ifdef xOTR1
	if (tTf(23, 0))
		printf("INTERP:  list=%l\n",list);
#	endif

#	ifdef xOTM
	if (tTf(76, 2))
		timtrace(11, 0);
#	endif

	byflag = (list == Bylist);	/* set byflag if aggregate function */
	tos = Stack-1;
	/* reset the concat and ascii operator buffer */
	seterr(Ovqpbuf, CBUFULL, ov_err);
	cb_mark = markbuf(Ovqpbuf);

loop:
#	ifdef xOTR1
	if (tTf(23, 1) && tos >= Stack)
	{
		printf("\ttops of stack=");
		prstack(tos);	/* print top of stack element */
	}
#	endif
	/* check for stack overflow */
	l1 = getsymbol(++tos, &list);

	if (l1)
	{
#		ifdef xOTM
		if (tTf(76, 2))
			timtrace(12, 0);
#		endif

		freebuf(Ovqpbuf, cb_mark);
		return ((struct symbol *) tos);
	}
	optype = tos->type;
	opval = *tos->value;
	tos--;	/* assume that stack will be poped */

	switch(optype)
	{
	  case INT:
	  case FLOAT:
	  case CHAR:
		++tos;		/* just leave symbol on stack */
		goto loop;

	  case COP:
		++tos;		/* new symbol goes on stack */
		tos->type = CHAR;
		switch (opval)
		{

		  case opDBA:
			cpderef(tos->value) = Admin.adhdr.adowner;
			tos->len = 2;
			goto loop;

		  case opUSERCODE:
			cpderef(tos->value) = Usercode;
			tos->len = 2;
			goto loop;
		}

	  case AND:		/* if top value false return immediately */
		if (!*tos->value)
		{
#			ifdef xOTM
			if (tTf(76, 2))
				timtrace(12, 0);
#			endif
			freebuf(Ovqpbuf, cb_mark);
			return((struct symbol *) tos);
		}
		else
			--tos;
		freebuf(Ovqpbuf, cb_mark);
		goto loop;

	  case OR:		/* if top value is true then skip to
				** end of disjunction. */
		if (*tos->value)
		{
			tos++;
			do
			{
				getsymbol(tos, &list);
			} while (tos->type != AND);
			optype = AND;
			--tos;
		}
		--tos;
		goto loop;

	  case RESDOM:
		freebuf(Ovqpbuf, cb_mark); /* init the concat and ascii buffer */
		if (Result)
		{
			if (opval)	/* if gt zero then opval represents a real domain */
			{
				if (byflag)
					opval++;	/* skip over count field for ag functs */
				rcvt(tos, Result->relfrmt[opval], Result->relfrml[opval]);
				tout(tos, Outtup+Result->reloff[opval], Result->relfrml[opval]);
			}
			else	/* opval refers to the tid and this is an update */
			{
				Uptid = i4deref(tos->value);	/* copy tid */
				if (Ov_qmode == mdREPL || (Diffrel && Ov_qmode == mdDEL && Result->relindxd > 0 ))
				{
					/* Origtup must be left with the orig
					** unaltered tuple, and Outtup must
					** be initialized with the orig tuple.
					**
					** Outtup only matters with REPL.
					** Scan() sets up Origtup so when
					** Source == Result, origtup is already
					** correct.
					*/

					if (Diffrel)
					{
						if (l1 = get(Result, &Uptid, &hitid, Origtup, CURTUP))
							syserr("interp:get on resdom %s, %d", locv(Uptid), l1);
						bmove(Origtup, Outtup, Result->relwid);
					}
					else
					{
						bmove(Intup, Outtup, Result->relwid);
					}
				}
			}
		}
		else
		{
			if (Equel)
				equelatt(tos);	/* send attribute to equel */
			else
			{
				if (tos->type == CHAR)
					s1 = cpderef(tos->value);
				else
					s1 = (char *) tos->value;
				printatt(tos->type, tos->len & 0377, s1);	/* print attribute */
			}
		}
		--tos;
		goto loop;


	  case BOP:
		op2 = (struct symbol *) tos--;
		op1 = (struct symbol *) tos;
		typecheck(op1, op2, opval);
		val1 = op1->value;
		val2 = op2->value;

		switch (tos->type)
		{
		  case INT:
			switch (tos->len)
			{
			  case 1:
			  case 2:
				switch (opval)
				{
				  case opADD:
					*val1 += *val2;
					goto loop;
				  case opSUB:
					*val1 -= *val2;
					goto loop;
				  case opMUL:
					*val1 *= *val2;
					goto loop;
				  case opDIV:
					*val1 /= *val2;
					goto loop;
				  case opMOD:
					*val1 %= *val2;
					goto loop;


				  case opPOW:
					itof(op1);
					itof(op2);
					f8deref(val1) = pow(f8deref(val1), f8deref(val2));
					goto loop;


				  /* relational operator */
				  default:
					l1 = *val1 - *val2;
					*val1 = relop_interp(opval, l1);
					goto loop;
				}

			  case 4:
				switch(opval)
				{
				  case opADD:
					i4deref(val1) += i4deref(val2);
					goto loop;

				  case opSUB:
					i4deref(val1) -= i4deref(val2);
					goto loop;

				  case opMUL:
					i4deref(val1) *= i4deref(val2);
					goto loop;

				  case opDIV:
					i4deref(val1) /= i4deref(val2);
					goto loop;

				  case opMOD:
					i4deref(val1) %= i4deref(val2);
					goto loop;

				  case opPOW:
					itof(op1);
					itof(op2);
					f8deref(val1) = pow(f8deref(val1), f8deref(val2));
					goto loop;

				  /* relational operator */
				  default: 
					tos->len =2;
					if (i4deref(val1) > i4deref(val2))
						l1 = 1;
					else
						if (i4deref(val1) == i4deref(val2))
							l1 = 0;
						else
							l1 = -1;

					*val1 = relop_interp(opval, l1);
					goto loop;

			    }
		}

		  case FLOAT:
			switch (opval)
			{
			  case opADD:
				f8deref(val1) += f8deref(val2);
				goto loop;

			  case opSUB:
				f8deref(val1) -= f8deref(val2);
				goto loop;

			  case opMUL:
				f8deref(val1) *= f8deref(val2);
				goto loop;

			  case opDIV:
				f8deref(val1) /= f8deref(val2);
				goto loop;

			  case opPOW:
				f8deref(val1) = pow(f8deref(val1), f8deref(val2));
				goto loop;

			  default:
				tos->type = INT;
				tos->len = 2;
				if (f8deref(val1) > f8deref(val2))
					l1 = 1;
				else
					if (f8deref(val1) == f8deref(val2))
						l1 = 0;
					else
						l1 = -1;
				*val1 = relop_interp(opval, l1);
				goto loop;
			}

		case CHAR:
			if (opval == opCONCAT)
			{
				concatsym(op1, op2);	/* concatenate the two symbols */
				goto loop;
			}
			l1 = lexcomp(cpderef(val1), op1->len & 0377, cpderef(val2), op2->len & 0377);
			tos->type = INT;
			tos->len = 2;
			*val1 = relop_interp(opval, l1);
			goto loop;
		}	/* end of BOP */
 
	   case UOP:
		val1 = tos->value;
		switch (opval)
		{
		   case opMINUS:
		   case opABS:
			if (tos->type == CHAR)
				ov_err(BADUOPC);
			l1 = opval == opMINUS;
			switch (tos->type)
			{
			   case INT:
				switch (tos->len)
				{
				  case 1:
				  case 2:
					if (l1 || (*val1 < 0))
						*val1 = -*val1;
			   		goto loop;

				  case 4:
					if (l1 || (i4deref(val1) < 0))
						i4deref(val1) = -i4deref(val1);
					goto loop;
				}

			  case FLOAT:
				if (l1 || (f8deref(val1) < 0.0))
					f8deref(val1) = -f8deref(val1);
				goto loop;
			}

		  case opNOT:
			*val1 = !*val1;
		  case opPLUS:
			if (tos->type == CHAR)
				ov_err(BADUOPC);
			goto loop;

		  case opASCII:
			ascii(tos);
			goto loop;

		  case opINT1:
			typecoerce(tos, INT, 1);
			goto loop;

		  case opINT2:
			typecoerce(tos, INT, 2);
			goto loop;

		  case opINT4:
			typecoerce(tos, INT, 4);
			goto loop;

		  case opFLOAT4:
			typecoerce(tos, FLOAT, 4);
			goto loop;

		  case opFLOAT8:
			typecoerce(tos, FLOAT, 8);
			goto loop;

		  default:
			if (tos->type == CHAR)
				ov_err(BADUOPC);
			if (tos->type == INT)
				itof(tos);
			switch (opval)
			{
			  case opATAN:
				f8deref(val1) = atan(f8deref(val1));
				goto loop;
	
			  case opGAMMA:
				f8deref(val1) = gamma(f8deref(val1));
				goto loop;
	
			  case opLOG:
				f8deref(val1) = log(f8deref(val1));
				goto loop;
	
			  case opSIN:
				f8deref(val1) = sin(f8deref(val1));
				goto loop;

			  case opCOS:
				f8deref(val1) = cos(f8deref(val1));
				goto loop;

			  case opSQRT:
				f8deref(val1) = sqrt(f8deref(val1));
				goto loop;

			  case opEXP:
				f8deref(val1) = exp(f8deref(val1));
				goto loop;

			  default:
				syserr("interp:bad uop %d",opval);
			}
		}


	   case AOP:
		aop_interp(opval, tos);
		goto loop;

	}
}



relop_interp(opval, l1)
int	opval;
int	l1;

/*
**	relop_interp interprets the relational operators
**	(ie. EQ, NE etc.) and returns true or false
**	by evaluating l1.
**
**	l1 should be greater than, equal or less than zero.
*/

{
	register int	i;

	i = l1;

	switch (opval)
	{

	  case opEQ:
		return (i == 0);

	  case opNE:
		return (i != 0);

	  case opLT:
		return (i < 0);

	  case opLE:
		return (i <= 0);

	  case opGT:
		return (i > 0);

	  case opGE:
		return (i >= 0);

	  default:
		syserr("relop:bad relop or bop %d", opval);
	}
}

aop_interp(opval, tosp)
struct stacksym	*tosp;

/*
**	Aggregate values are stored in Outtup. Tend points
**	to the spot for the next aggregate. Aop_interp()
**	computes the value for the aggregate and leaves
**	the result in the position pointed to by Tend.
*/

{
	register struct stacksym	*tos;
	register char			*number;
	register int			i;
	int				l1;
	char				numb[8];	/* used for type conversion */

	tos = tosp;
	number = numb;
	bmove(Tend, number, 8);	/* note: this assumes that there are always 8 bytes which can be moved.
				** if it moves beyond Tend, it's ok */
	switch (opval)
	{

	  case opSUMU:
	  case opSUM:
		if (*Counter <= 1)
			goto puta;
		switch (tos->type)
		{
		  case INT:
			switch(tos->len)
			{
			  case 1:
				i1deref(tos->value) += i1deref(number);
				goto puta;

			  case 2:
				i2deref(tos->value) += i2deref(number);
				goto puta;

			  case 4:
				i4deref(tos->value) += i4deref(number);
				goto puta;
			}

		  case FLOAT:
			if (tos->len == 4)
				f8deref(number) = f4deref(number);
			f8deref(tos->value) += f8deref(number);
			goto puta;

		  default:
			ov_err(BADSUMC);	/* cant sum char fields */
		}

	  case opCOUNTU:
	  case opCOUNT:
		tos->type = CNTTYPE;
		tos->len = CNTLEN;
		i4deref(tos->value) = *Counter;
		goto puta;

	  case opANY:
		tos->type = ANYTYPE;
		tos->len = ANYLEN;
		if (*Counter)
		{
			*tos->value = 1;
			if (!Bylist && (Agcount == 1))
				Targvc = 0;	/* if simple agg. stop scan */
		}
		else
			*tos->value = 0;
		goto puta;

	  case opMIN:
	  case opMAX:
		if (*Counter<=1)
			goto puta;
		switch (tos->type)
		{
		  case INT:
			switch (tos->len)
			{
			  case 1:
				i = (i1deref(tos->value) < i1deref(number));
				break;

			  case 2:
				i = (i2deref(tos->value) < i2deref(number));
				break;

			  case 4:
				i = (i4deref(tos->value) < i4deref(number));
				break;
			}
			break;

		  case FLOAT:
			if (tos->len == 4)
				f8deref(number) = f4deref(number);
			i  = (f8deref(tos->value) < f8deref(number));
			break;

		  case CHAR:
			l1 = tos->len & 0377;
			i = (lexcomp(cpderef(tos->value), l1, Tend, l1) < 0);
			break;

		  default:	
			syserr("interp:bad op type for opMIN/MAX %d", tos->type);
		}

		/* check result of comparison */
		if (opval == opMAX)
			i = !i;	/* complement test for opMAX */
		if (i)
			goto puta;	/* condition true. new value */

		/* condition false. Keep old value */
		goto done;


	  case opAVGU:
	  case opAVG:
		if (tos->type == INT)
			itof(tos);
		else
			if (tos->type == CHAR)
				ov_err(BADAVG);
		if (*Counter > 1)
		{
			f8deref(tos->value) = f8deref(number) + (f8deref(tos->value) - f8deref(number))/ *Counter;
		}
		tos->len = 8;
		goto puta;

	  default:
		syserr("interp:bad agg op %d", tos->type);
	}

puta:
	tout(tos, Tend, tos->len);
done:
	Tend += tos->len & 0377;
}

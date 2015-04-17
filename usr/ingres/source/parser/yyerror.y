/*
** This is the QUEL syntax-error diagnostic routine.   Yyerror()  is
**	called whenever the parser is unable to recognize the syntax of a
**	user query, or the scanner discovers a lexical error.
**	It returns via reset();
*/

/*
** The vector following is used to map error numbers for
**	incomplete commands.  The partial command is identified
**	by 'Opflag' and is used to index the vector.  The contents
**	of the vector location is the error number.  The 0 entries
**	are for those command which can never be incomplete without
**	generating a syntax error from the yacc parser.  In other
**	words, they should never happen.
*/
int	Messages[] =
{
	0,		/* mdRETTERM */
	2502,		/* mdRETR */
	2503,		/* mdAPP */
	2504,		/* mdREPL */
	2505,		/* mdDEL */
	0,		/* mdCOPY */
	0,		/* mdCREATE */
	2506,		/* mdDESTROY */
	2507,		/* mdHELP */
	0,		/* mdINDEX */
	2508,		/* mdMODIFY */
	2509,		/* mdPRINT */
	0,		/* mdRANGE */
	0,		/* mdSAVE */
	0,		/* mdDEFINE, not user specifiable */
	2510,		/* mdRET_UNI */
	2511,		/* mdVIEW */
	0,		/* mdUPDATE, not user specifiable */
	0,		/* mdRESETREL, not user specifiable */
	2512,		/* mdDISPLAY, help view, help integrity, help permit */
	0,		/* mdNETQRY, reserved for distr ingres */
	0,		/* mdMOVEREL, reserved for distr ingres */
	2513,		/* mdPROT */
	2514,		/* mdINTEG */
	0,		/* mdDCREATE, reserved for distr ingres */
	0,		/* mdWAITQRY, reserved for distr ingres */
	2515		/* mdREMQM, destroy permit, destroy integrity */
};

yyerror(num, a, b, c)
int	num;
char	*a, *b, *c;
{
	char		buff[30];
	register char	*buf;

	buf = buff;
	if (num == SYMERR || num == NXTCMDERR)
	{
		/* syntax error */
		a = buf;
		b = 0;
		switch (Lastok.toktyp)
		{
		  case I2CONST:
			itoa(i2deref(Lastok.tok), buf);
			break;

		  case I4CONST:
			smove(locv(i4deref(Lastok.tok)), buf);
			break;

		  case F4CONST:
			ftoa(f4deref(Lastok.tok), buf, 10, 3, 'n');
			break;

		  case F8CONST:
			ftoa(f8deref(Lastok.tok), buf, 10, 3, 'n');
			break;

		  case SCONST:
			smove(Lastok.tok, buf);
			break;

		  case 0:
			a = "EOF";
			break;

		  default:
			syserr("bad Lastok format");
		}
	}
#	ifdef	xPTR3
	tTfp(13, 0, "yyerror: %d\n", num);
#	endif
	if (num == NXTCMDERR)
	{
		num = Messages[Opflag];
		b = 0;
	}
	error(num, iocv(yyline), a, b, c, 0);

	/* clean up and sync with monitor */
	endgo();

	/* execute non-local goto */
	reset();
}
int
neederr(errnum)
int	errnum;
{
	yyerror(errnum, 0);
}

# include "../ingres.h"
# include "../scanner.h"

/*
** COMMENT
** scans comments (as delimited by the tokens 'Tokens.bgncmnt'
** and 'Tokens.endcmnt') and removes them from the query text.
*/
comment()
{
	register int		i, l;
	register struct optab	*op;
	char			buf[3];

	/* find the end_of_comment operator */
	for (op = Optab; op->term; op++)
		if (op->token == Tokens.endcmnt)
			break;
	if (!op->term)
		syserr("no end_of_comment token");

	/* scan for the end of the comment */
	l = length(op->term);
	for (i = 0; i < l; i++)		/* set up window on input */
		if ((buf[i] = gtchar()) <= 0)
			/* non-terminated comment */
			yyerror(COMMTERM, 0);
	while (!bequal(buf, op->term, l))
	{
		/* move window on input */
		for (i = 0; i < l-1; i++)
			buf[i] = buf[i+1];
		if ((buf[l-1] = gtchar()) <= 0)
			/* non terminated comment */
			yyerror(COMMTERM, 0);
	}
	return (0);
}

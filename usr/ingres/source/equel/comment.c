#
/*
**  COMMENT.C -- routine to scan comments inside an equel statement
**
**	Uses the endcmnt token code to find the endcmnt
**	terminal string, then reads the input until it sees this 
**	terminal (must be <= 2 characters), returning EOF_TOK and
**	giving an error diagnostic if end-of-file is encountered.
**
**	Parameters:
**		none
**
**	Returns:
**		CONTINUE -- valid comment
**		EOF_TOK -- EOF in comment
**
**	Side Effects:
**		deletes comments from within an equel statement
**
**	Defines:
**		comment()
**
**	Requires:
**		Optab -- for endcomment terminal
**		Tokens -- for endcmnt token code
**		bequal()
**		getch()
**		sequal()
**		yysemerr()
**		syserr()
**
**	Called By:
**		operator()
**
**	Files:
**		"globals.h" -- for globals
**		"constants.h" -- for constants
**
**	Diagnostics:
**		"premature EOF encountered in comment"
**
**	Syserrs:
**		"no end of comment operator in the parse tables" --
**			"end-of-comment" missing from Optab
**		"buf too short for endcmnt %s %d" --
**			the ENDCMNT token is longer than can fit in buf
**
**	History:
**		5/31/78 -- (marc) modified for equel from
**			rick's quel comment routine.
*/

# include	<stdio.h>

# include	"constants.h"
# include	"globals.h"

comment()
{
	register int		i, l;
	register struct optab	*op;
	char			buf [3];

	/* find end of comment operator */
	for (op = Optab; op->op_term; op++)
		if (op->op_token == Tokens.sp_endcmnt)
			break;

	if (!op->op_term)
		syserr("no end of comment operator in the parse tables");
	/* scan for the end of comment */
	l = length(op->op_term);
	if (l > sizeof buf - 1)
		syserr("comment : buf too short for endcmnt %s %d",
		op->op_term, l);

	/* fill buffer to length of endmnt terminal */
	for (i = 0; i < l; i++)		
	{
		if ((buf [i] = getch()) == EOF_TOK)
		{
nontermcom :
			/* non-terminated comment */
			yysemerr("premature EOF encountered in comment", 0);
			return (EOF_TOK);
		}
	}

	/* shift on input until endcmnt */
	while (!bequal(buf, op->op_term, l))
	{
		for (i = 0; i < l - 1; i++)
			buf [i] = buf [i + 1];
		if ((buf [l - 1] = getch()) == EOF_TOK)
			goto nontermcom;
	}
	return (CONTINUE);
}

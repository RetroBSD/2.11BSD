#
# include	<stdio.h>

# include	"constants.h"
# include	"globals.h"

/*
**  NAME -- Process an identifier or keyword token.
**
**	Name gets the identifier that follows in the std.
**	input, and checks if it is a keyword.
**	An identifier is defined as a sequence of
**	MAXNAME or fewer alphanumerics, starting with an
**	alphabetic character.
**
**	Parameters:
**		chr - the first character of the identifier
**
**
**	Returns:
**		Tokens.sp_name - for a user-defined name
**		Tokens.sp_struct_var -- if the name is declared 
**			a structurw variable
**		other - lexical codes for keys
**
**	Side Effects:
**		Adds a token to the symbol space.
**		yylval is set to the new node in the space.
**		If the identifier is a keyword, sets Opcode to
**		op_code from tokens.y.
**
**	Defines:
**		name()
**
**	Requires:
**		yylval - to return a value for the token
**		Cmap - to get character classes
**		Opcode - to return opcodes of keywords
**		Tokens - for the lex codes of NAMEs
**		getch() - to get next character
**		yysemerr() - to report buffer ovflo
**		salloc() - to allocate place for a string
**		addsym() - to add a symbol to the symbol space
**		backup() - to push back a single char on the input stream
**		getkey() - to search the keyword table for one
**		getcvar() - to see if the id is a struct id
**
**	Called By:
**		yylex()
**
**	Files:
**		globals.h - for globals
**		constants.h
**
**	Diagnostics:
**		"name too long"	- along with the first
**				  MAXNAME chars of the name,
**				  (which is what the name is 
**				  truncated to)
**
**	History:
**		first written - 4/19/78 (marc)
*/

name(chr)
char		chr;
{
	int			lval;
	register		i;
	char			wbuf [MAXNAME + 1];
	register char		*cp;
	register char		c;
	struct optab		*op;
	extern struct optab	*getkey();
	extern struct cvar	*getcvar();

	c = chr;
	cp = wbuf;
	for (i = 0; i <= MAXNAME; i++)
	{
		lval = Cmap [c];
		if (i < MAXNAME &&
		   (lval == ALPHA || lval == NUMBR))
		{
			*cp++ = c;
			c = getch();
		}
		else if (lval == ALPHA || lval == NUMBR)
		{
			/* {i == MAXNAME && "c is legal" && 
			 *  cp == &wbuf [MAXNAME]} 
			 */
			*cp = '\0';
			yysemerr("name too long", wbuf);
			/* chomp to end of identifier */

			do
			{
				c = getch();
				lval = Cmap [c];
			}  while (lval == ALPHA || lval == NUMBR);
			backup(c);
			
			/* take first MAXNAME characters as IDENTIFIER 
			 * (non-key)
			 */
			yylval = addsym(salloc(wbuf));
			return (Tokens.sp_name);
		}
		else
		{
			/* {cp <= &wbuf [MAXNAME] && i <= MAXNAME
			 * && "c is not part of id"}
			 */
			backup(c);
			*cp = '\0';
			i = 0;
			break;
		}
	}
	op = getkey(wbuf);

	/* Is it a keyword ? */
	if (op)
	{
		yylval = addsym(op->op_term);
		Opcode = op->op_code;
		return (op->op_token);
	}
	/* user-defined name */
	yylval = addsym(salloc(wbuf));
	if (getcvar(wbuf)->c_type == opSTRUCT)
		return(Tokens.sp_struct_var);
	return (Tokens.sp_name);
}

#
# include	<stdio.h>

# include	"constants.h"
# include	"globals.h"


/*
**  YYERROR -- Error reporting routines
**
**	Defines:
**		yyerror() 
**		yyserror()
**		yysemerr()
**
**	History:
**		6/1/78 -- (marc) written
*/


/*
**  YYERROR -- Yacc error reporting routine.
**	Yyerror reports on syntax errors encountered by 
**	the yacc parser.
**
**	Parameters:
**		s -- a string explaining the error
**
**	Returns:
**		none
**
**	Called By:
**		yyparse()
**
**	History:
**		6/1/78 -- (marc) written
*/


yyerror(s)
char	*s;
{
	extern char	*yysterm [];

	file_spec();
	if (yychar == 0)
		printf("EOF = ");
	else
		printf("%s = ", yysterm [yychar - 0400]);

	if (yylval)
		printf("'%s' : ", ((struct disp_node *)yylval)->d_elm);

	printf("line %d, %s\n",
	yyline, s);
}

/*
**  YYSEMERR -- scanner error reporter
**
**	Parameters:
**		s -- string explaining the error
**		i -- if !0 a string which caused the error
**
**	Returns:
**		none
**
**	Called By:
**		lexical analysis routines -- if called from somewhere else,
**			the line number is likely to be wrong.
**
**	History:
**		6/1/78 -- (marc) written
*/


yysemerr(s, i)
char		*s;
char		*i;
{
	char	*str;
	extern char	*yysterm[];

	file_spec();
	if (i)
		printf("'%s' : ", i);
	printf("line %d, %s\n",
	yyline, s);
}

/*
**  YYSERROR -- Semantic error reportin routine
**	reports on an error on an entry in the symbol space,
**	using the line number built into the entry.
**
**	Parameters:
**		s -- a string explaining the error
**		d -- a symbol space node
**
**	Returns:
**		none
**
**	Called By:
**		semantic productions
**
**	History:
**		6/1/78 -- (marc) written
*/


yyserror(s, d)
char			*s;
struct disp_node	*d;
{
	file_spec();
	printf("'%s' : line %d, %s\n",
	d->d_elm, d->d_line, s);
}

/*
**  FILE_SPEC -- If in an included file, specify the name of that file.
**
**	History:
**		6/10/78 -- (marc) written
**
*/


file_spec()
{
	if (Inc_files)
		printf("** %s : ", Input_file_name);
}

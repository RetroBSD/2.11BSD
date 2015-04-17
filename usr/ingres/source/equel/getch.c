#
# include	<stdio.h>

# include	"constants.h"
# include       "globals.h"

/*
**  GETCH.C -- I/O manipulation routines
**
**	Defines:
**		getch()
**		backup()
**		int Peekc[2]  -- 2 char stack, if [0]  != -1, the character to 
**			be returned on the next call to getch().
**		char *Line_pos -- if != 0 *Line_pos is next 
**			character that should be returned.
**		char Line_buf [MAXSTRING + 1] -- holds current
**			input line's text.
**
**	Files:
**		globals.h
**
**	Compilation Flags:
**		xDEBUG -- to test Chardebug for 
**			returning activity of getch() and backup().
**
**	History:
**		4/20/78 - written (marc)
**		9/5/78 -- Peekc made Peekc [2] to allow correct
**			processing in eat_display() [display.c]
**			Also include processing put below this
**			abstraction level
**
*/

/*
**  GETCH -- Get a character from the input stream
**
**	Parameters:
**		none
**
**	Returns:
**		the next character on the input stream, or
**		the backed up character if there is one.
**		EOF_TOK is returned on EOF.
**
**	Side Effects:
**		If a backed up character exists, unloads it.
**		*Line_pos is returned if Line_pos != 0
**		and no backup character exists. Line_buf 
**		contains the line from which characters are being
**		returned. A call with Line_pos == 0 forces 
**		reading of the next input line. Yyline is 
**		incremented when a newline is returned. 
**		If an EOF is in the middle of a line,
**		then a newline will be appended.
**
**	Requires:
**		getc() - from utility library
**		tst_include() - [include.c]
**
**	Diagnostics:
**		"WARNING : line %d too long, broken in two\n" --
**			a input line may contain at most MAXSTRING
**			characters, including the terminating
**			newline.
**
**	Called By:
**		all lexical analisys routines
**		include() looks at Line_buf and manipulates
**		Line_pos to skip the "#include"d lines.
**
**	History:
**		4/20/78 - written (marc)
**		9/5/78 -- modified (marc)
*/


/* initializes peek buffer to be empty */
int		Peekc [2]	= {-1, -1};

getch()
{
	register char	*cp;
	register char	ch;
	static		eoflag;
	extern int	yyline;
	extern FILE 	*In_file;

	if (Peekc [0] >= 0)
	{
		/* have a backed up character */
		ch = Peekc [0];
		if (ch == '\n')
			yyline += 1;
		Peekc [0] = Peekc [1];
		Peekc [1] = -1;
	}
	else
	{
		for ( ; ; )
		{
			/* no lookahead character */
			if (!Line_pos)
			{
				if (eoflag)
				{
					eoflag = 0;

					/* try to restore previous file */
					if (!restoref())
						return (0);
#					ifdef xDEBUG
					if (Chardebug || Lex_debug)
						printf("include file - pop\n");
#					endif
				}
				for (cp = Line_buf; (*cp = getc(In_file)) != '\n'
						    && *cp != EOF; cp++)
				{
					if (cp - Line_buf > sizeof Line_buf - 1)
					{
						printf("WARNING : line %d too long, broken in two\n",
						yyline);
						break;
					}
				}
				if (*cp == EOF)
				{
					eoflag++;
					if (cp == Line_buf)
						/* EOF after '\n' */
						continue;
					/* EOF in middle of line */
					*cp = '\n';
				}
				Line_pos = Line_buf;

				/* test for a "#include" line */
				if (tst_include())
				{
					/* Force reloading Line_buf */
					Line_pos = 0;
					eoflag = 0;
#					ifdef xDEBUG
					if (Chardebug || Lex_debug)
						printf("include file - push\n");
#					endif
					continue;
				}
			}
			cp =  Line_pos;
			if (*cp == '\n')
			{
				Line_pos = 0;
				yyline += 1;
			}
			else
				Line_pos++;
			ch = *cp;
			break;
		}
	}
	ch &= 0377;

#	ifdef xDEBUG
	if (Chardebug)
		printf("getch - returning '%c'.\n", ch);
#	endif

	return (ch);
}

/*
**  BACKUP -- Back up a character on the input stream.
**	Backs up a single character into Peekc.
**
**	Parameters:
**		ch - character to back up
**
**	Returns:
**		none
**
**	Side Effects:
**		pushes Peekc [0] to Peekc [1].
**		sets Peekc [0] to backed up character.
**
**	Requires:
**		Peekc [2]  -- integers.
**
**	Called By:
**		most lexical analysis routines.
**
**	Syserrs:
**		"backup buffer overflow on line %d, \"%c%c\"." -
**			can only back up two character.
**	History:
**		4/20/78 - written (marc)
**		9/5/78 -- added on backup character (marc)
**
*/

backup(ch)
char		ch;
{
	extern int	yyline;


#	ifdef xDEBUG
	if (Chardebug)
		printf("backed up : '%c'\n", ch);
#	endif

	if (Peekc [1] >= 0)
		syserr("backup buffer overflow on line %d, \"%c%c%c\".",
		yyline, Peekc [0], Peekc [1], ch);
	
	Peekc [1] = Peekc [0];
	if ((Peekc [0] = ch & 0377) == '\n')
		--yyline;
}

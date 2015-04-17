%{
/*
 * oldassn.l - find old C assignment operators
 *
 * Quick little lex program to find occurances of old form assignment
 * operators in C source programs: =-, =+, etc.
 *
 * usage: oldassn [file] ...
 *
 * NOTES:
 *	Won't work on files with lines longer than MAXLINE
 *	File names should be less than MAXPATHLEN in length ...
 *	The standard lex environment already includes <stdio.h>
 *	"yyin" is where lex reads its input from
 *	lex keeps the current input line number in "yylineno"
 */

#define	MAXLINE		1024
#define MAXPATHLEN	1024

char	*ourname,		/* guess */
	**filelist;		/* list of files we're checking */
int	errors	= 0;		/* number of errors of course! */

main(argc, argv)
	int	argc;
	char	**argv;
{
	ourname = *argv;
	if (argc == 1)
	{
		static char	*siv[] = { "stdin", (char *)0 };
		filelist = siv;
	}
	else
	{
		filelist = argv+1;
		if (yyopen() == 1)
			exit(errors);
	}
	yylex();
	exit(errors);
}

/*
 * Open next file when end of file reached on current input.
 */
yywrap()
{
	filelist++;
	return(yyopen());
}

/*
 * Open next file from filelist and set it up as lex's input.  Return 1
 * when no more files can be opened, 0 on success.
 */
yyopen()
{
	fclose(yyin);
	yylineno = 1;
	for (;;)
	{
		if (!*filelist)
			return(1);
		if (yyin = fopen(*filelist, "r"))
			return(0);
		else
		{
			fprintf(stderr, "%s: can't open %s\n", ourname, *filelist);
			errors++;
			filelist++;
		}
	}
}

/*
 * Catch cpp "# lineno file" controls and use them to set our current idea of
 * what file we're reading and what line number we're on.
 */
yypunt(yytext)
	char	*yytext;
{
	static char	curfile[MAXPATHLEN];
	register char	*cp	= curfile,
			*yp	= yytext;

	while (*yp < '0' || '9' < *yp)
		yp++;
	yylineno = *yp++ - '0';
	while ('0' <= *yp && *yp <= '9')
		yylineno = 10*yylineno + *yp++ - '0';

	while (*yp++ != '\"')
		continue;
	while (*yp != '\"')
		*cp++ = *yp++;
	*cp = '\0';
	*filelist = curfile;
}
%}

%%

%{
	/*
	 * The following definitions will appear within the function yylex.
	 * Note that yylex must *not* return except at an end of line or at
	 * end of file or the line buffer variable will be trashed (and *no*,
	 * you can't have a "static register ..."  The use of register
	 * variables for "linep", "bufend" and "cp" below in the macro
	 * catenate speeds this program up by about 10%).
	 */
	static char	linebuf[MAXLINE+1];	/* current input line */
	register char	*linep	= linebuf,	/* linebuf input pointer */
			*bufend	= linebuf+MAXLINE+1,
			*cp;
	static int	oldassn	= 0,		/* old assignment in line */
			comment	= 0,		/* in a comment */
			string	= 0;		/* in a string */

#	define	catenate(s) \
	{ \
		cp = s; \
		while (linep < bufend && *cp) \
			*linep++ = *cp++; \
		*linep = '\0'; \
	}
%}

^\#(line)?\ [0-9]+\ \".*\"\n	yypunt(yytext);

[ \t]*			catenate(yytext);

[-+*/%&^|=!]=		|
>>=|<<=			catenate(yytext);

=[-+*/%&^|]		|
=>>|=<<			{
				catenate(yytext);
				if (!comment && !string)
					oldassn = 1;
			}

"/*"			{
				catenate(yytext);
				if (!comment && !string)
					comment = 1;
			}
"*/"			{
				catenate(yytext);
				if (comment)
					comment = 0;
			}
\'\"\'|\\\"		catenate(yytext);
\"			{
				catenate(yytext);
				if (!comment)
					string = !string;
			}
.			catenate(yytext);
\n			{
				/*
				 * If an old assignment occured on this line,
				 * then print the file name, line number and
				 * and line ala grep.
				 */
				 if (oldassn)
				 	printf("%s: %d: %s\n",
						*filelist,
						yylineno-1,
						linebuf);
				*(linep = linebuf) = '\0';
				oldassn = 0;
			}


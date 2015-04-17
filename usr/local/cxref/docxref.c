/*
** docxref.c
**
** Driver for lex based scanner.  Arranges for stdin to be each named
** file in turn, so that yylex() never has to know about files.
** Some of this code is not pretty, but it's not too bad.
**
** Arnold Robbins, Information and Computer Science, Georgia Tech
**	gatech!arnold
** Copyright (c) 1984 by Arnold Robbins.
** All rights reserved.
** This program may not be sold, but may be distributed
** provided this header is included.
*/

#include <stdio.h>
#include <ctype.h>

#define TRUE	1
#define FALSE	0

extern char yytext[];
extern int yyleng;

int line_no = 1;	/* current line number */
char *fname = NULL;	/* current file name */

char *basename();	/* strips leading path of a file name */

main(argc,argv)
int argc;
char **argv;
{
	FILE saved_in, *fp;
	char *name;
	int more_input = FALSE;		/* more files to process */
	int read_stdin = FALSE;

	name = basename(argv[0]);	/* save command name */
	fname = "stdin";		/* assume stdin */

	if(argc == 1)
	{
		yylex();
		exit(0);
	}

	if(argv[1][0] == '-' && argv[1][1] != '\0')
			usage(argv[0]);	/* will exit */

	saved_in = *stdin;
	/* save stdin in case "-" is found in middle of command line */

	for(--argc, argv++; argc > 0; --argc, argv++)
	{
		if(fileno(stdin) != fileno((&saved_in)) || read_stdin)
			fclose(stdin);
		/* free unix file descriptors */

		if(strcmp(*argv, "-") == 0)
		{
			*stdin = saved_in;
			fname = "stdin";
			read_stdin = TRUE;
			more_input = (argc - 1 > 0);
		}
		else if((fp = fopen(*argv,"r")) == NULL)
		{
			fprintf(stderr,"%s: can't open %s\n", name, *argv);
			continue;
		}
		else
		{
			*stdin = *fp;
			/* do it this way so that yylex() */
			/* never knows about files etc. */
			more_input = (argc - 1 > 0);
			fname = *argv;
		}

		yylex();	/* do the work */

		if(more_input)
			line_no = 1;
	}
}

outid()
{
	char *basename();

	printf("%s\t%s\t%d\n", yytext, basename(fname), line_no);
}

usage(name)
char *name;
{
	fprintf(stderr,"usage: %s [files]\n", name);
	exit(1);
}

#include "basename.c"

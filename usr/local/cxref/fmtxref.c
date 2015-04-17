/*
** fmtxref.c
**
** format the output of the C cross referencer.
** this program relies on the fact that its input
** is sorted and uniq-ed.
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

#define MAXID	121	/* maximum lengths of identifiers, file names, etc. */
#define MAXFILE	15
#define MAXLINE	121
#define MAXNUM	7

#define ID	1	/* return codes to indicate what is new on input line */
#define NEWFILE	2
#define LINE	3
#define ERROR	4

#define WIDTH	80	/* default line output width */

int width = WIDTH;

char prev_id[MAXID] = "";
char prev_file[MAXFILE] = "";

char id[MAXID] = "";
char file[MAXFILE] = "";
char line[MAXNUM] = "";

char *name;
char *basename();	/* strips leading path name */

main(argc, argv)
int argc;
char **argv;
{
	char inline[BUFSIZ];
	char *gets();
	int val;

	name = basename(argv[0]);

	/*
	 * since this program is ONLY called by the cxref driver,
	 * we know that it will be called "fmtxref -w width"
	 * so we don't have to do complicated argument parsing.
	 * we also know that cxref makes sure we get a valid width.
	 */

	if (argc > 1)
		if (argc == 3)
			if (strcmp(argv[1], "-w") == 0)
				width = atoi(argv[2]);
			else
				usage();
		else
			usage();
	/* else
		use default width */

	if(gets(inline) == NULL)
	{
		fprintf(stderr, "%s: standard input is empty.\n", name);
		exit(1);
	}

	if((val = breakup(inline)) == ERROR)
	{
		fprintf(stderr, "%s: malformed input '%s'\n", name, inline);
		exit(2);
	}

	output(val);		/* does proper formatting */

	while(gets(inline) != NULL && val != ERROR)
	{
		val = breakup(inline);
		output(val);
	}

	if(val == ERROR)
	{
		fprintf(stderr, "%s: malformed input '%s'\n", name, inline);
		exit(2);
	}

	putchar('\n');
}

breakup(text)
char *text;
{
	int retval;
	int i, j;

	if(text[0] == '"' || text[0] == '\'')
	{
		/* quoted stuff, break the line up by hand */

		i = 0;
		id[i++] = text[0];

		for(j = 1; text[j] != text[0]; i++, j++)
		{
			id[i] = text[j];
			if(id[i] == '\\')
				id[++i] = text[++j];	/* e.g. \" */
		}
		id[i++] = text[0];
		id[i] = '\0';
		j++;	/* skip close quote */

		while(isspace(text[j]))
			j++;
		
		for(i = 0; !isspace(text[j]); i++, j++)
			file[i] = text[j];
		file[i] = '\0';


		while(isspace(text[j]))
			j++;

		for(i = 0; !isspace(text[j]) && text[j] != '\0'; i++, j++)
			line[i] = text[j];
		line[i] = '\0';
	}
	else
	{
		if(sscanf(text, "%s %s %s", id, file, line) != 3)
			return(ERROR);
	}

	/* now decide about return code for formatting */

	if(strcmp(prev_id, id) != 0)	/* different identifiers */
	{
		strcpy(prev_id, id);
		strcpy(prev_file, file);
		retval = ID;
	}
	else if(strcmp(prev_file, file) != 0)	/* different files */
	{
		strcpy(prev_file, file);
		retval = NEWFILE;
	}
	else
		retval = LINE;

	return(retval);
}

output(val)
int val;
{
	static int curpos = 1;
	static int first = TRUE;
	int line_len = strlen(line);

	switch(val) {
	case ID:
		if(! first)
			putchar('\n');	/* finish off last line of prev id */
		else
			first = FALSE;

		printf("%-20.20s\t%-14.14s\t%s", id, file, line);
		curpos = 40 + line_len;
		break;

	case NEWFILE:
		printf("\n\t\t\t%-14.14s\t%s", file, line);
		curpos = 40 + line_len;
		break;

	case LINE:
		if(curpos + line_len + 2 < width)
		{
			printf(", %s", line);	/* same output line */
			curpos += 2 + line_len;
		}
		else
		{
			printf(",\n\t\t\t\t\t%s", line);	/* new line */
			curpos = 40 + line_len;
		}
		break;

	case ERROR:
		/* shouldn't come to here */
		fprintf(stderr, "%s: internal error: output() called with %s\n",
			name, "a value of ERROR");
		fprintf(stderr, "%s: id == '%s'\tfile == '%s'\tline == '%s'\n",
			name, id, file, line);
		break;

	default:
		/* shouldn't come to here either */
		fprintf(stderr, "%s: internal error: output() called with %s %d\n",
			name, "the unknown value", val);
		fprintf(stderr, "%s: id == '%s'\tfile == '%s'\tline == '%s'\n",
			name, id, file, line);
		break;
	}
}

usage()
{
	char *basename();

	fprintf(stderr, "usage: %s [-w width]\n", basename(name));
	exit (1);
}

#include "basename.c"

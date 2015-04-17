/*
** cxrfilt.c
**
** if called with no flags, or with the -c or -s flags, it will
** separate out integer and floating point constants into
** their own files, pass char and string constants on through.
** input: sorted output of docxref, contains identifiers and constants.
** output: identifiers, char and string constants, depending on flags.
**      output goes to fmtxref. floats and ints to separate files for sorting.
**
** if called with -i or -f option, behavior is to put sorted ints or floats
** back into their original formats, and then pass the output to fmtxref.
**
** originally, there was a separate program to do float and int, but these two
** have been merged to reduce the total number of programs needed for cxref.
**
** Arnold Robbins, Information and Computer Science, Georgia Tech
**	gatech!arnold
** Copyright (c) 1984 by Arnold Robbins
** All rights reserved
** This program may not be sold, but may be distributed
** provided this header is included.
*/

#include <stdio.h>
#include "constdefs.h"

#define MAXFILE		120
#define MAXLINE		120

FILE *fp1, *fp2;
int cflag = 0;
int sflag = 0;
char *name;
char *basename();

main(argc, argv)
int argc;
char **argv;
{
	char buf[BUFSIZ];
	char file1[MAXFILE], file2[MAXFILE];
	int i;

	name = basename(argv[0]);

	if (argc <= 1)
		usage();

	if(argv[1][0] == '-')
	{
		for (i = 1; argv[1][i] != '\0'; i++)
			switch (argv[1][i]) {
			case 'c':
				cflag = 1;
				break;

			case 's':
				sflag = 1;
				break;

			case 'i':
				intfilter();
				exit(0);
				break;
			
			case 'f':
				floatfilter();
				exit(0);
				break;
			
			default:	/* bad option given */
				usage();
				break;
			}

		/* if it gets to here, we were called only w/-c or -s */
		if (argc == 2)
			usage();

		argv++;
	}

	/* code for splitting constants off into separate files */

	sprintf(file1, "/tmp/cxr.%d.1", atoi(argv[1]));
	sprintf(file2, "/tmp/cxr.%d.2", atoi(argv[1]));

	if ((fp1 = fopen(file1, "w")) == NULL)
	{
		fprintf(stderr,"%s: couldn't create tempfile 1\n");
		exit (2);
	}

	if ((fp2 = fopen(file2, "w")) == NULL)
	{
		fprintf(stderr,"%s: couldn't create tempfile 2\n");
		exit (3);
	}

	while (gets(buf) != NULL)
	{
		if (buf[0] != '~')
			printf("%s\n", buf);
		else
			switch (buf[1]) {
			case CHAR:
				if (! cflag)
					printf("%s\n", &buf[2]);
				break;

			case STRING:
				if (! sflag)
					printf("%s\n", &buf[2]);
				break;
			
			case INT:
				outint(buf);
				break;
			
			case FLOAT:
				outfloat(buf);
				break;
			
			default:
				fprintf(stderr,"%s: bad input line '%s'\n",
					name, buf);
				exit (4);
			}
	}

	fclose(fp1);
	fclose(fp2);

	exit(0);
}

#define OCTAL	1
#define HEX	2
#define DEC	3

outint(buf)
char *buf;
{
	char file[MAXLINE], line[MAXLINE];
	int val;
	int type = 0;

	buf += 2;		/* skip leading ~INT */
	file[0] = line[0] = '\0';

	if (buf[0] == '0')	/* octal or hex */
	{
		if (buf[1] == 'x' || buf[1] == 'X')	/* hex */
		{
			type = HEX;
			buf += 2;	/* skip leading 0x */
			sscanf(buf, "%x %s %s", &val, file, line);
		}
		else
		{
			type = OCTAL;
			sscanf(buf, "%o %s %s", &val, file, line);
		}
	}
	else
	{
		type = DEC;
		sscanf(buf, "%d %s %s", &val, file, line);	/* decimal */
	}

	/*
	 * strategy is to convert to decimal for numeric sorting,
	 * then have output filter convert back to right base.
	 *
	 * type is used to tell intfilter() what to turn it back into.
	 */

	fprintf(fp1, "%d\t%s\t%s\t%d\n", val, file, line, type);
}

outfloat(buf)
char *buf;
{
	char file[MAXLINE], line[MAXLINE];
	char mantissa[MAXLINE], exponent[MAXLINE];
	char strval[MAXLINE];		/* character representation of float */
	char controlstr[MAXLINE];
	double val;
	int i, j;

	buf += 2;	/* skip ~FLOAT */

	mantissa[0] = exponent[0] = file[0] = line[0] = '\0';

	sscanf(buf, "%lf %s %s", &val, file, line);

	for (i = 0; buf[i] != '\t'; i++)
		if (buf[i] == '.')
			break;

	for (j = i + 1; buf[j] != 'E' && buf[j] != 'e' && buf[j] != '\t'; j++)
		;

	j -= i - 1;	/* j is now num digits to right decimal point. */
	if (j < 6)
		j = 6;	/* default */

	sprintf(controlstr, "%%1.%dg", j);	/* make control string */
	sprintf(strval, controlstr, val);	/* make character string */

	/*
	 * strategy is a follows:
	 * 1) convert all floats to a common printed format (%g)
	 * 2) split up mantissa and exponent into separate parts for sorting
	 * 3) put them back together later when called w/-f option.
	 */

	for(i = j = 0; strval[j] != 'e' && strval[j] != 'E' && strval[j] != '\0'; i++, j++)
		mantissa[i] = strval[j];
	mantissa[i] = '\0';

	if (strval[j] == 'e' || strval[j] == 'E')
	{
		j++;
		for(i = 0; strval[j] != '\0'; i++, j++)
			exponent[i] = strval[j];
		exponent[i] = '\0';
	}
	else
		exponent[0] = '\0';
	
	fprintf(fp2, "%s\t%s\t%s\t%s\n", mantissa,
		exponent[0] != '\0' ? exponent : "0", file, line);
}

usage()
{
	fprintf(stderr, "usage: %s [-csfi] pid\n", name);
	exit (1);
}


intfilter()	/* put sorted ints back into their original bases */
{
	char buf[BUFSIZ];
	char file[MAXLINE], number[MAXLINE];
	int val;
	int type;

	while (gets(buf) != NULL)
	{
		sscanf(buf, "%d %s %s %d", &val, file, number, &type);

		switch (type) {
		case OCTAL:
			if (val == 0)		/* don't print 00 */
				printf("0\t%s\t%s\n", file, number);
			else
				printf("0%o\t%s\t%s\n", val, file, number);
				/* supply leading 0 */
			break;

		case DEC:
			printf("%d\t%s\t%s\n", val, file, number);
			break;
		
		case HEX:
			printf("0x%x\t%s\t%s\n", val, file, number);
			break;
		
		default:
			fprintf(stderr,"%s: bad input line '%s'\n", name, buf);
			exit (4);
		}
	}
}

floatfilter()	/* put sorted floats back together */
{
	char buf[BUFSIZ];
	char file[MAXLINE], number[MAXLINE];
	char mantissa[MAXLINE], exponent[MAXLINE];

	while (gets(buf) != NULL)
	{
		sscanf(buf, "%s %s %s %s", mantissa, exponent, file, number);

		if (strcmp(exponent, "0") == 0)
			printf("%s", mantissa);
		else
			printf("%sE%s", mantissa, exponent);
		
		printf("\t%s\t%s\n", file, number);
	}
}

#include "basename.c"

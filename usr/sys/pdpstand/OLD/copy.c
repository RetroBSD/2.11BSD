/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)bcopy.c	1.1 (2.10BSD Berkeley) 7/25/87
 */

#define BLKSIZE 1024
char module[] = "copy";			/* module name -- used by trap */
char buffer[BLKSIZE];
main()
{
	int c, o, i;
	char buf[50];

	printf("%s\n",module);
	do {
		printf("Infile: ");
		gets(buf);
		i = open(buf, 0);
	} while (i <= 0);
	do {
		printf("Outfile: ");
		gets(buf);
		o = open(buf, 1);
	} while (o <= 0);

	while ((c = read(i, buffer, BLKSIZE)) > 0)
		write(o, buffer, c);
	exit(0);
}

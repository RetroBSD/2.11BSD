/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)cat.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

char module[] = "cat";		/* module name -- used by trap */
main()
{
	int c, i;
	char buf[50];

	printf("%s\n",module);
	do {
		printf("File: ");
		gets(buf);
		i = open(buf, 0);
	} while (i <= 0);

	while ((c = getc(i)) > 0)
		putchar(c);
	exit(0);
}

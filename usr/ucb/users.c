/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	!defined(lint) && defined(DOSCCS)
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)users.c	5.2.1 (2.11BSD) 1996/11/17";
#endif

/*
 * users
 */

#include <stdio.h>
#include <utmp.h>

#define NMAX sizeof(utmp.ut_name)
#define LMAX sizeof(utmp.ut_line)

struct utmp utmp;

main(argc, argv)
char **argv;
{
	register char *tp, *s;
	register FILE *fi;

	s = _PATH_UTMP;
	if(argc == 2)
		s = argv[1];
	if ((fi = fopen(s, "r")) == NULL) {
		perror(s);
		exit(1);
	}
	while (fread((char *)&utmp, sizeof(utmp), 1, fi) == 1) {
		if(utmp.ut_name[0] == '\0')
			continue;
		putline();
	}
	summary();
}

char	*names[128];
char	**namp = names;
putline()
{
	char temp[NMAX+1];
	strncpy(temp, utmp.ut_name, NMAX);
	temp[NMAX] = 0;
	*namp = (char *)malloc(strlen(temp) + 1);
	strcpy(*namp++, temp);
}

scmp(p, q)
char **p, **q;
{
	return(strcmp(*p, *q));
}
summary()
{
	register char **p;

	qsort(names, namp - names, sizeof names[0], scmp);
	for (p=names; p < namp; p++) {
		if (p != names)
			putchar(' ');
		fputs(*p, stdout);
	}
	if (namp != names)		/* at least one user */
		putchar('\n');
}

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	defined(DOSCCS) && !defined(lint)
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";

static char sccsid[] = "@(#)strip.c	5.1.1 (2.11BSD GTE) 1/1/94";
#endif

#include <a.out.h>
#include <signal.h>
#include <stdio.h>
#include <sys/file.h>

struct	xexec head;
int	status;

main(argc, argv)
	char *argv[];
{
	register i;

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	for (i = 1; i < argc; i++) {
		strip(argv[i]);
		if (status > 1)
			break;
	}
	exit(status);
}

strip(name)
	char *name;
{
	register f;
	long size;

	f = open(name, O_RDWR);
	if (f < 0) {
		fprintf(stderr, "strip: "); perror(name);
		status = 1;
		goto out;
	}
	if (read(f, (char *)&head, sizeof (head)) < 0 || N_BADMAG(head.e)) {
		printf("strip: %s not in a.out format\n", name);
		status = 1;
		goto out;
	}
	if ((head.e.a_syms == 0) && ((head.e.a_flag & 1) != 0))
		goto out;

	size = N_DATOFF(head) + head.e.a_data;
	head.e.a_syms = 0;
	head.e.a_flag |= 1;
	if (ftruncate(f, size) < 0) {
		fprintf("strip: "); perror(name);
		status = 1;
		goto out;
	}
	(void) lseek(f, (long)0, L_SET);
	(void) write(f, (char *)&head.e, sizeof (head.e));
out:
	close(f);
}

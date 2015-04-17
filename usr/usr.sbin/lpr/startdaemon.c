/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if	defined(DOSCCS) && !defined(lint)
static char sccsid[] = "@(#)startdaemon.c	5.1.2 (2.11BSD GTE) 1996/3/22";
#endif

/*
 * Tell the printer daemon that there are new files in the spool directory.
 */

#include <stdio.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "lp.local.h"

startdaemon(printer)
	char *printer;
{
	struct sockaddr_un sun;
	register int s, n;
	char buf[BUFSIZ];

	s = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s < 0) {
		extern errno;

		if (errno == EPROTONOSUPPORT) {
			char current[40];
			char *cp;
			FILE *fp;

			if ((fp = fopen(MASTERLOCK, "r")) == NULL) {
				perr("fopen");
				return(0);
			}
			cp = current;
			while ((*cp = getc(fp)) != EOF && *cp != '\n')
				cp++;
			*cp = '\0';
			if (kill(atoi(current), SIGUSR1)) {
				perr("kill");
				return(0);
			}
			return(1);
		}
		perr("socket");
		return(0);
	}
	sun.sun_family = AF_UNIX;
	strcpy(sun.sun_path, SOCKETNAME);
	if (connect(s, &sun, strlen(sun.sun_path) + 2) < 0) {
		perr("connect");
		(void) close(s);
		return(0);
	}
	(void) sprintf(buf, "\1%s\n", printer);
	n = strlen(buf);
	if (write(s, buf, n) != n) {
		perr("write");
		(void) close(s);
		return(0);
	}
	if (read(s, buf, 1) == 1) {
		if (buf[0] == '\0') {		/* everything is OK */
			(void) close(s);
			return(1);
		}
		putchar(buf[0]);
	}
	while ((n = read(s, buf, sizeof(buf))) > 0)
		fwrite(buf, 1, n, stdout);
	(void) close(s);
	return(0);
}

static
perr(msg)
	char *msg;
{
	extern char *name;

	printf("%s: %s: %s\n", name, msg, strerror(errno));
}

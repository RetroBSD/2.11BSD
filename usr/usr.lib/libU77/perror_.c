/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)perror_.c	5.1.1	1996/3/22
 */

/*
 * write a standard error message to the standard error output
 *
 * calling sequence:
 *	call perror(string)
 * where:
 *	string will be written preceeding the standard error message
 */

#include	<stdio.h>
#include	"../libI77/fiodefs.h"
#include	"../libI77/f_errno.h"
#include	<string.h>

extern char *f_errlist[];
extern int f_nerr;
extern unit units[];

perror_(s, len)
char *s; long len;
{
	register unit	*lu;
	register char	*mesg = s + len;

	while (len > 0 && *--mesg == ' ')
		len--;
	if (errno >= F_ER && errno < (F_ER + f_nerr))
		mesg = f_errlist[errno - F_ER];
	else
		mesg = strerror(errno);
	lu = &units[STDERR];
	if (!lu->uwrt)
		nowwriting(lu);
	while (len-- > 0)
		fputc(*s++, lu->ufd);
	fprintf(lu->ufd, ": %s\n", mesg);
}

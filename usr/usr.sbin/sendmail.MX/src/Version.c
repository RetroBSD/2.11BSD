/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *  Sendmail
 *  Copyright (c) 1983  Eric P. Allman
 *  Berkeley, California
 */

#if !defined(lint) && !defined(NOSCCS)
static char sccsid[] = "@(#)version.c	5.59 (Berkeley) 3/13/88";
#endif /* not lint */

char	Version[] = "5.59";

# ifdef COMMENT

code versions:

alias.o
arpadate.o
clock.o
collect.o
conf.o
convtime.o
daemon.o
deliver.o
domain.o
envelope.o
err.o
headers.o
macro.o
main.o
	sendmail.h	5.11		3/13/88
parseaddr.o
queue.o
readcf.o
recipient.o
savemail.o
srvrsmtp.o
stab.o
stats.o
sysexits.o
trace.o
usersmtp.o
util.o

# endif COMMENT

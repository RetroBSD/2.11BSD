#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.2 (2.11BSD) 1997/10/17
#
CFLAGS=	-O
SEPFLAG= -i
DESTDIR=

more:	more.o
	${CC} ${SEPFLAG} -o more more.o -ltermcap

install: more more.help
	install -m 755 -s more ${DESTDIR}/usr/ucb/more
	rm -f ${DESTDIR}/usr/ucb/page
	ln ${DESTDIR}/usr/ucb/more ${DESTDIR}/usr/ucb/page
	install -m 444 -c more.help ${DESTDIR}/usr/share/misc/more.help

clean:
	rm -f more more.o core a.out errs

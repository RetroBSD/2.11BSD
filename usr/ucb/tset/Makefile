#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4.1 (2.11BSD) 1997/3/28
#
DESTDIR=
CFLAGS=-O
SEPFLAG= -i

all: tset tset.0

tset:	tset.o
	${CC} ${SEPFLAG} ${CFLAGS} tset.o -o tset -ltermlib

tset.0:	tset.1
	/usr/man/manroff tset.1 > tset.0

install: all
	install -s tset ${DESTDIR}/usr/ucb
	rm -f ${DESTDIR}/usr/ucb/reset
	ln ${DESTDIR}/usr/ucb/tset ${DESTDIR}/usr/ucb/reset
	rm -f ${DESTDIR}/usr/man/cat1/reset.0
	install -c -o bin -g bin -m 444 tset.0 ${DESTDIR}/usr/man/cat1/tset.0
	ln ${DESTDIR}/usr/man/cat1/tset.0 ${DESTDIR}/usr/man/cat1/reset.0

clean:
	rm -f *.o tset tset.o tset.0 tags

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

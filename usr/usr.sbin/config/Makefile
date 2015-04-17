#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.3.1 (2.11BSD) 1996/11/27
#
#
CFILES=	config.c
OBJS=	config.o
SEPFLAG= -i

CFLAGS=-O
LDFLAGS=-ll

all:	config

config: ${OBJS}
	${CC} ${OBJS} ${SEPFLAG} -o config ${LDFLAGS}

clean:
	rm -f ${OBJS} lex.yy.c y.tab.c y.tab.h config

install: config
	install -s config ${DESTDIR}/usr/sbin/config

lint: ${CFILES}
	lint ${CFILES}

depend:	${CFILES}
	mkdep ${CFLAGS} ${CFILES}

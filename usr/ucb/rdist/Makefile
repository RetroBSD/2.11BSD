#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.4 (2.11BSD GTE) 1997/7/3
#
DESTDIR=
SEPFLAG= -i
XSTR=	xstr
YACC=	yacc
SRCS =	docmd.c expand.c gram.y lookup.c main.c server.c
OBJS =	docmd.o expand.o gram.o lookup.o main.o server.o
LINT =	lint -ha
CFLAGS= -O

.c.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
	${CC} ${CFLAGS} -c x.c
	mv -f x.o $*.o
	rm -f x.c

rdist:	${OBJS} strings.o
	${CC} ${SEPFLAG} -o rdist ${OBJS} strings.o -lstubs

${OBJS}:  defs.h

strings.o: strings
	${XSTR}
	${CC} -c xs.c
	mv -f xs.o strings.o
	rm -f xs.c

gram.c: gram.y
	${YACC} gram.y
	mv y.tab.c gram.c

clean:
	rm -f ${OBJS} gram.c errs rdist strings strings.o x.c xs.c y.tab.c

install: rdist
	install -s -m 4751 rdist ${DESTDIR}/usr/ucb/rdist

lint:	docmd.c expand.c gram.c lookup.c main.c server.c
	${LINT} docmd.c expand.c gram.c lookup.c main.c server.c

print:	${SRCS}
	lpr -p ${SRCS} defs.h

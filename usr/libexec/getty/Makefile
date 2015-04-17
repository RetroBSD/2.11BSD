#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.2 (2.11BSD) 1997/4/28
#
DESTDIR=
CFLAGS=	-O
SEPFLAG=-i

OBJS=	main.o init.o subr.o gettytab.o get_date.o

all:	getty

getty:	${OBJS}
	${CC} ${SEPFLAG} -o getty ${OBJS}

install: all
	install -s -m 555 -o bin -g bin getty ${DESTDIR}/usr/libexec/getty

clean:
	rm -f getty ${OBJS} a.out core errs

main.o: gettytab.h
init.o: gettytab.h
subr.o: gettytab.h

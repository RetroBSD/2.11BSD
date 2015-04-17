#
#	@(#)Makefile	1.4	(Berkeley)	83/06/30
#
DESTDIR=
SRCS=	dc.c dc.h
CFLAGS=	-O
SEPFLAG= -i

all: dc

dc:	dc.c dc.h
	${CC} ${CFLAGS} ${SEPFLAG} dc.c -o dc

install: dc
	install -s dc ${DESTDIR}/usr/bin/dc

clean:
	rm -f *.o dc a.out errs core

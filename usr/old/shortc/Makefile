#
# shortc Makefile.  Revised 1996/1/28
#
DESTDIR=

DEFS=
CFLAGS=	-O ${DEFS}
SEPFLAG=-i

all: shortc shortc.0

shortc:	shortc.o
	cc ${CFLAGS} ${SEPFLAG} shortc.o -o shortc

install: shortc shortc.0
	install -s shortc ${DESTDIR}/usr/new/shortc
	install -c -m 444 -o bin -g bin shortc.0 ${DESTDIR}/usr/new/man/cat1

shortc.0: shortc.1
	/usr/man/manroff shortc.1 > shortc.0

clean:
	rm -f shortc shortc.0 *.o MAKELOG

depend:
tags:
lint:

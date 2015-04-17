#
# oldassn Makefile.  Revised 1996/1/28
#
DESTDIR=

DEFS=
CFLAGS=	-O ${DEFS}
SEPFLAG=-i

all: oldassn oldassn.0

oldassn: oldassn.l
	lex oldassn.l
	cc ${CFLAGS} ${SEPFLAG} -w -o oldassn lex.yy.c
	rm lex.yy.c

install: oldassn oldassn.0
	install -s oldassn ${DESTDIR}/usr/new/oldassn
	install -c -m 444 -o bin -g bin oldassn.0 ${DESTDIR}/usr/new/man/cat1

oldassn.0: oldassn.1
	/usr/man/manroff oldassn.1 > oldassn.0

clean:
	rm -f oldassn *.o lex.yy.c MAKELOG oldassn.0

depend:
tags:
lint:

#	@(#)Makefile	4.3.1	(2.11BSD)	1996/10/23
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

# The programs themselves
#
PROG=	find
LIBS=	bigram code

# Sources
#
SRCS=	find.c bigram.c code.c

all: ${PROG} ${LIBS}

find: find.o
	cc ${SEPFLAG} ${CFLAGS} -o find find.o

bigram: bigram.o
	cc ${SEPFLAG} ${CFLAGS} -o bigram bigram.o

code: code.o
	cc ${SEPFLAG} ${CFLAGS} -o code code.o

install: ${PROG} ${LIBS}
	install -s -m 755 ${PROG} ${DESTDIR}/usr/bin/${PROG}
	-mkdir ${DESTDIR}/var/db ${DESTDIR}/usr/libexec/find
	chmod 755 ${DESTDIR}/usr/libexec/find ${DESTDIR}/var/db
	install -s -m 755 bigram ${DESTDIR}/usr/libexec/find/bigram
	install -s -m 755 code ${DESTDIR}/usr/libexec/find/code
	install -c -m 755 updatedb.csh ${DESTDIR}/usr/libexec/find/updatedb

clean:
	rm -f *.o a.out core ${LIBS} ${PROG}

depend:
	chmod u+w Makefile
	mkdep ${CFLAGS} ${SRCS}

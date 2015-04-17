#
# Public Domain.  1999/2/19 - Steven Schultz
#
#	@(#)Makefile	1.2 (2.11BSD) 1999/2/19
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	acctd.c
OBJS=	acctd.o
MAN=	acctd.0
MANSRC=	acctd.8

all: acctd acctd.0

acctd: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

acctd.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} acctd tags ${MAN}

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: acctd
	install -s -o root -g bin -m 700 acctd ${DESTDIR}/usr/libexec/acctd
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8/${MAN}

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

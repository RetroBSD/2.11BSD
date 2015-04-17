#
#	@(#)Makefile	1.1 (2.11BSD) 1996/1/28
#

CFLAGS=	-O
SEPFLAG= -i
SRCS=	ping.c
OBJS=	ping.o
MAN=	ping.0
MANSRC=	ping.8

all: ping ${MAN}

ping: ${OBJS}
	${CC} ${SEPFLAG} -o $@ ${CFLAGS} ${OBJS}

ping.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} ping

cleandir: clean
	rm -f tags .depend

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: ${MAN} ping
	install -s -o root -g bin -m 4751 ping ${DESTDIR}/bin
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8

lint: ${SRCS}
	lint -haxc ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

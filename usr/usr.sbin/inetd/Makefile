#
# Public Domain.  1996/11/6 - Steven Schultz
#
#	@(#)Makefile	1.1 (2.11BSD) 1996/11/13
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	inetd.c
OBJS=	inetd.o
MAN=	inetd.0
MANSRC=	inetd.8

all: inetd inetd.0

inetd: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

inetd.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} inetd tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: inetd
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -s -o root -g bin -m 755 inetd ${DESTDIR}/usr/sbin/inetd

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

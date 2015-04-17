#
# Public Domain.  1996/11/16 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	rlogind.c
OBJS=	rlogind.o
MAN=	rlogind.0
MANSRC=	rlogind.8

all: rlogind rlogind.0

rlogind: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS} -lutil

rlogind.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} rlogind tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: rlogind
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -s -o root -g bin -m 755 rlogind ${DESTDIR}/usr/libexec/rlogind

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

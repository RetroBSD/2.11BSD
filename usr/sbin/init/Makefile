#
# Public Domain.  1997/1/10 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1997/1/10
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	init.c
OBJS=	init.o
MAN=	init.0
MANSRC=	init.8

all: init init.0

init: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

init.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} init tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: init
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	-mv /etc/init /etc/init.old
	install -s -o root -g bin -m 700 init ${DESTDIR}/etc/init

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

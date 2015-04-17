#
# Public Domain.  1997/3/27 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1997/3/27
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	stty.c
OBJS=	stty.o
MAN=	stty.0
MANSRC=	stty.1

all: stty stty.0

stty: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

stty.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} stty tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: stty
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
	install -s -o root -g bin -m 755 stty ${DESTDIR}/bin/stty

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

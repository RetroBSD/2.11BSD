#
# Public Domain.  2/4/1995 - Steven Schultz
#
#	@(#)Makefile	1.1 (2.11BSD GTE) 2/3/95
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	uname.c
OBJS=	uname.o
MAN=	uname.0
MANSRC=	uname.1

all: uname uname.0

uname: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

uname.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} uname tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
	install -s -o root -g bin -m 755 uname ${DESTDIR}/usr/bin/uname

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

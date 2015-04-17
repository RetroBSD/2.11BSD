#
# Public Domain.  1996/11/16 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	badsect.c
OBJS=	badsect.o
MAN=	badsect.0
MANSRC=	badsect.8

all: badsect badsect.0

badsect: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

badsect.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} badsect tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: badsect
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -s -o root -g bin -m 755 badsect ${DESTDIR}/sbin/badsect

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

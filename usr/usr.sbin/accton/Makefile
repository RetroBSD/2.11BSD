#
# Public Domain.  1996/11/16 - Steven Schultz
#
#	@(#)Makefile	1.1 (2.11BSD) 1999/2/19
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	accton.c
OBJS=	accton.o
MAN=	accton.0
MANSRC=	accton.8

all: accton accton.0

accton: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

accton.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} accton tags ${MAN}

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: accton
	install -s -o root -g bin -m 755 accton ${DESTDIR}/usr/sbin/accton
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8/${MAN}

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

#
# Public Domain.  2/3/1995 - Steven Schultz
#
#	@(#)Makefile	1.1 (2.11BSD GTE) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	dev_mkdb.c
OBJS=	dev_mkdb.o
MAN=	dev_mkdb.0
MANSRC=	dev_mkdb.8

all: dev_mkdb dev_mkdb.0

dev_mkdb: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

dev_mkdb.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} dev_mkdb tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -s -o root -g bin -m 755 dev_mkdb ${DESTDIR}/usr/sbin/dev_mkdb

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

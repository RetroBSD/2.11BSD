#
# 1997/6/25 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1997/6/25
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	id.c
OBJS=	id.o
MAN=	id.0 groups.0 whoami.0

.SUFFIXES: .0 .1

.1.0:
	/usr/man/manroff $*.1 > $@

all: id ${MAN}

id: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

clean:
	rm -f ${OBJS} ${MAN} id tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: id
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
	install -s -o root -g bin -m 755 id ${DESTDIR}/usr/bin/id
	install -c -o bin -g bin -m 755 groups.sh ${DESTDIR}/usr/bin/groups
	install -c -o bin -g bin -m 755 whoami.sh ${DESTDIR}/usr/bin/whoami

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

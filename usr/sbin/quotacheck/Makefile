#
# Public Domain.  1996/1/23 - Steven Schultz
#
#	@(#)Makefile	1.1 (2.11BSD GTE) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	quotacheck.c preen.c
OBJS=	quotacheck.o preen.o
MAN=	quotacheck.0
MANSRC=	quotacheck.8

all: quotacheck quotacheck.0

quotacheck: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

quotacheck.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} quotacheck tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -s -o root -g bin -m 755 quotacheck ${DESTDIR}/sbin/quotacheck

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

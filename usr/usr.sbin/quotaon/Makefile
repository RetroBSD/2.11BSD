#
# Public Domain.  1996/11/16 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	quotaon.c
OBJS=	quotaon.o
MAN=	quotaon.0
MANSRC=	quotaon.8

all: quotaon quotaon.0

quotaon: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

quotaon.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} quotaon tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: quotaon
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	-rm -f ${DESTDIR}/usr/man/cat8/quotaoff.0
	ln ${DESTDIR}/usr/man/cat8/quotaon.0 ${DESTDIR}/usr/man/cat8/quotaoff.0
	install -s -o root -g bin -m 755 quotaon ${DESTDIR}/usr/sbin/quotaon
	-rm -f ${DESTDIR}/usr/sbin/quotaoff
	ln ${DESTDIR}/usr/sbin/quotaon ${DESTDIR}/usr/sbin/quotaoff

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

#
# Public Domain.  1996/11/16 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	arff.c
OBJS=	arff.o
MAN=	arff.0
MANSRC=	arff.8

all: arff arff.0

arff: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

arff.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} arff tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: arff
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	-rm -f ${DESTDIR}/usr/man/cat8/flcopy.0
	ln ${DESTDIR}/usr/man/cat8/arff.0 ${DESTDIR}/usr/man/cat8/flcopy.0
	install -s -o root -g bin -m 755 arff ${DESTDIR}/usr/sbin/arff

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

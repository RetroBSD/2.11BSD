#
# Public Domain.  1996/3/14 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1996/3/14
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	mkerrlst.c
OBJS=	mkerrlst.o
MAN=	mkerrlst.0
MANSRC=	mkerrlst.1

all: mkerrlst mkerrlst.0

mkerrlst: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS} -lerrlst

mkerrlst.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} mkerrlst tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: mkerrlst
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
	install -s -o root -g bin -m 755 mkerrlst ${DESTDIR}/usr/bin/mkerrlst

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

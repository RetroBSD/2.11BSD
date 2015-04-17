#
# Public Domain.  1999/7/21 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1999/7/21
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	gt307.c
OBJS=	gt307.o
MAN=	gt307.0
MANSRC=	gt307.8

all: gt307 gt307.0

gt307: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

gt307.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} gt307 tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: gt307
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -s -o root -g bin -m 755 gt307 ${DESTDIR}/sbin/gt307

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

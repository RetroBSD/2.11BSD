#
# Public Domain.  1996/11/16 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	mknod.c
OBJS=	mknod.o
MAN=	mknod.0
MANSRC=	mknod.8

all: mknod mknod.0

mknod: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

mknod.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} mknod tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: mknod
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -s -o root -g bin -m 755 mknod ${DESTDIR}/sbin/mknod

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

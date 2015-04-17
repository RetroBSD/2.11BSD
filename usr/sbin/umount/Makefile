#
# Public Domain.  1996/1/15 - Steven Schultz
#
#	@(#)Makefile	1.1 (2.11BSD GTE) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	umount.c
OBJS=	umount.o
MAN=	umount.0
MANSRC=	umount.8

all: umount umount.0

umount: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

umount.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} umount tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -s -o root -g bin -m 755 umount ${DESTDIR}/sbin/umount

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

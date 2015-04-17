#
# Public Domain.  1996/1/15 - Steven Schultz
#
#	@(#)Makefile	1.1 (2.11BSD GTE) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	mount.c getmntopts.c mount_ufs.c
OBJS=	mount.o getmntopts.o mount_ufs.o
#
# We do NOT install the getmntopts.3 man page
#
MAN=	mount.0
MANSRC=	mount.8

all: mount mount.0

mount: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

mount.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} mount tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -s -o root -g bin -m 755 mount ${DESTDIR}/sbin/mount

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

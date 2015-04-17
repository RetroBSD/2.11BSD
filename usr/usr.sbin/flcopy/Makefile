#
# Public Domain.  1996/11/16 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	flcopy.c
OBJS=	flcopy.o

all: flcopy

flcopy: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

clean:
	rm -f ${OBJS} flcopy tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: flcopy
	install -s -o root -g bin -m 755 flcopy ${DESTDIR}/usr/sbin/flcopy

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

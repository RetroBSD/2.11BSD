#
# Public Domain.  1995/03/13 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1995/03/13
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	xargs.c
OBJS=	xargs.o

all: xargs xargs.0

xargs: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

xargs.0: xargs.1
	/usr/man/manroff xargs.1 > xargs.0

clean:
	rm -f ${OBJS} tags xargs xargs.0

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -s -o bin -g bin -m 755 xargs ${DESTDIR}/usr/bin
	install -c -o bin -g bin -m 444 xargs.0 ${DESTDIR}/usr/man/cat1

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

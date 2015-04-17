#
# Public Domain.  1995/03/13 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1995/03/13
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	hostname.c
OBJS=	hostname.o

all: hostname hostname.0

hostname: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

hostname.0: hostname.1
	/usr/man/manroff hostname.1 > hostname.0

clean:
	rm -f ${OBJS} tags hostname hostname.0

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -s -o bin -g bin -m 755 hostname ${DESTDIR}/bin
	install -c -o bin -g bin -m 444 hostname.0 ${DESTDIR}/usr/man/cat1

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

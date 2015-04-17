#
# Public Domain.  1996/6/25 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1996/6/25
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	ctimed.c
OBJS=	ctimed.o

all: ctimed

ctimed: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

clean:
	rm -f ${OBJS} ctimed tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: ctimed
	install -s -o root -g bin -m 755 ctimed ${DESTDIR}/usr/libexec/ctimed

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

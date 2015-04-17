#
# Public Domain.  1996/10/24 - sms
#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1996/10/24
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	makekey.c
OBJS=	makekey.o

all: makekey

makekey: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

clean:
	rm -f ${OBJS} makekey tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: makekey
	install -s -o root -g bin -m 755 makekey ${DESTDIR}/usr/libexec/makekey

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

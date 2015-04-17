#
# Public Domain.  1996/10/24 - sms
#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1996/10/24
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	getNAME.c
OBJS=	getNAME.o

all: getNAME

getNAME: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

clean:
	rm -f ${OBJS} getNAME tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: getNAME
	install -s -o root -g bin -m 755 getNAME ${DESTDIR}/usr/libexec/getNAME

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

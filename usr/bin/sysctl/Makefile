#
# Public Domain.  1/15/1995 - Steven Schultz
#
#	@(#)Makefile	1.2 (2.11BSD GTE) 1996/1/12
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	sysctl.c
OBJS=	sysctl.o
MAN=	sysctl.0
MANSRC=	sysctl.8

all: sysctl sysctl.0

sysctl: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

sysctl.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} sysctl tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -s -o root -g bin -m 755 sysctl ${DESTDIR}/bin/sysctl

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

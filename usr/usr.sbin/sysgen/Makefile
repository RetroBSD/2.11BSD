#	SYSGEN -- an imitation of the CONFIG command of VMS SYSGEN.
#
#	$Id: Makefile,v 1.5 1997/06/08 12:35:38 tih Exp $

PROG=	sysgen
CFLAGS = -O
SEPFLAG= -i
SRCS=	sysgen.c
OBJS=	sysgen.o
MAN=	sysgen.0

DISTFILES = Makefile sysgen.c devices.h sysgen.1

.SUFFIXES: .0 .1

.1.0:
	/usr/man/manroff $*.1 > $@

all: sysgen ${MAN}

sysgen: sysgen.o devices.h
	cc ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

install: sysgen sysgen.0
	install -c -s ${PROG} ${DESTDIR}/usr/sbin/${PROG}
	install -m 444 ${MAN} ${DESTDIR}/usr/man/cat8/${MAN}

clean:
	rm -f ${OBJS} ${MAN} ${PROG} tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

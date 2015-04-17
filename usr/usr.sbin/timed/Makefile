#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	2.1.1	(2.11BSD)	1996/11/16
#
# optional flags are: MEASURE TESTING DEBUG

CFLAGS= -O -DMEASURE
SEPFLAG= -i
DESTDIR=
MACHINE= pdp
LIBS=
PROGRS=	timed timedc

SRCDS=	acksend.c candidate.c correct.c \
	master.c networkdelta.c readmsg.c slave.c timed.c

OBJDS=	acksend.o candidate.o correct.o \
	master.o networkdelta.o readmsg.o slave.o timed.o

SRCCS=	cmds.c cmdtab.c timedc.c

OBJCS=	cmds.o cmdtab.o timedc.o

COMMS=	byteorder.c measure.c ${MACHINE}/cksum.c

COMMO=	byteorder.o measure.o cksum.o

all:	${PROGRS}

#cksum.o:
#	cc ${CFLAGS} -c ${MACHINE}/$*.c
cksum.o:
	cc ${CFLAGS} -S ${MACHINE}/cksum.c
	ed - < ${MACHINE}/cksum-fix > /dev/null cksum.s
	as - -o $@ cksum.s
	rm -f cksum.s

timed: 	${OBJDS} ${COMMO}
	cc ${SEPFLAG} ${CFLAGS} ${OBJDS} ${COMMO} ${LIBS} -o timed 

timedc: ${OBJCS} ${COMMO}
	cc ${SEPFLAG} ${CFLAGS} ${OBJCS} ${COMMO} ${LIBS} -o timedc

install: ${PROGRS}
	install -s timed ${DESTDIR}/usr/sbin
	install -s -o root -m 4511 timedc ${DESTDIR}/usr/sbin

clean:
	rm -f a.out core *.o Makefile.bak errs ${PROGRS}

lint:
	lint ${SRCDS} ${COMMS}
	lint ${SRCCS} ${COMMS}

depend: ${SRCCS} ${SRCDS} ${COMMS}
	mkdep ${CFLAGS} ${SRCCS} ${SRCDS} ${COMMS}

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

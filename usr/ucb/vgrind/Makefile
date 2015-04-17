#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.1 (2.11BSD) 1996/10/23
#
DESTDIR=
CFLAGS=-O 
SEPFLAG= -i
SOURCES=vfontedpr.c vgrindefs.c regexp.c retest.c
CMDS=vfontedpr retest
OBJS=retest.o regexp.o vfontedpr.o vgrindefs.o

all:	$(CMDS) ${OBJS}

vfontedpr: vfontedpr.o vgrindefs.o regexp.o
	cc ${SEPFLAG} ${CFLAGS} -o vfontedpr vfontedpr.o vgrindefs.o regexp.o
retest: retest.o regexp.o
	cc ${SEPFLAG} ${CFLAGS} -o retest retest.c regexp.o

install: all
	install -s vfontedpr ${DESTDIR}/usr/libexec/vfontedpr
	install -c vgrind.sh ${DESTDIR}/usr/ucb/vgrind
	install -c -m 444 vgrindefs.src ${DESTDIR}/usr/share/misc/vgrindefs
	install -c -m 444 tmac.vgrind ${DESTDIR}/usr/share/tmac/tmac.vgrind

clean:
	rm -f ${CMDS} ${OBJS}

vprintall:
	csh /usr/ucb/vprint ${SOURCES}

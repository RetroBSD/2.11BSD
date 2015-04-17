#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.3.2 (2.11BSD) 1996/11/17
#
# makefile for line printer spooling system
#
# Ownerships {see note at the top of lpd.c}
# ROOT		a user that can access any and all files
# DAEMON	someone special
# SPGRP		the group id of the spooling programs
#
CFLAGS=-O
SEPFLAG=-i
SBINDIR=/usr/sbin
BINDIR=/usr/bin
SPOOLDIR=/usr/spool/lpd
CXREF=/usr/ucb/ctags -x
ROOT=root
DAEMON=daemon
SPGRP=daemon
DESTDIR=
SRCS=	lpd.c lpr.c lpq.c lprm.c pac.c lpd.c cmds.c cmdtab.c \
	printjob.c recvjob.c displayq.c rmjob.c \
	startdaemon.c common.c printcap.c lpdchar.c
ALL=	lpd lpr lpq lprm lpc lptest pac

all:	${ALL} FILTERS

lpd:	lpd.o printjob.o recvjob.o displayq.o rmjob.o
lpd:	startdaemon.o lpdchar.o common.o printcap.o
	${CC} ${SEPFLAG} -o lpd lpd.o printjob.o recvjob.o displayq.o rmjob.o \
		startdaemon.o lpdchar.o common.o printcap.o

lpr:	lpr.o startdaemon.o printcap.o
	${CC} ${SEPFLAG} -o lpr lpr.o startdaemon.o printcap.o

lpq:	lpq.o displayq.o common.o printcap.o
	${CC} ${SEPFLAG} -o lpq lpq.o displayq.o common.o printcap.o -ltermcap

lprm:	lprm.o rmjob.o startdaemon.o common.o printcap.o
	${CC} ${SEPFLAG} -o lprm lprm.o rmjob.o startdaemon.o common.o \
		printcap.o

lpc:	lpc.o cmds.o cmdtab.o startdaemon.o common.o printcap.o
	${CC} ${SEPFLAG} -o lpc lpc.o cmds.o cmdtab.o startdaemon.o common.o \
		printcap.o

lptest:	lptest.c
	${CC} ${CFLAGS} ${SEPFLAG} -o lptest lptest.c

pac:	pac.o printcap.o
	${CC} ${SEPFLAG} -o pac pac.o printcap.o

lpd.o lpr.o lpq.o lprm.o pac.o: lp.h lp.local.h
recvjob.o printjob.o displayq.o rmjob.o common.o: lp.h lp.local.h
startdaemon.o: lp.local.h
lpc.o cmdtab.o: lpc.h
cmds.o: lp.h lp.local.h

FILTERS:
	cd filters; make ${MFLAGS}

install: all
	install -s -o ${ROOT} -g ${SPGRP} -m 6711 lpd ${DESTDIR}/${SBINDIR}/lpd
	install -s -o ${ROOT} -g ${SPGRP} -m 6711 lpr ${DESTDIR}/${BINDIR}/lpr
	install -s -o ${ROOT} -g ${SPGRP} -m 6711 lpq ${DESTDIR}/${BINDIR}/lpq
	install -s -o ${ROOT} -g ${SPGRP} -m 6711 lprm ${DESTDIR}/${BINDIR}/lprm
	install -s -g ${SPGRP} -m 2711 lpc ${DESTDIR}/${SBINDIR}/lpc
	install -s lptest ${DESTDIR}/${SBINDIR}/lptest
	install -s pac ${DESTDIR}/${SBINDIR}/pac
	chown ${DAEMON} ${DESTDIR}/${SPOOLDIR}
	chgrp ${SPGRP} ${DESTDIR}/${SPOOLDIR}
	chmod 775 ${DESTDIR}/${SPOOLDIR}
	cd filters; make ${MFLAGS} DESTDIR=${DESTDIR} install

clean:
	rm -f ${ALL} *.o
	cd filters; make ${MFLAGS} clean

print:
	@pr makefile
	@${CXREF} *.c | pr -h XREF
	@pr *.h *.c

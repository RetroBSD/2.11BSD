# sccsid = "@(#)Makefile	1.5 (2.11BSD GTE) 1996/11/16"
#
#	dump.h			header file
#	dumpitime.c		reads /etc/ddate
#	dumpmain.c		driver
#	dumpoptr.c		operator interface
#	dumptape.c		handles the mag tape and opening/closing
#	dumptraverse.c		traverses the file system
#	unctime.c		undo ctime
#
#	dumpdir.c		program to output tape directories
#
#	DEBUG			use local directory to find ddate and dumpdates
#	TDEBUG			trace out the process forking

CFLAGS=	-O -I.
SEPFLAG=-i

HDRS=	dump.h
SRCS=	dumpitime.c dumpmain.c dumpoptr.c dumptape.c dumptraverse.c unctime.c \
	dumprmt.c
OBJS=	dumpitime.o dumpmain.o dumpoptr.o dumptape.o dumptraverse.o unctime.o
ROBJS=	dumpitime.o dumprmain.o dumpoptr.o dumprtape.o dumptraverse.o \
	unctime.o dumprmt.o

all: dump rdump dumpdir

dump: ${OBJS}
	cc ${CFLAGS} ${SEPFLAG} ${OBJS} -o dump

rdump: ${ROBJS}
	cc ${CFLAGS} ${SEPFLAG} ${ROBJS} -o rdump

dumpdir: dumpdir.c
	cc ${CFLAGS} ${SEPFLAG} dumpdir.c -o dumpdir

install: all
	install -s dump ${DESTDIR}/sbin/dump
	install -s rdump ${DESTDIR}/sbin/rdump
	install -s dumpdir ${DESTDIR}/sbin/dumpdir

clean:
	rm -f *.o dump rdump dumpdir

lint:
	lint -chabx ${CFLAGS} -DRDUMP ${SRCS}

${OBJS}: dump.h

${ROBJS}: dump.h

dumprmain.o: dumpmain.c
	cc -c -DRDUMP ${CFLAGS} dumpmain.c
	mv dumpmain.o dumprmain.o

dumprtape.o: dumptape.c
	cc -c -DRDUMP ${CFLAGS} dumptape.c
	mv dumptape.o dumprtape.o

# sccsid = "@(#)Makefile	1.3.1 (2.11BSD) 1996/11/17"
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
SRCS=	dumpitime.c dumpmain.c dumpoptr.c dumptape.c dumptraverse.c unctime.c
OBJS=	dumpitime.o dumpmain.o dumpoptr.o dumptape.o dumptraverse.o unctime.o

all: dump dumpdir

dump: ${OBJS}
	cc ${CFLAGS} ${SEPFLAG} ${OBJS} -o dump

dumpdir: dumpdir.c
	cc ${CFLAGS} ${SEPFLAG} dumpdir.c -o dumpdir

install:
	install -s dump ${DESTDIR}/sbin/dump
	install -s dumpdir ${DESTDIR}/sbin/dumpdir

clean:
	rm -f *.o dump dumpdir

lint:
	lint -chapbx ${CFLAGS} ${SRCS}

${OBJS}: dump.h

#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.5.1 (2.11BSD) 1996/10/26";
#
#	Makefile for Xerox Routing Information Protocol Daemon
#
ALL=	XNSrouted
OBJS=	XNSrouted.o
CFLAGS=  -O
SEPFLAG= -i

all:	${ALL}

XNSrouted: ${OBJS}
	${CC} ${SEPFLAG} -o XNSrouted ${OBJS}

install: XNSrouted
	install -s XNSrouted ${DESTDIR}/sbin

clean:
	rm -f XNSrouted *.o core

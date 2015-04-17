# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	@(#)Makefile	5.8.1 (2.11BSD) 1996/12/1
#
#
CFLAGS=	-O -i
SRCS=	host.c inet.c if.c main.c mbuf.c route.c unix.c ns.c
OBJS=	host.o inet.o if.o main.o mbuf.o route.o unix.o ns.o

all: netstat

netstat: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS}

clean: FRC
	rm -f ${OBJS} core netstat

depend: FRC
	mkdep ${CFLAGS} ${SRCS}

install: netstat
	install -s -g kmem -m 2755 netstat ${DESTDIR}/usr/ucb/netstat

lint: FRC
	lint ${CFLAGS} ${SRCS}

tags: FRC
	ctags ${SRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

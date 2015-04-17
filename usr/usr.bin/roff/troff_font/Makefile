#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2.1	(2.11BSD)	1996/12/1
#
CFLAGS=	-O
SRCS=	mkfont.c
OBJS=	mkfont.o

all: mkfont

mkfont: ${OBJS}
	${CC} -o $@ ${CFLAGS} ${OBJS}

clean: FRC
	rm -f ${OBJS} core mkfont

depend: FRC
	mkdep ${CFLAGS} ${SRCS}

install: FRC

lint: FRC
	lint ${CFLAGS} ${SRCS}

tags: FRC
	ctags ${SRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.


#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	4.4.1	(2.11BSD)	1996/12/1
#

# Defined SHARED_DATA to be whatever will make your compiler make initialized
# variables shared and read only.  Don't do this on a PDP-11 if you're also
# compiling separate I&D since doing so puts the shared data in text space
# which can't be accessed normally.
#
#SHARED_DATA=-R

# Add -DVMUNIX to CFLAGS to generate a version of nroff with bigger
# table sizes
#
CFLAGS=	-O -DNROFF -I${COMMON}
SEPFLAG=-i

COMMON=	../common_source
VPATH=	${COMMON}
SRCS=	ni.c nii.c n1.c n2.c n3.c n4.c n5.c n6.c n7.c n8.c n9.c n10.c \
	hytab.c ntab.c suftab.c
OBJS=	ni.o nii.o n1.o n2.o n3.o n4.o n5.o n6.o n7.o n8.o n9.o n10.o \
	hytab.o ntab.o suftab.o

all: nroff

nroff: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

hytab.o: ${COMMON}/hytab.c ${COMMON}/textscript
	${CC} ${SHARED_DATA} -c ${COMMON}/hytab.c

suftab.o: ${COMMON}/suftab.c ${COMMON}/textscript
	${CC} ${SHARED_DATA} -c ${COMMON}/suftab.c

ntab.o: ntab.c ${COMMON}/textscript
	${CC} ${SHARED_DATA} -c ntab.c

clean: FRC
	rm -f ${OBJS} core nroff

depend: ${SRCS} FRC
	mkdep ${CFLAGS} ${SRCS}

install: nroff
	install -s -o bin -g bin -m 751 nroff ${DESTDIR}/usr/bin/nroff

lint: ${SRCS} FRC
	lint ${CFLAGS} ${SRCS}

tags: ${SRCS} FRC
	ctags ${SRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.


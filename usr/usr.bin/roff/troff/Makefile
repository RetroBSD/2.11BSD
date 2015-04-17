#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.1	(2.11BSD)	1996/12/1
#

# Defined SHARED_DATA to be whatever will make your compiler make initialized
# variables shared and read only.  Don't do this on a PDP-11 if you're also
# compiling separate I&D since doing so puts the shared data in text space
# which can't be accessed normally.
#
#SHARED_DATA=-R

# Add -DVMUNIX to CFLAGS to generate a version of troff with bigger
# table sizes
#
CFLAGS=	-O -I${COMMON}
SEPFLAG=-i

COMMON=	../common_source
VPATH=	${COMMON}
SRCS=	ni.c nii.c n1.c n2.c n3.c n4.c n5.c t6.c n7.c n8.c n9.c t10.c \
	hytab.c suftab.c tab3.c
OBJS=	ni.o nii.o n1.o n2.o n3.o n4.o n5.o t6.o n7.o n8.o n9.o t10.o \
	hytab.o suftab.o tab3.o

all: troff

troff: ${OBJS} ${LIBC}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

hytab.o: ${COMMON}/hytab.c ${COMMON}/textscript
	${CC} ${SHARED_DATA} -c ${COMMON}/hytab.c

suftab.o: ${COMMON}/suftab.c ${COMMON}/textscript
	${CC} ${SHARED_DATA} -c ${COMMON}/suftab.c

clean: FRC
	rm -f ${OBJS} core troff

depend: ${SRCS} FRC
	mkdep ${CFLAGS} ${SRCS}

install: troff
	install -s -o bin -g bin -m 751 troff ${DESTDIR}/usr/bin/troff

lint: ${SRCS} FRC
	lint ${CFLAGS} ${SRCS}

tags: ${SRCS} FRC
	ctags ${SRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.


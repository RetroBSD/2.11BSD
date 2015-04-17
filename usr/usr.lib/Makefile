#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.12.3 (2.11BSD GTE) 1996/12/13
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i
TAGSFILE=tags

# Subdirectories whose routines are included in the making of the
# master tags file (/usr/lib/tags); the Fortran libraries should
# be on this list.
#
TAGSDIR=libcurses libdbm libln libm libmp libpc libtermlib libutil libvmf \
	liberrlst

# Programs that live in subdirectories, and have makefiles of their own.
#
# 2.11BSD XXX - Libpc isn't important till someone - if ever - ports
# the 4.3BSD version of ucb/pascal.
#
SUBDIR=	lib2648 libF77 libI77 libU77 libcurses libdbm libln \
	libom libmp libplot libtermlib liby libutil libvmf \
	liberrlst libident libstubs

# Shell scripts that need only be installed and are never removed.
#

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} SEPFLAG=${SEPFLAG}

FRC:

install: FRC
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	rm -f ${DESTDIR}/usr/lib/libm.a
	ln ${DESTDIR}/usr/lib/libom.a ${DESTDIR}/usr/lib/libm.a

tags:
	-for i in ${TAGSDIR}; do \
		(cd $$i; make ${MFLAGS} TAGSFILE=../${TAGSFILE} tags); \
	done

clean:
	rm -f a.out core *.s *.o tags
	-for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

depend:

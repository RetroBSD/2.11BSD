#
# Copyright (c) 1986 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	4.15	(2.11BSD)	96/10/21
#
# This makefile is designed to be run as:
#	make build
#	make installsrc
# The `make build' will compile and install the libraries
# before building the rest of the sources. The `make installsrc'
# will then install the remaining binaries.
# 
# It can also be run in the more conventional way:
#	make
#	make install
# The `make' will compile everything without installing anything.
# The `make install' will then install everything. Note however
# that all the binaries will have been loaded with the old libraries.
#
# C library options: passed to libc makefile.
# See lib/libc/Makefile for explanation.
# NOTE: The method of hostname lookup (hosts file or nameserver) is no
#	longer selected here.  Make sure to edit lib/libc/Makefile to set
#	HOSTLOOKUP
# DFLMON must be either mon.o or gmon.o.
# DEFS may include -DLIBC_SCCS, -DSYSLIBC_SCCS, both, or neither.
#
DFLMON=mon.o
DEFS= 
LIBCDEFS= DFLMON=${DFLMON} DEFS="${DEFS}"

# global flags
# SRC_MFLAGS are used on makes in command source directories,
# but not in library or compiler directories that will be installed
# for use in compiling everything else.
#
DESTDIR=
CFLAGS=	-O
SRC_MFLAGS = -k

# Programs that live in subdirectories, and have makefiles of their own.
#
# 'share' has to be towards the front of the list because programs such as
# lint(1) need their data files, etc installed first.

LIBDIR= lib usr.lib
SRCDIR=	share bin sbin etc games libexec local new ucb usr.bin usr.sbin man

all:	${LIBDIR} ${SRCDIR}

lib:	FRC
	cd lib/libc; make ${MFLAGS} ${LIBCDEFS}
	cd lib; make ${MFLAGS} ccom cpp c2

usr.lib ${SRCDIR}: FRC
	cd $@; make ${MFLAGS} ${SRC_MFLAGS}

build: buildlib ${SRCDIR}

buildlib: FRC
	@echo installing /usr/include
	# cd include; make ${MFLAGS} DESTDIR=${DESTDIR} install
	@echo
	@echo compiling libc.a
	cd lib/libc; make ${MFLAGS} ${LIBCDEFS}
	@echo installing /lib/libc.a
	cd lib/libc; make ${MFLAGS} DESTDIR=${DESTDIR} install
	@echo
	@echo compiling C compiler
	cd lib; make ${MFLAGS} ccom cpp c2
	@echo installing C compiler
	cd lib/ccom; make ${MFLAGS} DESTDIR=${DESTDIR} install
	cd lib/cpp; make ${MFLAGS} DESTDIR=${DESTDIR} install
	cd lib/c2; make ${MFLAGS} DESTDIR=${DESTDIR} install
	cd lib; make ${MFLAGS} clean
	@echo
	@echo re-compiling libc.a
	cd lib/libc; make ${MFLAGS} ${LIBCDEFS}
	@echo re-installing /lib/libc.a
	cd lib/libc; make ${MFLAGS} DESTDIR=${DESTDIR} install
	@echo
	@echo re-compiling C compiler
	cd lib; make ${MFLAGS} ccom cpp c2
	@echo re-installing C compiler
	cd lib/ccom; make ${MFLAGS} DESTDIR=${DESTDIR} install
	cd lib/cpp; make ${MFLAGS} DESTDIR=${DESTDIR} install
	cd lib/c2; make ${MFLAGS} DESTDIR=${DESTDIR} install
	@echo
	@echo compiling usr.lib
	cd usr.lib; make ${MFLAGS} ${SRC_MFLAGS}
	@echo installing /usr/lib
	cd usr.lib; make ${MFLAGS} ${SRC_MFLAGS} DESTDIR=${DESTDIR} install

FRC:

install:
	-for i in ${LIBDIR} ${SRCDIR}; do \
		(cd $$i; \
		make ${MFLAGS} ${SRC_MFLAGS} DESTDIR=${DESTDIR} install); \
	done

installsrc:
	-for i in ${SRCDIR}; do \
		(cd $$i; \
		make ${MFLAGS} ${SRC_MFLAGS} DESTDIR=${DESTDIR} install); \
	done

tags:
	for i in include lib usr.lib; do \
		(cd $$i; make ${MFLAGS} TAGSFILE=../tags tags); \
	done
	sort -u +0 -1 -o tags tags

clean:
	rm -f a.out core *.s *.o
	for i in ${LIBDIR} ${SRCDIR}; do (cd $$i; make -k ${MFLAGS} clean); done

#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	4.19.2 (2.11BSD) 1997/8/28
#
# The system has two monitoring schemes, utilizing either ``prof''
# or ``gprof''. DFLMON determines the default profiler; mon.o
# specifies that ``prof'' is the default, gmon.o specifies that
# ``gprof'' is the default.
#
# Machine dependent routines are located in a subtree which parallels
# the top directories.  This subtree is identified by the machine name.
#
# Compatibility routines are kept in directories with a prefixing
# ``compat'' (so they all sort together).
# 
# The C run-time startup code is always machine dependent and expected
# to be located in ${MACHINE}/csu
#
# All files contain sccsid strings, but these are not compiled into
# library objects by default, as a space-saving measure.  To produce
# a library that contains these strings in every object except
# system call stubs, add -DLIBC_SCCS to DEFS below; to put these
# strings into system call stubs, use -DSYSLIBC_SCCS.
#
# To compile a non-floating point versions of some standard library
# routines add -DNONFP.  This will speed up some operations if you don't
# have hardware floating point.  To compile a non-separate I&D version add
# -DNONSEPARATE.
#
# There are two possible methods of doing host name look up.  They are
# to use the name server or /etc/hosts. HOSTLOOKUP defines which method
# is to be compiled into libc  :
#								     
#	defining HOSTLOOKUP to be "named", compiles the         
#	host lookup routines that use the BIND name server.
#
#	defining HOSTLOOKUP to be "hosttable", compiles
#	the host lookup routines that use /etc/hosts.
#
HOSTLOOKUP=	named
#HOSTLOOKUP=	hosttable
#							
DESTDIR=
INSTALL=install -m 644
DFLMON=	mon.o
MACHINE=pdp
DEFS=
COMPAT=	compat-4.1 compat-sys5 compat-43
ALL=	gen inet net ns stdio stdlib string ${MACHINE} ${COMPAT}
TAGSFILE=tags

libc.a libc_p.a: ${MACHINE}/csu ${ALL} net/${HOSTLOOKUP}
	rm -rf tmp
	mkdir tmp
	cd tmp; \
		for i in ${ALL}; do ar x ../$$i/$${i}lib; done; \
		ar x ../net/${HOSTLOOKUP}/hostlib; \
		ar cr ../libc.a `lorder *.o | tsort` \
			../${MACHINE}/csu/${DFLMON}
	ar ma findiop.o libc.a exit.o
	ar ma exit.o libc.a fakcu.o
	ar ma fltpr.o libc.a doprnt.o
	ar ma doprnt.o libc.a ffltpr.o
	rm -rf tmp
	mkdir tmp
	cd tmp; \
		for i in ${ALL}; do ar x ../$$i/$${i}lib_p; done; \
		ar x ../net/${HOSTLOOKUP}/hostlib_p; \
		ar cr ../libc_p.a `lorder *.o | tsort` \
			../${MACHINE}/csu/${DFLMON}
	ar ma findiop.o libc_p.a exit.o
	ar ma exit.o libc_p.a fakcu.o
	ar ma fltpr.o libc_p.a doprnt.o
	ar ma doprnt.o libc_p.a ffltpr.o
	rm -rf tmp

${MACHINE}/csu ${ALL} net/${HOSTLOOKUP}: FRC
	cd $@; make ${MFLAGS} DEFS=${DEFS}

FRC:

install: libc.a libc_p.a
	${INSTALL} libc.a ${DESTDIR}/lib/libc.a
	ranlib ${DESTDIR}/lib/libc.a
	${INSTALL} libc_p.a ${DESTDIR}/usr/lib/libc_p.a
	ranlib ${DESTDIR}/usr/lib/libc_p.a
	cd ${MACHINE}/csu; make DESTDIR=${DESTDIR} install

tags:
	for i in ${ALL}; do \
		(cd $$i; make ${MFLAGS} TAGSFILE=../${TAGSFILE} tags); \
	done
	cd net/${HOSTLOOKUP}; \
		make ${MFLAGS} TAGSFILE=../../${TAGSFILE} tags

clean:
	for i in ${MACHINE}/csu ${ALL} net/hosttable net/named; \
		do (cd $$i; make ${MFLAGS} clean); done
	rm -rf tmp

depend:
	for i in ${MACHINE}/csu ${ALL} net/hosttable net/named; \
		do (cd $$i; make ${MFLAGS} DEFS=${DEFS} depend); done

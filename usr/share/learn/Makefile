# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	4.3.1	(2.11BSD)	1996/10/24
#
LESSONS=C editor eqn files macros morefiles vi
FILES=	Linfo Xinfo
DESTDIR=

all:

install: ${LESSONS} ${FILES}
	-mkdir -p ${DESTDIR}/usr/share/learn \
		${DESTDIR}/var/log/learn \
		${DESTDIR}/usr/share/learn/bin
	chown root.staff ${DESTDIR}/usr/share/learn \
		${DESTDIR}/usr/share/learn/bin
	chmod 755 ${DESTDIR}/usr/share/learn \
		${DESTDIR}/var/log/learn \
		${DESTDIR}/usr/share/learn/bin
	@for i in ${LESSONS}; do \
		touch ${DESTDIR}/var/log/learn/$$i; \
		chmod 666 ${DESTDIR}/var/log/learn/$$i; \
	done
	@for i in ${FILES}; do \
		echo copy $$i; \
		install -c -m 644 $$i ${DESTDIR}/usr/share/learn/$$i; \
	done
# For some unknown reason the following line sometimes fails with an
# exit -115 ...
	-tar cf - ${LESSONS} | (cd ${DESTDIR}/usr/share/learn; tar xpf -)
	-chmod 555 ${DESTDIR}/usr/share/learn/*/Init
	cd ${DESTDIR}/usr/share/learn/C; cc -O -c getline.c getnum.c

clean:

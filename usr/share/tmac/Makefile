#
# Copyright (c) 1985 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.2.1 (2.11BSD) 1996/10/21
#
#
#  Makefile for -man macros
#

SRCS=	tmac.a tmac.an tmac.an.new tmac.an6n tmac.an6t tmac.ayday \
	tmac.cp tmac.imagen tmac.os tmac.r tmac.s tmac.scover \
	tmac.sdisp tmac.skeep tmac.srefs tmac.vcat

all:	${SRCS}

install:${SRCS}
	-mkdir -p ${DESTDIR}/usr/share/tmac
	-chmod a+r,a+x ${DESTDIR}/usr/share/tmac
	install -c -m 444 ${SRCS} ${DESTDIR}/usr/share/tmac

clean:

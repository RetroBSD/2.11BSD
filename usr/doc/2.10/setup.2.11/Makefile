#
# Copyright (c) 1986 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.2.1 (2.11BSD) 1995/06/12
#
PRINTER=-Plz001
LPR=	lpr -h -n -Plz001

SRC=	0.t 1.t 2.t 3.t 4.t 5.t 6.t a.t b.t

all:
	@echo pick one of troff, nroff, groff or ditroff

troff: ${SRC}
	eqn ${SRC} | tbl | troff ${PAGES} -ms > setup.out

nroff: ${SRC}
	eqn ${SRC} | tbl | nroff ${PAGES} -ms > setup.out

groff: ${SRC}
	groff -e -t -ms -Tps ${SRC} > setup.out

ditroff: ${SRC}
	dieqn ${SRC} | ditbl | ditroff ${PAGES} -ms > setup.out

clean:
	rm -f setup.out ?.t.spell

spell:	${SRC}
	@for i in ${SRC}; do \
		echo $$i; spell $$i | sort | comm -23 - spell.ok > $$i.spell; \
	done

#
# Copyright (c) 1986 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	1.5 (Berkeley) 4/16/86
#
PRINTER=-Plz001
# TROFF=	xroff -tkchanges.LN03
TROFF=	ditroff 
EQN=	dieqn 
TBL=	ditbl
LPR=	lpr -n ${PRINTER}

SRC=	0.t 1.t 2.t 3.t

all: ${SRC}
	${EQN} ${SRC} | ${TBL} | ${TROFF} -ms ${PAGES} | ${LPR}

clean:
	rm -f kchanges.out *.t.spell errs Errs

spell:	${SRC}
	@for i in ${SRC}; do \
		echo $$i; spell $$i | sort | comm -23 - spell.ok > $$i.spell; \
	done

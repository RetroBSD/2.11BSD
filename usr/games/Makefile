#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.5 (2.11BSD) 1997/7/26
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	adventure backgammon battlestar boggle btlgammon cribbage fortune \
	hack \
	hangman hunt mille monop pdp phantasia quiz robots sail snake \
	trek warp words

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	wargames

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	arithmetic banner bcd cfscores factor fish morse number ppt \
	wump

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	canfield primes rain worm worms

all:	${SUBDIR} ${STD} ${NSTD}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} SEPFLAG=${SEPFLAG}

FRC:

${STD}:
	cc ${CFLAGS} ${SEPFLAG} -o $@ $@.c

install: ${STD} ${NSTD}
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	-for i in ${SCRIPT}; do \
		(install -c -m 755  $$i.sh ${DESTDIR}/usr/games/$$i); done
	-for i in ${STD} ${NSTD}; do \
		(install -s -m 755 $$i ${DESTDIR}/usr/games/$$i); done
	cat >${DESTDIR}/usr/games/lib/cfscores </dev/null
	chmod 777 ${DESTDIR}/usr/games/lib/cfscores

clean:
	rm -f a.out core *.s *.o
	-for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD}

depend:
	for i in ${STD} ${NSTD}; do \
	    cc -M ${INCPATH} $$i.c | sed 's/\.o//' | \
	    awk ' { if ($$1 != prev) \
		{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# Files listed in ${NSTD} have explicit make lines given below.

canfield:
	cc -o canfield ${CFLAGS} ${SEPFLAG} canfield.c -lcurses -ltermcap

primes:
	cc -o primes ${CFLAGS} ${SEPFLAG} primes.c -lm

rain:
	cc -o rain ${CFLAGS} ${SEPFLAG} rain.c -lcurses -ltermcap

worm:
	cc -o worm ${CFLAGS} ${SEPFLAG} worm.c -lcurses -ltermcap

worms:
	cc -o worms ${CFLAGS} ${SEPFLAG} worms.c -lcurses -ltermcap

# DO NOT DELETE THIS LINE -- make depend uses it


#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.19.10 (2.11BSD GTE) 1997/3/27
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	adb ar as awk chflags chpass csh diff disklabel hostname ld login \
	ls make nm passwd ping sed sh stty sysctl tcsh test tp

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	false true

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	cat cc chgrp chmod cmp cp date dd du echo ed grep hostid \
	kill ln mkdir mt mv nice od pagesize pr \
	pwd rm rmail rmdir size sync tar tee time who

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	expr

# Programs that must run setuid to root
#
SETUID=	mail rcp su

# Programs that run set-group-id operator
#
OPERATOR= df

# Programs that must run set-group-id kmem.
#
KMEM=	ps

# Programs that must run set-group-id tty.
#
TTY=	wall write

# 'strip' is handled specially because 'install -s' now uses 'strip' and
# thus we can't do a 'install -s strip /bin/strip' without an error.
#
BINS=	${STD} ${NSTD} ${SETUID} ${OPERATOR} ${KMEM} ${TTY} strip

all:	${SUBDIR} ${BINS}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} SEPFLAG=${SEPFLAG}

FRC:


${STD} ${SETUID} ${KMEM} ${OPERATOR} ${TTY} strip:
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ $@.c

install: ${BINS}
	install -s -m 751 -g staff ${STD} ${NSTD} ${DESTDIR}/bin
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	-for i in ${SCRIPT}; do (install -c $$i.sh ${DESTDIR}/bin/$$i); done
	-for i in ${SETUID}; do \
		(install -o root -m 4751 -s $$i ${DESTDIR}/bin/$$i); done
	-for i in ${OPERATOR}; do \
		(install -g operator -m 2751 -s $$i ${DESTDIR}/bin/$$i); done
	-for i in ${KMEM}; do \
		(install -g kmem -m 2751 -s $$i ${DESTDIR}/bin/$$i); done
	-for i in ${TTY}; do \
		(install -g tty -m 2751 -s $$i ${DESTDIR}/bin/$$i); done
	-install -s -m 751 -g staff strip ${DESTDIR}/bin/xstrip
	-mv ${DESTDIR}/bin/xstrip ${DESTDIR}/bin/strip
	rm -f ${DESTDIR}/bin/e; ln ${DESTDIR}/bin/ed ${DESTDIR}/bin/e

clean:
	rm -f ${BINS} expr.c a.out core *.s *.o errs
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

depend: expr.c
	for i in ${BINS}; do \
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

expr: expr.o
	${CC} ${CFLAGS} ${SEPFLAG} -o expr expr.o

# DO NOT DELETE THIS LINE -- make depend uses it


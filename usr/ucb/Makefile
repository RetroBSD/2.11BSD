#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.17.4 (2.11BSD GTE) 1997/6/27
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	Mail compress dbx error ex finger fp ftp indent lock man \
	more msgs netstat pascal rdist sendbug talk tftp \
	tn3270 tset vgrind vlp window

# Shell scripts that need only be installed and are never removed.
#
CSHSCRIPT= which

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	apply biff checknr colcrt colrm ctags expand fold \
	from gprof grep head last lastcomm leave logger mkstr \
	printenv ruptime rwho sccs script soelim strings strcompact \
	symcompact symdump symorder tail tcopy telnet unexpand unifdef users \
	whois what wc xstr yes

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	clear ul vacation

# Programs that must run setuid to root
#
SETUID=	quota rlogin rsh

# Programs that must run set-group-id kmem.
#
KMEM=	gcore vmstat w

all:	${SUBDIR} ${STD} ${NSTD} ${KMEM} ${SETUID}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} SEPFLAG=${SEPFLAG}

${STD} ${KMEM} ${SETUID}:
	cc ${CFLAGS} ${SEPFLAG} -o $@ $@.c

install: ${STD} ${NSTD} ${KMEM} ${SETUID}
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	-for i in ${CSHSCRIPT}; do \
		(install -m 755 -c $$i.csh ${DESTDIR}/usr/ucb/$$i); done
	-for i in ${STD} ${NSTD}; do \
		(install -s $$i ${DESTDIR}/usr/ucb/$$i); done
	-for i in ${KMEM}; do \
		(install -g kmem -m 2755 -s $$i ${DESTDIR}/usr/ucb/$$i); done
	-for i in ${SETUID}; do \
		(install -o root -m 4755 -s $$i ${DESTDIR}/usr/ucb/$$i); done
	rm -f ${DESTDIR}/usr/ucb/uptime ${DESTDIR}/usr/ucb/f
	ln ${DESTDIR}/usr/ucb/w ${DESTDIR}/usr/ucb/uptime
	ln ${DESTDIR}/usr/ucb/finger ${DESTDIR}/usr/ucb/f
	rm -f ${DESTDIR}/usr/ucb/u
	ln ${DESTDIR}/usr/ucb/users ${DESTDIR}/usr/ucb/u

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD} ${SETUID} ${KMEM}

FRC:

depend:
	for i in ${STD} ${NSTD} ${SETUID} ${KMEM}; do \
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

clear:
	${CC} -o clear ${SEPFLAG} ${CFLAGS} clear.c -ltermlib

ul:
	${CC} -o ul ${SEPFLAG} ${CFLAGS} ul.c -ltermlib

vacation: vacation.c
	${CC} -o vacation ${SEPFLAG} ${CFLAGS} vacation.c -ldbm

# DO NOT DELETE THIS LINE -- make depend uses it


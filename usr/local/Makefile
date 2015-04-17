#
# 	#(@)Makefile	1.2.2	(2.11BSD) 1996/11/27
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR= afio cxref ddd less mkovmake mp mtools welcome qterm zmodem

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD= decompr16 trace 

# C programs that live in the current directory and need explicit make lines.
#
NSTD=

# Shell scripts
#
SHELL= none

# Manpage directory
MANDIR= ${DESTDIR}/usr/local/man

all:	${SUBDIR} ${STD} ${NSTD}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} SEPFLAG=${SEPFLAG}

FRC:

${STD}:
	cc ${CFLAGS} ${SEPFLAG} -o $@ $@.c

# Files listed in ${NSTD} have explicit make lines given below.

install: ${STD} ${NSTD}
	-mkdir ${MANDIR}
	-chmod a+r ${MANDIR}
	-for i in 1 2 3 4 5 6 7 8; do \
		(mkdir ${MANDIR}/cat$$i; chmod a+r ${MANDIR}/cat$$i); done
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	-for i in ${STD} ${NSTD}; do \
		(install $$i ${DESTDIR}/usr/local/$$i); done
	-chgrp kmem ${DESTDIR}/usr/local/trace
	-chmod 2755 ${DESTDIR}/usr/local/trace
#	-for i in ${SHELL}; do \
#		(install -c -m 755 $$i.sh ${DESTDIR}/usr/local/$$i); done
	-sh /usr/sbin/makewhatis ${MANDIR}

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD}

depend:
	cat </dev/null >x.c
	for i in ${STD} ${NSTD}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above

#	Makefile	1.9 (2.11BSD GTE) 1997/7/26
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	at calendar dc diction diff3 eqn fstat f77 find graph id learn \
	lex lint \
	m4 neqn plot printf prof ranlib ratfor refer roff spell struct tbl \
	mkerrlst patch \
	renice shar tip uucp uname whereis yacc xargs xinstall

# Shell scripts that need only be installed and are never removed.
#
SCRIPT=	lorder mkdep nohup

# C source that lives in the current directory
#
SRCS=	basename.c cal.c cb.c checkeq.c col.c comm.c crypt.c deroff.c \
	fgrep.c file.c iostat.c join.c look.c mesg.c ptx.c \
	rev.c sleep.c sort.c spline.c split.c sum.c tabs.c \
	tc.c tk.c touch.c tr.c tsort.c tty.c uniq.c units.c

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	basename cal cb checkeq col comm crypt deroff fgrep file join look \
	mesg ptx rev sleep sort spline split sum tabs tc tk touch tr \
	tsort tty uniq units

# C programs that live in the current directory and need explicit make lines.
# (make depend has special rules for these files)
#
NSTD=	bc egrep

# Programs that must run set-group-id kmem.
#
KMEM=	iostat

all: ${SUBDIR} ${STD} ${NSTD} ${KMEM}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} SEPFLAG=${SEPFLAG}

FRC:

${STD} ${KMEM}:
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ $@.c

install: ${STD} ${NSTD} ${KMEM}
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
	-for i in ${SCRIPT}; do \
		(install -c -m 555 $$i.sh ${DESTDIR}/usr/bin/$$i); done
	-for i in ${KMEM}; do \
		(install -s -g kmem -m 2755 $$i ${DESTDIR}/usr/bin/$$i); done
	install -s ${STD} ${NSTD} ${DESTDIR}/usr/bin

clean:
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f a.out core *.s *.o y.tab.c ${STD} ${NSTD} ${KMEM}

depend:
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} depend); done
	mkdep -p ${CFLAGS} ${SRCS}

FRC:

# Files listed in ${NSTD} have explicit make lines given below.

bc egrep:
	${YACC} ${YFLAGS} $@.y
	${CC} -c ${CFLAGS} y.tab.c
	mv y.tab.o $@.o
	${CC} ${SEPFLAG} -o $@ $@.o
	rm -f $@.o

# DO NOT DELETE THIS LINE -- make depend uses it


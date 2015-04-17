#
#	@(#)Makefile	1.15	(2.11BSD GTE)	1996/10/23
#
M=/usr/src/lib/mip
P=/usr/src/lib/pcc

XSTR=	/usr/ucb/xstr
MKSTR=	/usr/ucb/mkstr

# turn off the debugging to save string space
STRINGS=\"/usr/share/misc/lintstrings\"
CFLAGS=-O -I$M -I. -DBUG1 -DBUG2 -DBUG3 -DBUG4 -DSTRINGFILE=${STRINGS}
LFLAGS=-i

SRCS=	cgram.y common.c optim.c pftn.c scan.c trees.c \
	xdefs.c lint.c hash.c pass1.h manifest.h macdefs.h \
	lpass2.c

.c.o:
	${MKSTR} - lintstrings XX $*.c
	${CC} -E ${CFLAGS} XX$*.c | ${XSTR} -c -
	${CC} -c ${CFLAGS} x.c
	mv -f x.o $*.o
	rm -f x.c XX$*.c

all:	links lpass1 lpass2

# Have to do this so that 'mkstr' can prepend a string to the filenames.
# If the VPATH capability is used the file names come out looking like
# "XX/usr/src/lib/mip/optim.c" which, obviously, does not work.

links:
	rm -f cgram.y common.c optim.c pftn.c scan.c trees.c pass1.h
	rm -f manifest.h xdefs.c comm1.c
	ln -s $M/cgram.y cgram.y
	ln -s $M/common.c common.c
	ln -s $M/optim.c optim.c
	ln -s $M/pftn.c pftn.c
	ln -s $M/scan.c scan.c
	ln -s $M/trees.c trees.c
	ln -s $M/xdefs.c xdefs.c
	ln -s $M/pass1.h pass1.h
	ln -s $M/manifest.h manifest.h

lpass1: cgram.o comm1.o optim.o pftn.o scan.o trees.o xdefs.o \
	lint.o hash.o strings.o
	${CC} ${LFLAGS} ${CFLAGS} cgram.o comm1.o optim.o pftn.o \
		scan.o trees.o xdefs.o lint.o hash.o strings.o -o lpass1

cgram.o: manifest.h pass1.h pcclocal.h macdefs.h cgram.c

strings.o: strings
	${XSTR}
	${CC} -c xs.c
	mv -f xs.o strings.o
	rm -f xs.c

comm1.o: manifest.h pass1.h pcclocal.h common.c macdefs.h
	rm -f comm1.c
	ln common.c comm1.c
	${MKSTR} - lintstrings XX comm1.c
	${CC} -E ${CFLAGS} -DPASS1COMMON XXcomm1.c | ${XSTR} -c -
	${CC} -c ${CFLAGS} -DPASS1COMMON x.c
	mv -f x.o comm1.o
	rm -f x.c comm1.c XXcomm1.c

optim.o: manifest.h macdefs.h pass1.h pcclocal.h optim.c

pftn.o: manifest.h macdefs.h pass1.h pcclocal.h pftn.c

scan.o: manifest.h macdefs.h pass1.h pcclocal.h scan.c

trees.o: manifest.h macdefs.h pass1.h pcclocal.h trees.c

xdefs.o: manifest.h pass1.h pcclocal.h macdefs.h xdefs.c

cgram.c: cgram.y pcctokens
	cat pcctokens cgram.y > gram.in
	$(YACC) gram.in
	fgrep -v "static char yaccpar_sccsid" y.tab.c >cgram.c
	rm -f y.tab.c

GREP=	egrep

pcclocal.h: $P/localdefs.h /usr/include/pcc.h
	rm -f pcclocal.h
	cat /usr/include/pcc.h $P/localdefs.h | $(GREP) '^#[ 	]*(define[ 	][ 	]*PCC(F|T|TM|OM)?_|ifdef|ifndef|endif)' | sed -e 's/PCC[A-Z]*_//' > pcclocal.h 

pcctokens: $P/localdefs.h /usr/include/pcc.h
	rm -f pcctokens
	cat /usr/include/pcc.h $P/localdefs.h | $(GREP) '^#[ 	]*define[ 	][ 	]*PCC_' | sed -e 's/^#[ 	]*define[ 	][ 	]*PCC_/%term	/' > pcctokens

lint.o: manifest.h macdefs.h pass1.h lmanifest.h

lpass2: lpass2.o hash.o
	${CC} ${LFLAGS} ${CFLAGS} lpass2.o hash.o -o lpass2

lpass2.o: manifest.h lmanifest.h pcclocal.h macdefs.h
	${CC} ${CFLAGS} -c -I$M -I. lpass2.c

hash.o: hash.c
	${CC} ${CFLAGS} -c -I$M -I. hash.c

lintall:
	lint -hpav -I. -I$M  -DPASS1COMMON cgram.c xdefs.c scan.c \
		common.c pftn.c trees.c optim.c hash.c lint.c

install: all SHELL
	-mkdir ${DESTDIR}/usr/libexec/lint
	chmod 755 ${DESTDIR}/usr/libexec/lint
	install -c -m 755 -s lpass1 ${DESTDIR}/usr/libexec/lint/lint1
	install -c -m 755 -s lpass2 ${DESTDIR}/usr/libexec/lint/lint2
	install -c -m 444 lintstrings ${DESTDIR}/usr/share/misc/lintstrings
	./libs
	-rm -f ${DESTDIR}/usr/share/lint/llib-ltermlib*
	ln -s llib-ltermcap ${DESTDIR}/usr/share/lint/llib-ltermlib
	ln -s llib-ltermcap.ln ${DESTDIR}/usr/share/lint/llib-ltermlib.ln
	install -c SHELL ${DESTDIR}/usr/bin/lint

clean:
	rm -f *.o errs pcctokens pcclocal.h x.c xs.c strings lintstrings
	rm -f lpass1 lpass2 cgram.c gram.in tags yacc.* y.tab.c XX*
	rm -f cgram.y common.c optim.c pftn.c scan.c trees.c pass1.h
	rm -f manifest.h xdefs.c comm1.c

tags:	${SRCS}
	ctags -w ${SRCS}

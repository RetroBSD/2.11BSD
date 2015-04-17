# Makefile for tn3270 and friends...
# @(#)Makefile	2.12	1996/11/16

CC	= cc
SEPFLAG	= -i

DEFINES = -DDEBUG
DEFINES =

INCLUDES = -I.
INCLUDES =

OPTIMIZE = -O

CFLAGS	= $(OPTIMIZE) $(INCLUDES) $(DEFINES)

# Lint flags
LINTFLAGS	= -hbxaz
# How to install the bloody thing...

DESTDIR=

BINDIR		= $(DESTDIR)/usr/ucb

# Names for the terminal libraries...
LIBCURSES	= -lcurses
LIBTERM		= -ltermlib

# The source files...
ALLH =	3270.h ascebc.h ectype.h options.h screen.h state.h DEFS.h
M4FILE = termcodes.m4

MOSTC = asctab.c ebctab.c options.c screen.c datastream.c keyboard.c \
	termin.c map3270.c ectype.c
TNMAIN = tn3270.c
MSMAIN = mset.c

#
#	In the vax and pdp environments, we use {vax,pdp}bsubs.s, which give
#	us a fair amount of increased performance.  We have provided
#	genbsubs.c, which perform (more or less) the same function.
#	Don't forget to specify the correct assembly rules if you use them.

#SUBS = genbsubs.c
SUBS = pdpbsubs.s
#SUBS = vaxbsubs.s

#SUBSO = genbsubs.o
SUBSO = pdpbsubs.o
#SUBSO = vaxbsubs.o

# PDP-11 assembly rules:
.s.o:
	/lib/cpp -E $*.s | sed -e 's;^#;/;' | as -V -o $@

# Vax assembly rules:
#.s.o:
#	/lib/cpp -E $< | as -o $@

ALLC = ${MOSTC} ${TNMAIN} ${MSMAIN} ${SUBS}
# ALLC = ${MOSTC} ${TNMAIN} ${MSMAIN}

ALLS =
#ALLS = vaxbsubs.s

MOSTO	= tn3270.o ebctab.o asctab.o termin.o keyboard.o \
		datastream.o screen.o map3270.o options.o \
		ectype.o $(SUBSO)

ALLO	= $(MOSTO) mset.o

all:	${ALLH} ${ALLC} ${ALLS} tn3270 mset

tn3270:	$(MOSTO)
	${CC} ${CFLAGS} ${SEPFLAG} -o tn3270 $(MOSTO) $(LIBCURSES) $(LIBTERM)

mset:	mset.o map3270.o
	${CC} ${CFLAGS} ${SEPFLAG} -o mset mset.o map3270.o $(LIBCURSES)

install: tn3270 mset
	install -s tn3270 $(BINDIR)
	install -s mset $(BINDIR)

clean:
	rm -f $(ALLO) mset tn3270 m4.out errs Makefile.bak

tags:	${ALLC} ${ALLH}
	ctags -t ${ALLC} ${ALLH}

lint:
	lint ${LINTFLAGS} ${TNMAIN} ${MOSTC} -lcurses
	lint ${LINTFLAGS} ${MSMAIN} map3270.c -lcurses

m4.out:		termcodes.m4
	rm -f m4.out
	m4 termcodes.m4 > m4.out
	chmod 444 m4.out

depend:	${ALLC}
	mkdep ${CFLAGS} ${ALLC}

# DO NOT DELETE THIS LINE

asctab.o: ascebc.h
ebctab.o: ascebc.h
options.o: options.h
screen.o: screen.h 3270.h
datastream.o: ascebc.h 3270.h screen.h
keyboard.o: m4.out ascebc.h 3270.h screen.h options.h ectype.h
termin.o: m4.out state.h
map3270.o: m4.out state.h default.map3270
ectype.o: ectype.h
mset.o: state.h m4.out

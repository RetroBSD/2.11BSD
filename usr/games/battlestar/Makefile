#
# Copyright (c) 1983 Regents of the University of California,
# All rights reserved.  Redistribution permitted subject to
# the terms of the Berkeley Software License Agreement.
#

# @(#)Makefile	1.7.1 1996/1/28

CFLAGS=		-O
SEPFLAG= -i
OBJS=		battlestar.o com1.o com2.o com3.o com4.o com5.o com6.o com7.o \
		init.o cypher.o getcom.o parse.o room.o save.o fly.o misc.o \
		globals.o dayfile.o nightfile.o dayobjs.o nightobjs.o words.o
CFILES=		battlestar.c com1.c com2.c com3.c com4.c com5.c com6.c com7.c \
		init.c cypher.c getcom.c parse.c room.c save.c fly.c misc.c \
		globals.c dayfile.c nightfile.c dayobjs.c nightobjs.c words.c
LIBS=		-lcurses -ltermlib
LINTFLAGS=	-h
JUNKFILES=	battlestar tags
DIR=		$(DESTDIR)/usr/games
LIB=		$(DESTDIR)/usr/games/lib
LOGFILE=	$(LIB)/battlestar.log
MAN=		battlestar.0

all:	battlestar ${MAN}

battlestar: $(OBJS)
	cc -o battlestar ${SEPFLAG} $(OBJS) $(LIBS)

$(OBJS): externs.h

# Uncomment the following lines on pdp-11's
STRINGFILE=	$(LIB)/battle_strings

dayfile.o: dayfile.c mkstr
	./mkstr - battle_strings xx dayfile.c
	cc -c xxdayfile.c
	mv xxdayfile.o dayfile.o
	rm -f xxdayfile.c

nightfile.o: nightfile.c mkstr
	./mkstr - battle_strings xx nightfile.c
	cc -c xxnightfile.c
	mv xxnightfile.o nightfile.o
	rm -f xxnightfile.c

globals.o: globals.c mkstr
	./mkstr - battle_strings xx globals.c
	cc -c xxglobals.c
	mv xxglobals.o globals.o
	rm -f xxglobals.c

mkstr:	mkstr.c
	cc -O -n -o mkstr mkstr.c

# End of pdp-11 specific instructions

lint: $(CFILES) 
	lint $(LINTFLAGS) $(CFILES) $(LIBS) 

install: battlestar $(MAN) $(LOGFILE) $(STRINGFILE)
	install -s battlestar $(DIR)/battlestar
	install -m 444 -o bin -g bin  ${MAN} ${DESTDIR}/usr/man/cat6

stringfile $(STRINGFILE): battle_strings
	install -c -m 644 battle_strings $(LIB)

logfile $(LOGFILE):
	cp /dev/null $(LOGFILE)
	chmod 666 $(LOGFILE)

man $(MAN): battlestar.6
	/usr/man/manroff battlestar.6 > battlestar.0

clean:
	rm -f $(OBJS) $(JUNKFILES) xx* mkstr battle_strings core a.out ${MAN}

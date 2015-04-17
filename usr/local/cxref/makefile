# makefile for cxref -- C cross referencing program
#
# Arnold Robbins, Information and Computer Science, Georgia Tech
#	gatech!arnold
# Copyright (c) 1985 by Arnold Robbins.
# All rights reserved.
# This program may not be sold, but may be distributed
# provided this header is included.

# some files are system dependant, e.g. where sort is.
# change the appropriate macro definitions and recompile.


### definitions of files to compile and load, and other targets for make

SCANOBJS= docxref.o cscan.o
SCANSRCS= docxref.c cscan.l

CXREF = cxref
INCLS= constdefs.h basename.c
PROGS= docxref fmtxref cxrfilt $(CXREF)
SRCS=  $(SCANSRCS) fmtxref.c cxrfilt.c $(CXREF).c
DOCS=  README makefile cxref.1

PRINTS= $(INCLS) $(SRCS) $(DOCS)

CFLAGS= -O
LFLAGS= -i

### system dependant definitions, change when you install cxref

BIN=${DESTDIR}/usr/local
LIB=/usr/local/lib/cxref

# lex library, may be -lln on some systems
LEXLIB= -ll

# may be /bin/sort on some systems
SORT=/usr/bin/sort

# printer program, prt is for me, use pr on other systems
P=pr

all: $(PROGS) cxref.0
	@echo "	all" done

docxref: $(SCANOBJS)
	$(CC) $(LFLAGS) $(SCANOBJS) $(LEXLIB) -o $@

cscan.o docxref.o cxrfilt.o: $(INCLS)

fmtxref: fmtxref.c
	$(CC) $(LFLAGS) $(CFLAGS) $@.c $(LDFLAGS) -o $@

cxrfilt: cxrfilt.c
	$(CC) $(LFLAGS) $(CFLAGS) $@.c $(LDFLAGS) -o $@

$(CXREF): $(CXREF).c
	$(CC) $(LFLAGS) $(CFLAGS) -DSRCDIR='"$(LIB)"' -DSORT='"$(SORT)"' $@.c $(LDFLAGS) -o $@

print:
	$(P) $(PRINTS) | lpr -b 'Cxref Source'
	touch print2

print2: $(PRINTS)
	$(P) $? | lpr -b 'Cxref New Source'
	touch print2

### edit this before installing!!

install: $(PROGS) cxref.0
# don't remove my bin!
	rm -f ${BIN}/cxref
	-mkdir $(LIB)
	install -s $(CXREF) ${BIN}/$(CXREF)
	install -s docxref  $(LIB)/docxref
	install -s fmtxref  $(LIB)/fmtxref
	install -s cxrfilt $(LIB)/cxrfilt
	install -m 444 -c cxref.0 /usr/local/man/cat1/cxref.0

cxref.0:	cxref.1
	/usr/man/manroff cxref.1 > cxref.0

clean:
	rm -f $(SCANOBJS) cxref.0

clobber: clean
	rm -f $(PROGS) print2

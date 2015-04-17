#!/bin/make -f
# pathalias -- by steve bellovin, as told to peter honeyman

### configuration section
###
# if you can't or don't intend to use dbm files,
# don't bother with DBM or makedb
DBM = -ldbm
# or if you roll your own ...
# DBM = dbm.o
###
# where is getopt (if not in the c library)?
# GETOPT = getopt.o
### end of configuration section 


CC = cc
CFLAGS = -O -DSTATIC=static
SEPFLAG = -i
LDFLAGS = $(GETOPT) $(SEPFLAG)
YFLAGS = -d

OBJ = addlink.o addnode.o getstruct.o local.o main.o mapit.o mapaux.o mem.o \
      parse.o printit.o
HDRS = def.h config.h
CSRC = addlink.c addnode.c getstruct.c local.c main.c mapit.c mapaux.c mem.c \
       printit.c
LSRC = $(CSRC) parse.c
SRC = $(CSRC) parse.y makedb.c arpatxt.c

.SUFFIXES: .1 .0

.1.0:
	/usr/man/manroff $*.1 > $*.0

all: pathalias makedb arpatxt pathalias.0

pathalias: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o pathalias

$(OBJ):	$(HDRS)

parse.c: parse.y $(HDRS)
	$(YACC) $(YFLAGS) parse.y
	mv y.tab.c parse.c

makedb: makedb.o
	$(CC) makedb.o $(LDFLAGS) $(DBM) -o makedb

makedb.o: config.h

arpatxt: arpatxt.o
	$(CC) arpatxt.o $(LDFLAGS) -o arpatxt

clean:
	rm -f *.o a.out core tags y.tab.? parse.c pathalias arpatxt makedb *.0

clobber: clean
	rm -f pathalias makedb arpatxt

tags: $(SRC) $(HDRS)
	ctags -w $(HDRS) $(SRC)

bundle:
	@bundle README CHANGES pathalias.1 Makefile ${HDRS} ${SRC}

lint:	$(LSRC)
	lint $(CFLAGS) $(LSRC)
	lint makedb.c
	lint arpatxt.c

install: pathalias makedb arpatxt pathalias.0
	install -s pathalias ${DESTDIR}/usr/new
	install -s makedb ${DESTDIR}/usr/new
	install -s arpatxt ${DESTDIR}/usr/new
	install -c -m 444 -o bin -g bin pathalias.0 ${DESTDIR}/usr/new/man/cat1

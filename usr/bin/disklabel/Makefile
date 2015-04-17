#	@(#)Makefile	1.0 (2.11BSD) 1995/04/10

DESTDIR=
CFLAGS=-O
SEPFLAG=-i
MANDIR=/usr/man/cat

all: disklabel disklabel.5.0 disklabel.0

disklabel: disklabel.o
	cc ${CFLAGS} ${SEPFLAG} -o $@ $@.o -lutil

install: all
	install -s -c -o bin -g operator -m 2550 disklabel \
			${DESTDIR}/bin/disklabel
	install -c -o bin -g bin -m 444 disklabel.5.0 \
			${DESTDIR}${MANDIR}5/disklabel.0
	install -c -o bin -g bin -m 444 disklabel.0 \
			${DESTDIR}/${MANDIR}8/disklabel.0

clean:
	rm -f *.o *.0 *.core disklabel

disklabel.0: disklabel.8
	/usr/man/manroff disklabel.8 > disklabel.0

disklabel.5.0: disklabel.5.5
	/usr/man/manroff disklabel.5.5 > disklabel.5.0

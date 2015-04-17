#
# makefile for as
#

AS=	/bin/as
SEPFLAG= -i -X

all: as

as: as0.o as2.o
	ld ${SEPFLAG} -o $@ /lib/crt0.o as0.o as2.o -lc

install: all
	install -s as ${DESTDIR}/bin

clean:
	rm -f *.o a.out as core errs

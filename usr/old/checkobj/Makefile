#
# checkobj Makefile.  Revised 1996/.1/28
#
DESTDIR=

DEFS=
CFLAGS=	-O ${DEFS}
SEPFLAG=-i

all: checkobj checkobj.0

checkobj: checkobj.o
	cc ${CFLAGS} ${SEPFLAG} checkobj.o -o checkobj

install: checkobj checkobj.0
	install -s checkobj ${DESTDIR}/usr/new/checkobj
	install -c -m 444 -o bin -g bin checkobj.0 ${DESTDIR}/usr/new/man/cat1

checkobj.0: checkobj.1
	/usr/man/manroff checkobj.1 > checkobj.0

clean:
	rm -f checkobj *.o checkobj.0 MAKELOG

depend:
tags:
lint:

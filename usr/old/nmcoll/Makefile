#
# 'nmcoll' is remnant from the era before long(er) symbol names were
# supported in object files.
#
DESTDIR=

DEFS=
CFLAGS=	-O ${DEFS}
SEPFLAG=-i

all: nmcoll.sh nmcoll.0

install: nmcoll.sh nmcoll.0
	install -c -m 755 nmcoll.sh ${DESTDIR}/usr/new/nmcoll
	install -c -m 444 -o bin -g bin nmcoll.0 ${DESTDIR}/usr/new/man/cat1

nmcoll.0: nmcoll.1
	/usr/man/manroff nmcoll.1 > nmcoll.0

clean:
	-rm -f nmcoll.0

depend:
tags:
lint:

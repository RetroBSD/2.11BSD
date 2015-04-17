#
# Public Domain.  1996/6/25 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1996/6/25
#
DEFS=
CFLAGS=-O $(DEFS)
TAGSFILE=tags

all: libstubs.a libstubs_p.a

libstubs.a: stubs.c
	${CC} -c ${CFLAGS} stubs.c
	ld -r -X -o libstubs.a stubs.o
	rm -f stubs.o

libstubs_p.a: stubs.c
	${CC} -p -c ${CFLAGS} stubs.c
	ld -r -X -o libstubs_p.a stubs.o
	rm -f stubs.o

install: all
	install -m 644 libstubs.a $(DESTDIR)/usr/lib
	install -m 644 libstubs_p.a $(DESTDIR)/usr/lib

tags:
	ctags -a -f ${TAGSFILE} `pwd`/stubs.c

clean:
	rm -f *.o libstubs.a libstubs_p.a

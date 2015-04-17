#
# shar Makefile.  Revised 1997/1/12
#
DESTDIR=

CFLAGS=	-O
SEPFLAG=-i

SRCS=	shar.c traverse.c
OBJS=	shar.o traverse.o

.SUFFIXES: .3 .1 .0

.1.0:
	/usr/man/manroff $*.1 > $*.0

.3.0:
	/usr/man/manroff $*.3 > $*.0

all: shar traverse shar.0 traverse.0

shar: ${OBJS} Makefile
	cc ${CFLAGS} ${SEPFLAG} -o shar ${OBJS}

traverse: traverse.c
	cc -o traverse -DSTANDALONE traverse.c

lint:
	lint -h ${SRCS}

install: shar shar.0
	install -s shar ${DESTDIR}/usr/bin/shar
	install -s traverse ${DESTDIR}/usr/bin/traverse
	install -c -m 444 -o bin -g bin shar.0 ${DESTDIR}/usr/man/cat1
	install -c -m 444 -o bin -g bin traverse.0 ${DESTDIR}/usr/man/cat3

tags: ${SRCS}
	ctags $<

clean:
	rm -f *.o shar traverse core tags *.0

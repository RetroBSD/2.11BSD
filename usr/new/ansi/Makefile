#	@(#)Makefile	5.3.1	(2.11BSD)	1996/1/28
#
DESTDIR=
CFLAGS=	-O
SEPFLAG=-i

SRCS=	ansitape.c vmsprep.c
MSRC=	ansitape.1 vmsprep.1
MOBJ=	ansitape.0 vmsprep.0

.SUFFIXES: .1 .0

.1.0:
	/usr/man/manroff $*.1 > $*.0

all:	ansitape vmsprep ${MOBJ}

ansitape: ansitape.o
	cc ${SEPFLAG} -o ansitape ansitape.o

vmsprep: vmsprep.o
	cc ${SEPFLAG} -o vmsprep vmsprep.o

install: vmsprep ansitape ${MOBJ}
	install -s ansitape ${DESTDIR}/usr/new/ansitape
	install -s vmsprep ${DESTDIR}/usr/new/vmsprep
	install -c -m 444 -o bin -g bin ansitape.0 ${DESTDIR}/usr/new/man/cat1
	install -c -m 444 -o bin -g bin vmsprep.0 ${DESTDIR}/usr/new/man/cat1

clean:
	rm -f *.o a.out core errs makefile.bak Makefile.bak ansitape vmsprep *.0

depend:
	mkdep ${CFLAGS} ${SRCS}

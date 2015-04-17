#	@(#)Makefile	5.8 (Berkeley) 3/10/91

CFLAGS=	-I. -O
LFLAGS=	-z

SRCS=	append.c ar.c archive.c contents.c delete.c extract.c misc.c \
	move.c print.c replace.c 

EXTRASRCS= strmode.c

OBJS=	append.o ar.o archive.o contents.o delete.o extract.o misc.o \
	move.o print.o replace.o 

EXTRAOBJ= strmode.o # strftime.o ctime.o

all: ar ar.1.0 ar.5.0

ar: ${OBJS} ${EXTRAOBJ}
	cc ${LFLAGS} -o ar ${OBJS} ${EXTRAOBJ}

ar.1.0: ar.1
	nroff -man ar.1 > ar.1.0

ar.5.0: ar.5.5
	nroff -man ar.5.5 > ar.5.0

install:
	install -s -m 755 ar ${DESTDIR}/bin/ar
	install -c -o bin -g bin -m 444 ar.1.0 ${DESTDIR}/usr/man/cat1/ar.0
	install -c -o bin -g bin -m 444 ar.5.0 ${DESTDIR}/usr/man/cat5/ar.0

lint:
	lint -havx -I. ${SRCS} ${EXTRASRCS}

clean:
	rm -f core ar *.o ar.1.0 ar.5.0

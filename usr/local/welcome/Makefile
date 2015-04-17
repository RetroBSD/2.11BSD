#	
#	Makefile for welcome.  Revised 1996/1/28
#
DESTDIR=
BINDIR=	/usr/local
CFLAGS= -O
SEPFLAG= -i
PROGRAM=	welcome
SRCS=		welcome.c
OBJS=		welcome.o
MAN=		welcome.0
LIBS=

all:	${PROGRAM} ${MAN}

${PROGRAM}:	${OBJS}
	${CC} ${SEPFLAG} ${CFLAGS} -o $@ ${OBJS} ${LIBS}

install: ${PROGRAM} welcome.0
	install -s -m 755 ${PROGRAM} ${DESTDIR}/${BINDIR}
	install -c -m 444 -o bin -g bin welcome.0 ${DESTDIR}/usr/local/man/cat1

welcome.0: welcome.1
	/usr/man/manroff welcome.1 > welcome.0

tags:
	ctags -tdw *.c

clean:
	rm -f a.out core *.o welcome.0 ${PROGRAM}

depend:
	mkdep welcome.c

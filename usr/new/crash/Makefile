#
# crash makefile
#

DESTDIR=

CFLAGS=	-O
SEPFLAG=-i

HDRS=	crash.h
SRCS=	crash.c crashsubs.c dispnet.c symbol.c if.c inet.c route.c
OBJS=	crash.o crashsubs.o dispnet.o symbol.o if.o inet.o route.o

.SUFFIXES: .1 .0

.1.0:
	/usr/man/manroff $*.1 > $*.0

crash: ${OBJS}
	${CC} -o $@ ${SEPFLAG} ${OBJS}

install: crash crash.0
	install -s crash ${DESTDIR}/usr/new/crash
	install -c crash.0 ${DESTDIR}/usr/new/man/cat1/crash.0

tags: ${HDRS} ${SRCS}
	ctags ${HDRS} ${SRCS}

clean:
	rm -f *.o crash tags core *.0

crash.o:	crash.c crash.h
crashsubs.o:	crashsubs.c crash.h
dispnet.o:	dispnet.c crash.h
symbol.o:	symbol.c crash.h

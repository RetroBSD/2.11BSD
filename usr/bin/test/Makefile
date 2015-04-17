#	Makefile for test
#
#	1.1 	Robin Birch	11 march 1995
#	1.2 	Steven Schultz	27 october 1996
#
#	Version 1.2
#
SEPFLAG= -i
CFLAGS = -O

SRCS = test.c operators.c
OBJS = test.o operators.o

all:	test test.0

test:	${OBJS}
	${CC} ${SEPFLAG} -o test ${OBJS}

test.0: test.1
	/usr/man/manroff test.1 > test.0

install: all
	install -s -m 755 -g bin -o bin test ${DESTDIR}/bin
	rm -f ${DESTDIR}/bin/[
	ln ${DESTDIR}/bin/test ${DESTDIR}/bin/[
	install -c -m 444 -g bin -o bin test.0 ${DESTDIR}/usr/man/cat1

clean:	
	rm -f *.o test *.core test.0

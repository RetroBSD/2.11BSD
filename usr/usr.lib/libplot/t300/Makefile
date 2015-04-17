#	Makefile	4.4	1/2/94
#
CFLAGS=	-O
OBJS=	arc.o box.o circle.o close.o dot.o erase.o label.o \
	line.o linmod.o move.o open.o point.o space.o subr.o

../lib300:	${OBJS}
	ar ru ../lib300 ${OBJS}

clean:
	rm -f ${OBJS} errs a.out core

install:

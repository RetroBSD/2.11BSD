#	Makefile	4.3	1/2/94
#
CFLAGS=	-O
OBJS=	arc.o box.o circle.o close.o cont.o dot.o erase.o label.o \
	line.o linmod.o move.o open.o point.o putsi.o space.o

../libplot:	${OBJS}
	ar ru ../libplot ${OBJS}

clean:
	rm -f ${OBJS} errs a.out core

install:

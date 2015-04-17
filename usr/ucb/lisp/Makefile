#
# lisp makefile
#
SEPFLAG= -i
CFLAGS=	-O

SRCS=	lisp.c
OBJS=	lisp.o

lisp: ${OBJS}
	${CC} -o lisp ${SEPFLAG} ${OBJS}

install: lisp
	install -s -m 711 lisp ${DESTDIR}/usr/ucb

clean:
	rm -f *.o a.out core errs lisp

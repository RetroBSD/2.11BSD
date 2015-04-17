#
# fp makefile
#
SEPFLAG= -i
CFLAGS=	-O

SRCS=	fp.c
OBJS=	fp.o

fp: ${OBJS}
	${CC} -o fp ${SEPFLAG} ${OBJS}

install: fp
	install -s fp ${DESTDIR}/usr/ucb

clean:
	rm -f *.o a.out core errs fp

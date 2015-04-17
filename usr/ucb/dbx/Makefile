#
# dbx makefile
#
SEPFLAG= -i
CFLAGS=	-O

SRCS=	dbx.c
OBJS=	dbx.o

dbx: ${OBJS}
	${CC} -o dbx ${SEPFLAG} ${OBJS}

install: dbx
	install -s dbx ${DESTDIR}/usr/ucb

clean:
	rm -f *.o a.out core errs dbx

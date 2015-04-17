# Version 2.2 January 24, 1998

SRCS=	ld.c
OBJS=	ld.o
MAN=	ld.0
MANSRC=	ld.1
CFLAGS=	-O -I../ar -DNUM_VM_PAGES=20

all:	ld ld.0

ld: ${OBJS}
	cc -i -o ld ${OBJS} -lvmf

install: all
	install -c -s -o bin -g bin -m 755 ld ${DESTDIR}/bin/ld
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1

lint:
	lint -havx -I../ar ${SRCS}

clean:
	rm -f ${OBJS} ${MAN} ld

${MAN}: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

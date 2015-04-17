SRCS=	nm.c
OBJS=	nm.o
CFLAGS=	-O -I../ar

all:	nm

nm: ${OBJS}
	cc -z -o nm ${OBJS}

install:
	install -c -s -o bin -g bin -m 755 nm ${DESTDIR}/bin/nm

lint:
	lint -havx -I../ar ${SRCS}

clean:
	rm -f *.o *.0 nm

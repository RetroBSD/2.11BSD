SRCS=	../../bin/ar/archive.c build.c misc.c ranlib.c touch.c
OBJS=	archive.o build.o ./misc.o ranlib.o touch.o
CFLAGS=	-O -I. -I../../bin/ar
VPATH= ../../bin/ar

all:	ranlib.0 ranlib.5.0 ranlib

ranlib.0: ranlib.1
	/usr/man/manroff ranlib.1 > ranlib.0

ranlib.5.0: ranlib.5.5
	/usr/man/manroff ranlib.5.5 > ranlib.5.0

ranlib: ${OBJS}
	cc -z -o ranlib ${OBJS}

install:
	install -c -o bin -g bin -m 444 ranlib.5.0 \
		${DESTDIR}/usr/man/cat5/ranlib.0
	install -c -o bin -g bin -m 444 ranlib.0 \
		${DESTDIR}/usr/man/cat1/ranlib.0
	install -c -s -o bin -g bin -m 755 ranlib ${DESTDIR}/usr/bin/ranlib

lint:
	lint -havx -I. -I../../bin/ar ${SRCS}

clean:
	rm -f *.o *.0 ranlib

CFLAGS = -n -O

all:	prep
	:

cp:	all
	cp prep ${DESTDIR}/usr/bin
	rm prep *.o

cmp:	all
	cmp prep ${DESTDIR}/usr/bin/prep
	rm *.o

prep:	prep0.o prep1.o prep2.o; cc -s -o prep -n *.o

prep0.o: prep0.c prep.h
prep1.o: prep1.c prep.h
prep2.o: prep2.c

clean:
	rm -f *.o

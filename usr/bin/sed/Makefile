#	Makefile	4.1	85/04/05
#
CFLAGS=-O -w
SEPFLAG=-i

sed:	sed0.o sed1.o
	cc ${SEPFLAG} -o sed *.o

sed0.o: sed0.c sed.h
sed1.o: sed1.c sed.h

install: sed
	install -s sed $(DESTDIR)/bin
clean:
	rm -f *.o sed

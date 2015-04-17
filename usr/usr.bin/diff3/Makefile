#	Makefile	4.1.1 (2.11BSD)	1996/10/23

CFLAGS=-O
SEPFLAG= -i
DESTDIR=

diff3: diff3.o
	cc ${SEPFLAG} -o diff3 diff3.o

install: diff3
	install -s -m 755 diff3 $(DESTDIR)/usr/libexec
	install -c -m 755 diff3.sh $(DESTDIR)/usr/bin/diff3

clean:
	rm -f diff3.o diff3	

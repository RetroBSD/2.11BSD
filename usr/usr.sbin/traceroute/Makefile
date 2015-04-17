#
# traceroute Makefile.  Revised 1996/11/17
#
INCL =
CFLAGS = -O -i
LIBS =

all: traceroute traceroute.0

traceroute: traceroute.o
	cc $(CFLAGS) $(INCL) -o traceroute traceroute.o $(LIBS)

install: traceroute traceroute.0
	install -c -s -o root -g bin -m 4551 traceroute ${DESTDIR}/usr/sbin/traceroute 
	install -c -m 444 -o bin -g bin traceroute.0 ${DESTDIR}/usr/man/cat8

traceroute.0: traceroute.8
	/usr/man/manroff traceroute.8 > traceroute.0

clean:
	rm -f *.o traceroute traceroute.0 core

lint:
	lint -b -h -x $(INCL) traceroute.c | \
	  grep -v 'possible pointer alignment problem'

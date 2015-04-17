#	Makefile	4.1.1	(2.11BSD) 1996/10/23
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

calendar: calendar.c
	${CC} ${CFLAGS} ${SEPFLAG} -o calendar calendar.c

install: calendar calendar.sh
	install -s calendar $(DESTDIR)/usr/libexec
	install -c -m 755 calendar.sh $(DESTDIR)/usr/bin/calendar

clean:
	rm -f calendar

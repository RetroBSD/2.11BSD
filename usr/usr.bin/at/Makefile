#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4.1 (2.11BSD) 1996/10/23
#
#       Makefile for at, atq, atrm, and atrun
#
ALL=		atq atrm at atrun
SRCS=		atq.c atrm.c at.c atrun.c
CFLAGS=		-O
DESTDIR=
SPOOLDIRS=	/usr/spool/at /usr/spool/at/past
SPOOLFILES=	/usr/spool/at/lasttimedone
SEPFLAG= -i

all: ${ALL}

at: at.c
	$(CC) $(CFLAGS) ${SEPFLAG} -o at at.c

atq: atq.c
	$(CC) $(CFLAGS) ${SEPFLAG} -o atq atq.c

atrm: atrm.c
	$(CC) $(CFLAGS) ${SEPFLAG} -o atrm atrm.c

atrun: atrun.c
	$(CC) $(CFLAGS) ${SEPFLAG} -o atrun atrun.c

install: all
	-for i in $(SPOOLDIRS); \
		do mkdir $(DESTDIR)/$$i; \
		chown daemon $(DESTDIR)/$$i; \
		chgrp daemon $(DESTDIR)/$$i; \
		chmod 755    $(DESTDIR)/$$i; \
		done
	-for i in $(SPOOLFILES); \
		do if [ ! -f $(DESTDIR)/$$i ]; \
		then echo 0 > $(DESTDIR)/$$i; fi; \
		chown daemon $(DESTDIR)/$$i; \
		chgrp daemon $(DESTDIR)/$$i; \
		done
	install -s -o daemon atrun $(DESTDIR)/usr/libexec
	install -s -o daemon -m 4755 atq $(DESTDIR)/usr/bin
	install -s -o daemon -m 4755 at $(DESTDIR)/usr/bin
	install -s -o daemon -m 4755 atrm $(DESTDIR)/usr/bin

clean:
	rm -f $(ALL) errs *.o

depend:
	chmod u+w Makefile
	mkdep ${CFLAGS} ${SRCS}

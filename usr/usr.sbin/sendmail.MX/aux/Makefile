#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	@(#)Makefile	5.2 (Berkeley) 5/24/88
#
CFLAGS=	-O -I../src -I../include -DVMUNIX
SEPFLAG=-i
ALL=	mconnect mailstats praliases
SRCS=	mconnect.c mailstats.c praliases.c
OBJS=	mconnect.o mailstats.o praliases.o

.c.o:
	${CC} -S ${CFLAGS} $*.c
	sed -f ../include/asm.sed $*.s > _xx.s
	as -V - -o $*.o _xx.s
	rm -f $*.s _xx.s

all: ${ALL}

mail: mail.o
	${CC} ${SEPFLAG} -o $@ mail.o

rmail: rmail.o
	${CC} ${SEPFLAG} -o $@ rmail.o

mconnect: mconnect.o
	${CC} ${SEPFLAG} -o $@ mconnect.o

mailstats: mailstats.o
	${CC} ${SEPFLAG} -o $@ mailstats.o

praliases: praliases.o
	${CC} ${SEPFLAG} -o $@ praliases.o -ldbm

clean: FRC
	rm -f core ${ALL} ${OBJS}

depend: FRC
	mkdep ${CFLAGS} ${SRCS}

install: ${ALL}
#	install -s -o root -m 4755 mail ${DESTDIR}/bin/mail
#	install -s -o bin -m 755 rmail ${DESTDIR}/bin/rmail
	install -s -o bin -m 755 mconnect mailstats praliases \
		${DESTDIR}/usr/bin

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

mconnect.o: mconnect.c /usr/include/stdio.h /usr/include/signal.h
mconnect.o: /usr/include/ctype.h /usr/include/sgtty.h /usr/include/sys/ioctl.h
mconnect.o: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
mconnect.o: /usr/include/sys/types.h /usr/include/sys/socket.h
mconnect.o: /usr/include/short_names.h /usr/include/netinet/in.h
mconnect.o: /usr/include/short_names.h /usr/include/netdb.h
mconnect.o: /usr/include/short_names.h
mailstats.o: mailstats.c /usr/include/sys/file.h ../src/sendmail.h
mailstats.o: /usr/include/stdio.h /usr/include/ctype.h /usr/include/setjmp.h
mailstats.o: ../src/conf.h ../include/useful.h /usr/include/sys/types.h
mailstats.o: /usr/include/sys/syslog.h /usr/include/sys/socket.h
mailstats.o: /usr/include/short_names.h /usr/include/netinet/in.h
mailstats.o: /usr/include/short_names.h /usr/include/sysexits.h
mailstats.o: ../src/mailstats.h
praliases.o: praliases.c ../src/sendmail.h /usr/include/stdio.h
praliases.o: /usr/include/ctype.h /usr/include/setjmp.h ../src/conf.h
praliases.o: ../include/useful.h /usr/include/sys/types.h
praliases.o: /usr/include/sys/syslog.h /usr/include/sys/socket.h
praliases.o: /usr/include/short_names.h /usr/include/netinet/in.h
praliases.o: /usr/include/short_names.h /usr/include/sysexits.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY

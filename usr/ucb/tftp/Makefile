#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2 (Berkeley) 2/6/86
#
ALL=	tftp tftp.0
DESTDIR=
CFLAGS=-O
SEPFLAG=-i

all: ${ALL}

tftp:	main.o tftp.o tftpsubs.o
	${CC} main.o tftp.o tftpsubs.o ${SEPFLAG} -o tftp

tftp.0:	tftp.1
	/usr/man/manroff tftp.1 > tftp.0

clean:
	rm -f ${ALL} *.o *.s errs core a.out t.? tftp.0

install:  tftp tftp.0
	install -s tftp ${DESTDIR}/usr/ucb
	install -c -m 444 tftp.0 ${DESTDIR}/usr/man/cat1

#
# Public Domain.  1996/11/16 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	mklost+found.sh
MAN=	mklost+found.0
MANSRC=	mklost+found.8

all: ${MAN}

mklost+found.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${MAN}

depend:

install:
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	install -c -o root -g bin -m 555 ${SRCS} ${DESTDIR}/usr/sbin/mklost+found

lint:

tags:

#
# Public Domain.  1995/05/02 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1995/05/02
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	printf.c
OBJS=	printf.o
MAN=	printf.0
MANSRC=	printf.1

all: printf printf.0

printf: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

printf.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} printf tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
	install -s -o root -g bin -m 755 printf ${DESTDIR}/usr/bin/printf

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

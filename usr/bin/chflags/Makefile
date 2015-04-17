#
# Public Domain.  12/3/1994 - Steven Schultz
#
#	@(#)Makefile	1.2 (2.11BSD GTE) 1996/1/27
#
CFLAGS=	 -O
SEPFLAG= -i
VPATH= ../ls
SRCS=	chflags.c stat_flags.c
OBJS=	chflags.o stat_flags.o
MAN=	chflags.0
MANSRC=	chflags.1

all: chflags chflags.0

chflags: ${OBJS}
	${CC} ${CFLAGS} ${SEPFLAG} -o $@ ${OBJS}

chflags.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} chflags tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: chflags
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1
	install -s -o root -g bin -m 755 chflags ${DESTDIR}/bin/chflags

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

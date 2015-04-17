#
# Public Domain.  1996/11/16 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/11/16
#
CFLAGS=	 -O
SEPFLAG= -i
SRCS=	mkhosts.c
OBJS=	mkhosts.o
MAN=	mkhosts.0
MANSRC=	mkhosts.8

all: mkhosts mkhosts.0

mkhosts: ${OBJS}
	@echo "Mkhosts only works with the dbm version of the hostent routines."
	@echo "If gethostent() is not in libc, mkhosts isn't installed."
	-cc ${CFLAGS} -o mkhosts ${OBJS}

mkhosts.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} mkhosts tags 

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: mkhosts
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat8
	-if  test -x mkhosts;\
	then \
		install -s -o root -g bin -m 755 mkhosts ${DESTDIR}/usr/sbin ;\
	else \
		:;\
	fi

lint: ${SRCS}
	lint -hax ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

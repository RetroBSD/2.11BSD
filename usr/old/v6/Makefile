#
# v6 makefile
#

SEPFLAG=-i
DEFS=	
CFLAGS=	-O ${DEFS}

SRCS=	v6.c
OBJS=	v6.o

all: ${OBJS} inode.h filsys.h
	cc ${SEPFLAG} ${OBJS} -o v6

install:
	install -s v6 ${DESTDIR}/usr/old/v6

clean:
	rm -f v6 *.o core

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f tags $$cwd/$$i; \
	done

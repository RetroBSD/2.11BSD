#
# Makefile for common directory
#

SRCS	=	version.c clientlib.c
OBJS	=	version.o clientlib.o
HFILES	=

clean:
	-rm -f ${OBJS}

distrib: clean
	rm -rf SCCS

check:
	sccs check

lint:
	lint ${SRCS}

tags:
	ctags ${CFILES}

#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1995/1/12
#
# This is the Makefile for 'stdlib'.  New routines ported from 4.4BSD's
# libc/stdlib directory go here but existing libc/gen files are being left
# where they are.
#

DEFS=
CFLAGS=	 -O ${DEFS}
CC=	cc

SRCS=	getopt.c getsubopt.c strtol.c strtoul.c
OBJS=	getopt.o getsubopt.o strtol.o strtoul.o

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	ld -X -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	ld -X -r $*.o
	mv a.out $*.o

all: stdliblib stdliblib_p

stdliblib stdliblib_p: ${OBJS}
	@echo "building profiled stdlib"
	@cd profiled; ar cru ../stdliblib_p ${OBJS}
	@echo "building normal stdlib"
	@ar cru stdliblib ${OBJS}

clean:
	rm -f ${OBJS} profiled/*.o tags Makefile.bak stdliblib stdliblib_p

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

#
#	@(#)Makefile	1.2 (2.11BSD GTE) 1996/3/20
#
# This is the Makefile for the 'string' functions.  New routines ported from 
# 4.4BSD's libc/string directory go here but existing libc/gen files are 
# being left where they are.
#

DEFS=
CFLAGS=	 -O ${DEFS}
CC=	cc

SRCS=	strcspn.c strpbrk.c strerror.c strsep.c strspn.c strstr.c strtok.c
OBJS=	strcspn.o strpbrk.o strerror.o strsep.o strspn.o strstr.o strtok.o

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	ld -X -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	ld -X -r $*.o
	mv a.out $*.o

all: stringlib stringlib_p

stringlib stringlib_p: ${OBJS}
	@echo "building profiled stringlib"
	@cd profiled; ar cru ../stringlib_p ${OBJS}
	@echo "building normal stringlib"
	@ar cru stringlib ${OBJS}

clean:
	rm -f ${OBJS} profiled/*.o tags Makefile.bak stringlib stringlib_p

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

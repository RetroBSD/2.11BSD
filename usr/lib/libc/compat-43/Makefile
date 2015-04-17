#
# Public domain 1997/8/28
#
#	@(#)Makefile	1.1 (2.11BSD) 1997/11/30
#
CFLAGS=	-O ${DEFS}
SRCS= creat.c gethostid.c sethostid.c sigcompat.c \
	setregid.c setreuid.c setrgid.c setruid.c
OBJS= creat.o gethostid.o sethostid.o sigcompat.o \
	setregid.o setreuid.o setrgid.o setruid.o

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	-ld -X -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -x -r $*.o
	mv a.out $*.o

all: compat-43lib compat-43lib_p

compat-43lib compat-43lib_p: ${OBJS}
	@echo "building profiled compat-43lib"
	@cd profiled; ar cru ../compat-43lib_p ${OBJS}
	@echo "buiding normal compat-43lib"
	@ar cru compat-43lib ${OBJS}

clean:
	rm -f compat-43lib compat-43lib_p *.o profiled/*.o Makefile.bak tags

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

# DO NOT DELETE THIS LINE -- make depend uses it

#	Makefile	1.6	1996/1/11
#
CFLAGS=	-O ${DEFS}
# strchr and strrchr.c are not used.  The assembly versions in pdp/gen are
# used instead.
SRCS=	memccpy.c memchr.c memcmp.c memcpy.c memset.c tmpnam.c
OBJS=	memccpy.o memchr.o memcmp.o memcpy.o memset.o tmpnam.o
TAGSFILE=tags

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	-ld -X -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -x -r $*.o
	mv a.out $*.o

compat-sys5lib compat-sys5lib_p: ${OBJS}
	@echo "building profiled compat-sys5lib"
	@cd profiled; ar cru ../compat-sys5lib_p ${OBJS}
	@echo "buiding normal compat-sys5lib"
	@ar cru compat-sys5lib ${OBJS}

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f compat-sys5lib compat-sys5lib_p *.o profiled/*.o Makefile.bak

depend:
	mkdep ${CFLAGS} ${SRCS}

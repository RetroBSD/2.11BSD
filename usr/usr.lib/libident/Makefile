#
# Place in the public domain 1996/12/13 - sms.
#
# @(#)Makefile 1.0 (2.11BSD) 1996/12/13
#

DESTDIR=
LIBDIR=$(DESTDIR)/usr/lib
INCDIR=$(DESTDIR)/usr/include
MANDIR=$(DESTDIR)/usr/man/cat3

CFLAGS=-O -DSIGRETURNTYPE=int -DHAVE_ANSIHEADERS

LFLAGS=-i
LIB= libident.a

SRCS = ident.c id_open.c id_close.c id_query.c id_parse.c support.c version.c
OBJS = ident.o id_open.o id_close.o id_query.o id_parse.o support.o version.o

.c.o:
	@${CC} -p ${CFLAGS} -c $*.c
	@-ld -X -o profiled/$*.o -r $*.o
	${CC} ${CFLAGS} -c $*.c
	@-ld -x -r $*.o
	@mv a.out $*.o

all: 	libident.a libident_p.a lookup-tester ident-tester ident.0

libident.a libident_p.a:	$(OBJS)
	@echo building normal libident
	@ar ru libident.a ${OBJS}
	ranlib libident.a
	@echo building profiled libident
	@cd profiled; ar ru ../libident_p.a ${OBJS}
	ranlib libident_p.a

ident-tester: libident.a ident-tester.o
	${CC} $(CFLAGS) ${LFLAGS} -o ident-tester ident-tester.o $(LIB)

lookup-tester: libident.a lookup-tester.o
	${CC} $(CFLAGS) ${LFLAGS} -o lookup-tester lookup-tester.o $(LIB)

ident.0: ident.3
	/usr/man/manroff ident.3 > ident.0

install: all
	install -c -m 644  libident.a $(LIBDIR)/libident.a
	install -c -m 644 libident_p.a $(LIBDIR)/libident_p.a
	ranlib $(LIBDIR)/libident.a $(LIBDIR)/libident_p.a
	install -c -m 444  ident.h $(INCDIR)/ident.h
	install -c -m 444 ident.0 $(MANDIR)/ident.0

clean:
	-rm -f libident.a libident_p.a ident-tester lookup-tester *.o ident.0
	-rm -f profiled/*.o

depend:
	mkdep ${CFLAGS} ${SRCS}

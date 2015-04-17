#
# Public Domain.  1996/3/20 - Steven Schultz
#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1996/3/20
#
DEFS=
CFLAGS=-O $(DEFS)
TAGSFILE=tags

all: liberrlst.a liberrlst_p.a

liberrlst.a: errlst.c
	${CC} -c ${CFLAGS} errlst.c
	ld -r -X -o liberrlst.a errlst.o
	rm -f errlst.o

liberrlst_p.a: errlst.c
	${CC} -p -c ${CFLAGS} errlst.c
	ld -r -X -o liberrlst_p.a errlst.o
	rm -f errlst.o

install: all
	install -m 644 liberrlst.a $(DESTDIR)/usr/lib
	install -m 644 liberrlst_p.a $(DESTDIR)/usr/lib

tags:
	ctags -a -f ${TAGSFILE} `pwd`/errlst.c

clean:
	rm -f *.o liberrlst.a liberrlst_p.a

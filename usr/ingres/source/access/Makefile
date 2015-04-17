CC=	cc
CFLAGS=	-O
AR=	ar
ARFLAGS=rvuc
LIB=	../../lib

OBJS=	opencatalog.o closecatalog.o catalog_desc.o inpcloser.o closer.o \
	noclose.o openr.o accbuf.o acc_addbuf.o batch.o batchxtra.o \
	writebatch.o getequal.o get.o insert.o replace.o delete.o setkey.o \
	find.o rhash.o ndxsearch.o findbest.o scan_dups.o get_tuple.o \
	put_tuple.o del_tuple.o tup_len.o newlino.o add_ovflo.o formatpg.o \
	last_page.o page.o compare.o add_prim.o utility.o cleanrel.o \
	readadmin.o

access: $(OBJS)
	-$(AR) $(ARFLAGS) access $(OBJS)
	ranlib access

install: access
	install -c -m 644 access $(LIB)/access
	ranlib $(LIB)/access

clean:
	rm -f access *.o *.core a.out

LIB=../../lib
BIN=../../bin
CC=cc
CFLAGS=-O
LDFLAGS= -i
AR=ar
LIBS=$(LIB)/parlib $(LIB)/access $(LIB)/iutil $(LIB)/gutil

OBJS=	main.o readtree.o qrymod.o issue.o define.o d_view.o d_integ.o \
	d_prot.o fillprotup.o view.o integrity.o protect.o writetree.o \
	range.o tree.o trscan.o treepr.o util.o catalog_desc.o

qrymod: qmlib
	$(CC) $(LDFLAGS) -o qrymod qmlib $(LIBS)

qmlib: $(OBJS)
	-$(AR) cruv qmlib $(OBJS)
	ranlib qmlib

install: qmlib qrymod
	install -c -m 644 qmlib $(LIB)/qmlib
	ranlib $(LIB)/qmlib
	install -c -m 700 qrymod $(BIN)/qrymod

clean: 
	rm -f qrymod qmlib *.o *.core a.out

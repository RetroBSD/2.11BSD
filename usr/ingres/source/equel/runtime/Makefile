INGRES=../../..
LIB=$(INGRES)/lib
BIN=$(INGRES)/bin
CC=cc
CFLAGS=-O
AR=ar

OBJS=	IIw_left.o IIw_right.o IIcvar.o IIgettup.o IIn_get.o IIn_ret.o \
	IIingres.o IIrdpipe.o IIretrieve.o IIsetup.o IIflushtup.o IIsync.o \
	IIwrite.o IIwrpipe.o IIexit.o IIbmove.o IIsyserr.o IIconvert.o \
	IIerror.o IIp_err.o IIgetc.o IIprpipe.o IIresync.o IIitos.o \
	IIconcatv.o IIbmove.o IIlength.o IIatoi.o IIsequal.o

libq.a: $(OBJS)
	$(AR) rvuc libq.a $(OBJS)
	ranlib libq.a

install: libq.a
	install -c -m 644 libq.a $(LIB)/libq.a
	ranlib $(LIB)/libq.a

clean:
	rm -f libq.a *.o *.core a.out

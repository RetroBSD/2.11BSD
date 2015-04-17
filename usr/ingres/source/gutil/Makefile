INGRES=	../..
CC=	cc
CFLAGS=	-O
AR=	ar
ARFLAGS=rvuc
LIB=	$(INGRES)/lib

OBJS=	atof.o atoi.o atol.o bequal.o bitcnt.o bitpos.o bmove.o cat.o \
	concat.o dysize.o fgetline.o flush.o freebuf.o ftoa.o fullwait.o \
	getufield.o iocv.o itoa.o length.o locv.o longconst.o need.o oatoi.o \
	pad.o pmove.o prargs.o scompare.o sequal.o setflag.o smove.o syserr.o \
	tTf.o trace.o xputchar.o ztack.o

gutil:	$(OBJS)
	-$(AR) $(ARFLAGS) gutil $(OBJS)	
	ranlib gutil

.s.o:
	as -o $@ $<

install: gutil
	install -c -m 644 gutil $(LIB)/gutil
	ranlib $(LIB)/gutil

clean:
	rm -f gutil *.o *.core a.out

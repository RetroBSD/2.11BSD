INGRES=	../..
CC=	cc
CFLAGS=	-O
LDFLAGS=-i
AR=	ar
ARFLAGS=rvuc
BIN=	$(INGRES)/bin
LIB=	$(INGRES)/lib
ACCESS=	$(LIB)/access
GUTIL=	$(LIB)/gutil
IUTIL=	$(LIB)/iutil

LIBS=	$(ACCESS) $(IUTIL) $(GUTIL)

OBJS=	ovqp.o startovqp.o getqry.o rdsym.o strategy.o findsimps.o \
	key.o scan.o interp.o interp1.o equelpr.o string.o typecheck.o \
	support.o prsym.o copyreturn.o

all: ovqplib ovqp

ovqp: main.o $(OBJS) $(LIBS)
	$(CC) $(LDFLAGS) -o ovqp main.o $(OBJS) $(LIBS) -lm

ovqplib: $(OBJS)
	-$(AR) $(ARFLAGS) ovqplib $(OBJS)
	ranlib ovqplib

install: ovqplib ovqp
	install -c -m 644 ovqplib $(LIB)/ovqplib
	ranlib $(LIB)/ovqplib
	install -c -m 700 ovqp $(BIN)/ovqp

clean:
	rm -f ovqplib ovqp *.o *.core a.out

LIB=../../lib
BIN=../../bin
LIBS=$(LIB)/access $(LIB)/iutil $(LIB)/gutil -lm
CC=cc
CFLAGS=-O
LDFLAGS=-i
AR=ar
ARFLAGS=rvuc

OBJS=	index.o create.o modify.o modupdate.o destroy.o userdestroy.o \
	purgetup.o print.o help.o copy.o save.o rupdate.o update.o convert.o \
	secupdate.o rdwrbatch.o resetrel.o get_seq_atts.o dbuproctab.o \
	display.o pr_tree.o readtree.o pr_prot.o rmqm.o get_p_tid.o

all: dbulib overlaya overlayc overlayi overlaym overlayn overlayg ksort

overlaya: dbulib main.o overlaya.o
	$(CC) $(LDFLAGS) -o overlaya main.o overlaya.o dbulib $(LIBS)

overlayc: dbulib main.o overlayc.o
	$(CC) $(LDFLAGS) -o overlayc main.o overlayc.o dbulib $(LIBS)

overlayi: dbulib main.o overlayi.o
	$(CC) $(LDFLAGS) -o overlayi main.o overlayi.o dbulib $(LIBS)

overlaym: dbulib main.o overlaym.o
	$(CC) $(LDFLAGS) -o overlaym main.o overlaym.o dbulib $(LIBS)

overlayn: dbulib main.o overlayn.o
	$(CC) $(LDFLAGS) -o overlayn main.o overlayn.o dbulib $(LIBS)

overlayg: dbulib main.o overlayg.o
	$(CC) $(LDFLAGS) -o overlayg main.o overlayg.o dbulib $(LIBS)

dbulib: $(OBJS)
	-$(AR) rvuc dbulib $(OBJS)
	ranlib dbulib

ksort: ksort.o
	$(CC) $(LDFLAGS) -o ksort ksort.o $(LIBS)

install: all
	install -c -m 644 dbulib $(LIB)/dbulib
	ranlib $(LIB)/dbulib
	install -c -m 700 overlaya $(BIN)/overlaya
	install -c -m 700 overlayc $(BIN)/overlayc
	install -c -m 700 overlayi $(BIN)/overlayi
	install -c -m 700 overlaym $(BIN)/overlaym
	install -c -m 700 overlayn $(BIN)/overlayn
	install -c -m 700 overlayg $(BIN)/overlayg
	install -c -m 700 ksort $(BIN)/ksort

clean:
	rm -f dbulib overlay[acimng] ksort *.o *.core a.out

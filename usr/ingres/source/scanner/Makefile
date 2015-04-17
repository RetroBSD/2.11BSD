INGRES=	../..
LIB=	$(INGRES)/lib
CC=	cc
CFLAGS=	-O
AR=	ar
ARFLAGS=rvuc


OBJS=	parser.o yylex.o inout.o name.o number.o operator.o string.o symtab.o \
	comment.o cmap.o

scanner: $(OBJS)
	-$(AR) $(ARFLAGS) scanner $(OBJS)
	ranlib scanner

install: scanner
	install -c -m 644 scanner $(LIB)/scanner
	ranlib $(LIB)/scanner

clean:
	rm -f scanner *.o *.core a.out

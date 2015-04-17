INGRES=../..
BIN=$(INGRES)/bin
LIB=$(INGRES)/lib
LIBS=$(LIB)/iutil $(LIB)/gutil -lm
YACC=$(BIN)/yyyacc
YFLAGS=
CC=cc
CFLAGS=-O
LDFLAGS=-i
AR=ar

OBJS=	main.o parser.o yylex.o cvar.o include.o name.o getkey.o number.o \
	operator.o prtout.o string.o comment.o retrieve.o display.o getch.o \
	cmap.o yyerror.o nalloc.o

equel: y.tab.o equelib
	$(CC) $(LDFLAGS) -o equel y.tab.o equelib $(LIBS)

y.tab.c: grammar.y tokens.y
	$(YACC) $(YFLAGS) grammar.y

equelib: $(OBJS)
	-$(AR) crvu equelib $(OBJS)
	ranlib equelib

install: equelib equel
	install -c -m 644 equelib $(LIB)/equelib
	ranlib $(LIB)/equelib
	install -c -m 751 equel $(BIN)/equel

clean:
	rm -f equel equelib y.tab.c *.o *.core a.out

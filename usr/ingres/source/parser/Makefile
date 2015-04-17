CC=cc
LIB=../../lib
BIN=../../bin
LIBS=$(LIB)/scanner $(LIB)/access $(LIB)/gutil $(LIB)/iutil -lm
CFLAGS=-O
LDFLAGS=-i
YACC=$(BIN)/yyyacc
YFLAGS=-s
AR= ar

OBJS=	attfcn.o xdot.o main.o rngfcn.o tree.o norml.o call_p.o call_tree.o \
	control.o format.o p_util.o getscr.o

parser: y.tab.o parlib
	$(CC) $(LDFLAGS) -o parser y.tab.o parlib $(LIBS)

parlib: $(OBJS)
	-$(AR) rvuc parlib $(OBJS)
	ranlib parlib

y.tab.c: grammar.y yyerror.y tables.y
	$(YACC) $(YFLAGS) grammar.y

grammar.y: master.grammar
	grep -v "DDD" master.grammar > grammar.y 

install: parlib parser
	install -c -m 644  parlib $(LIB)/parlib
	ranlib $(LIB)/parlib
	install -c -m 700 parser $(BIN)/parser

clean:
	rm -f parser parlib grammar.y y.tab.c *.o *.core a.out

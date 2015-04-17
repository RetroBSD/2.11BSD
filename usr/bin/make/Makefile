# Description file for the Make command
# Makefile 4.5 94/1/3

OBJECTS=ident.o main.o doname.o misc.o files.o dosys.o gram.o 
LIBES= 
LINT=	lint -ps
XSTR=   /usr/ucb/xstr
CFLAGS=	-O -I. -I../ar
SEPFLAG= -i

# Special massaging of C files for sharing of strings
.c.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
	${CC} -c ${CFLAGS} x.c 
	mv -f x.o $*.o
	rm -f x.c

all:	make

make:	${OBJECTS} strings.o
	${CC} -o make ${CFLAGS} ${SEPFLAG} ${OBJECTS} strings.o ${LIBES}

${OBJECTS}:  defs

clean:
	-rm -f *.o gram.c make a.out errs x.c xs.c strings

install: make
	install -s make ${DESTDIR}/bin/make

lint :  dosys.c doname.c files.c main.c misc.c ident.c gram.c
	${LINT} dosys.c doname.c files.c main.c misc.c ident.c gram.c
	rm -f gram.c

gram.c: gram.y
	yacc gram.y
	fgrep -v "static char yaccpar_sccsid" y.tab.c >gram.c
	rm -f y.tab.c

strings.o: strings
	${XSTR}
	${CC} -c xs.c
	mv -f xs.o strings.o
	rm -f xs.c

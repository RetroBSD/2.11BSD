#	@(#)Makefile	4.1	12/18/82

CFLAGS=-O
SEPFLAG= -i
h = tp.h tp_defs.h
o  = tp0.o tp1.o tp2.o tp3.o

tp :	$o
	$(CC) ${SEPFLAG} -o tp $o

$o :	$h

install : tp
	install -s tp ${DESTDIR}/bin

clean : 
	rm -f *.o

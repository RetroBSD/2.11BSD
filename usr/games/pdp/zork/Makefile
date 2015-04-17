BINDIR = /usr/games
LIBDIR = /usr/games/lib
DESTDIR=

CFLAGS = -O
SEPFLAG= -i

all:	z zork

z:	z.c
	cc -O ${SEPFLAG} -o z z.c

# All the 1.o stuff is just to do a . = . + 140000
zork:	1.o o.o 
	ld 1.o 1.o 1.o o.o
	nm a.out >syms
	dd if=a.out of=h bs=16 count=1
	dd if=a.out of=b.out bs=26120 skip=2
	cat h d b.out >zork
	rm a.out h b.out
	chmod 755 zork
	./p1

install: z zork
	install -s z ${DESTDIR}${BINDIR}/zork
	install -s zork ${DESTDIR}${LIBDIR}/zork
	install -c -m 644 doverlay ${DESTDIR}${LIBDIR}
	install -c -m 644 dindex.dat ${DESTDIR}${LIBDIR}
	install -c -m 644 dtext.dat ${DESTDIR}${LIBDIR}

clean:
	rm -f 1.out a.out b.out h *.o core dungeon.dat zork z syms

1.o:	1.s
o.o:	o.s

.s.o:
	/lib/cpp -E $< | sed -e 's;^#;/;' | as -o $@

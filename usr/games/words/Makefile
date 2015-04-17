LIBDIR=/usr/games/lib

all:	words1

words1:	words1.c
	cc -O -n -o words1 words1.c

install: words1 words.sh
	install -s words1 ${LIBDIR}
	install -c words.sh ${DESTDIR}/usr/games/words

clean:
	rm -f *.o words1

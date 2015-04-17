#	@(#)Makefile	4.3.1 (2.11BSD)	1996/10/23

CFLAGS= -O
SEPFLAG= -i

all:	spell hlist hlista hlistb hstop spellin spellout

install:	all
	install -s spell $(DESTDIR)/usr/libexec
	install -s spellin ${DESTDIR}/usr/bin
	install -s spellout ${DESTDIR}/usr/bin
	install -c hlista $(DESTDIR)/usr/dict
	install -c hlistb $(DESTDIR)/usr/dict
	install -c hstop $(DESTDIR)/usr/dict
	install -c spell.sh $(DESTDIR)/usr/bin/spell

clean:	
	rm -f *.o spell spellin hlista hlistb hlist hstop spellout

spell: spell.o
	cc ${SEPFLAG} spell.o -o spell
spellin: spellin.o
	cc ${SEPFLAG} spellin.o -o spellin
spellout: spellout.o
	cc ${SEPFLAG} spellout.o -o spellout

hlist: /usr/dict/words spellin
	spellin </usr/dict/words >hlist
hlista: american local hlist spellin
	(cat american local)|spellin hlist >hlista
hlistb: british local hlist spellin
	(cat british local)|spellin hlist >hlistb
hstop: stop spellin
	spellin <stop >hstop

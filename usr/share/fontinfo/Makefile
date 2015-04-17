# Public domain - 1996/10/22 - sms
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/10/22
#
#  Makefile for fontinfo files.
#

SRCS= apl basker.b basker.i basker.r bocklin bodoni.b bodoni.i bodoni.r \
	chess clarendon cm.b cm.i cm.r countdown cyrillic delegate.b \
	delegate.i delegate.r fix gacham.b gacham.i gacham.r graphics \
	greek h19 hebrew meteor.b meteor.i meteor.r mona nonie.b nonie.i \
	nonie.r oldenglish otimes.b otimes.i otimes.r otimes.s pip playbill \
	script seal shadow sign stare.b stare.i stare.r times.b times.i \
	times.r times.s ugramma wumpus

all:	${SRCS}

install:${SRCS}
	-mkdir -p ${DESTDIR}/usr/share/fontinfo
	-chmod a+r,a+x ${DESTDIR}/usr/share/fontinfo
	install -c -m 444 ${SRCS} ${DESTDIR}/usr/share/fontinfo

clean:

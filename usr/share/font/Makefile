#
# Public domain - 1996/10/23 - sms
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/10/23
#
#  Makefile for the troff/nroff fonts
#

SRCS= fontlist ftB ftBC ftC ftCE ftCI ftCK ftCW ftFD ftG ftGI ftGM ftGR \
	ftH ftHI ftHM ftI ftL ftLI ftPA ftPB ftPI ftR ftS ftSB ftSI ftSM \
	ftTB ftTI ftTR ftTS ftUD ftap ftbb ftbi ftbk ftbr ftcb ftch ftci \
	ftcl ftco ftcr ftct ftcy ftdb ftdi ftdr ftfx ftgb ftgf ftgi ftgk \
	ftgr fthb fthn ftm ftmb ftmi ftmn ftmr ftnb ftni ftnr ftob ftoe ftoi \
	ftor ftpb ftpp ftsb ftsc ftsg ftsh ftsi ftsl ftsr fttB fttI fttR fttS \
	fttb ftti fttr ftts ftwp ftxx

all:	${SRCS}

install:${SRCS}
	-mkdir -p ${DESTDIR}/usr/share/font
	-chmod a+r,a+x ${DESTDIR}/usr/share/font
	install -c -m 444 ${SRCS} ${DESTDIR}/usr/share/font

clean:

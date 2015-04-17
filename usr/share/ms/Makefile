#
# Public domain - 1996/10/21 - sms.
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/10/21
#
#  Makefile for -ms macros
#

SRCS=	README end.awk endnote s.acc s.cov s.eqn s.ref s.tbl s.ths s.toc

all:	${SRCS}

install:${SRCS}
	-mkdir -p /usr/share/ms
	-chmod a+r /usr/share/ms
	install -c -m 444 ${SRCS} ${DESTDIR}/usr/share/ms
	chmod a+x ${DESTDIR}/usr/share/ms/endnote

clean:

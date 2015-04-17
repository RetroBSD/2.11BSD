#
# Public domain - 1996/10/21 - sms
#
#	@(#)Makefile	1.0 (2.11BSD) 1996/10/21
#
#  Makefile for adb scripts
#

SRCS= buf callout callout.next file ifaddr ifnet inode ip label mbstat \
	mbuf mbuf.nxt mbufs mbufs.nxt proc protosw rusage socket tcpcb u xip

all:	${SRCS}

install:${SRCS}
	-mkdir -p ${DESTDIR}/usr/share/adb
	-chmod a+r,a+x ${DESTDIR}/usr/share/adb
	install -c -m 444 ${SRCS} ${DESTDIR}/usr/share/adb

clean:

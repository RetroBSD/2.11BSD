#
# Public domain - 1996/10/23 - sms
#
#	@(#)Makefile	1.1 (2.11BSD) 1997/9/26
#
#  Makefile for the lint libraries.
#
LINT1=/usr/libexec/lint/lint1
C=/lib/cpp
S=${DESTDIR}/usr/share/lint

SRCS= llib-lc llib-lcurses llib-ldbm llib-lm llib-lmp \
	llib-lplot llib-ltermcap llib-port

all:

install:${SRCS}
	-mkdir -p ${S}
	-chmod a+r,a+x ${S}
	install -c -m 444 ${SRCS} ${S}

clean:

tags:

libs:
	${C} -C -Dlint ${S}/llib-port | ${LINT1} -puv > ${S}/llib-port.ln
	${C} -C -Dlint ${S}/llib-ldbm | ${LINT1} -uv > ${S}/llib-ldbm.ln
	${C} -C -Dlint ${S}/llib-lm | ${LINT1} -uv > ${S}/llib-lm.ln
	${C} -C -Dlint ${S}/llib-lmp | ${LINT1} -uv > ${S}/llib-lmp.ln
	${C} -C -Dlint ${S}/llib-lplot | ${LINT1} -uv > ${S}/llib-lplot.ln
	${C} -C -Dlint ${S}/llib-ltermcap | ${LINT1} -uv > ${S}/llib-ltermcap.ln
	${C} -C -Dlint ${S}/llib-lc | ${LINT1} -v > ${S}/llib-lc.ln
	${C} -C -Dlint ${S}/llib-lcurses | ${LINT1} -v > ${S}/llib-lcurses.ln

depend:

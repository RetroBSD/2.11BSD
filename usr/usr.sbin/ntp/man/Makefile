DESTDIR=
MANDIR=/usr/man/cat8
MANSRC= ntp.8 ntpd.8 ntpdc.8
MAN= ntp.0 ntpd.0 ntpdc.0

.SUFFIXES: .8 .0
 
.8.0:
	${DESTDIR}/usr/man/manroff $*.8 > $*.0

all: ${MAN}

install: ${MAN}
	install -c -m 444 -o bin -g bin ntp.0 ${DESTDIR}/${MANDIR}
	install -c -m 444 -o bin -g bin ntpd.0 ${DESTDIR}/${MANDIR}
	install -c -m 444 -o bin -g bin ntpdc.0 ${DESTDIR}/${MANDIR}

clean:
	@rm -f *~ ${MAN}

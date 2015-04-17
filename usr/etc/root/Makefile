#
# Public domain - 1997/1/10 - sms
#
#	@(#)Makefile	1.0 (2.11BSD) 1997/1/10
#
# DESTDIR is defined here to be something which does *NOT* exist - it must be
#  specified on the command line when doing a "make DESTDIR=/mnt distribution".
#  This is aimed at avoiding overwriting the system disk's /etc files.

DESTDIR=/foobar
OWN=root
GRP=wheel

all install depend lint clean tags:

distribution:
	install -c -o ${OWN} -g ${GRP} -m 644 dot.cshrc ${DESTDIR}/.cshrc
	install -c -o ${OWN} -g ${GRP} -m 644 dot.login ${DESTDIR}/.login
	install -c -o ${OWN} -g ${GRP} -m 644 dot.profile ${DESTDIR}/.profile

#
# Public domain - 1997/1/10 - sms
#
#	@(#)Makefile	1.0 (2.11BSD) 1997/1/10
#
# DESTDIR is defined here to be something which does *NOT* exist - it must be
#  specified on the command line when doing a "make DESTDIR=/mnt distribution".
#  This is aimed at avoiding overwriting the system disk's /etc files.

DESTDIR=/foobar

OWN=uucp
GRP=daemon

FILES= L-devices L-dialcodes L.cmds L.sys L_stat L_sub SEQF USERFILE \
	clean.daily clean.weekly uucp-map

all install depend lint clean tags:

distribution:
	-rm -rf ${DESTDIR}/etc/uucp
	mkdir ${DESTDIR}/etc/uucp
	chown uucp.daemon ${DESTDIR}/etc/uucp
	chmod 755 ${DESTDIR}/etc/uucp
	install -c -o ${OWN} -g ${GRP} -m 644 ${FILES} ${DESTDIR}/etc/uucp
	chmod 600 ${DESTDIR}/etc/uucp/L.sys
	chmod 640 ${DESTDIR}/etc/uucp/L_stat ${DESTDIR}/etc/uucp/L_sub
	chmod 640 ${DESTDIR}/etc/uucp/SEQF
	chmod 755 ${DESTDIR}/etc/uucp/clean.*

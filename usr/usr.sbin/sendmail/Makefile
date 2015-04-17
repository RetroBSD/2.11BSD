#
#	Makefile for sendmail base directory
#
#		@(#)Makefile	4.9.2	(2.11BSD) 1997/9/22
#

ALL=	src/sendmail
DIRS=	$(DESTDIR)/usr/spool/mqueue
#
ALLDIRS=include adm src aux md lib doc adb test
SENDMAIL=$(DESTDIR)/usr/sbin/sendmail
SYSLOG=	/usr/spool/mqueue/syslog
HELPFILE=$(DESTDIR)/usr/share/misc/sendmail.hf
STATUSFILE=$(DESTDIR)/var/log/sendmail.st
STRINGFILE=${DESTDIR}/usr/share/misc/sendmail.sr
FROZENFILE=${DESTDIR}/etc/sendmail.fc

# When hardlinks are used, the references to LN below will need changes
# for DESTDIR != /
LN=	ln -s

SEPFLAG= -i

all:
	cd lib; make ${MFLAGS} SEPFLAG=${SEPFLAG}
	cd src; m4 Makefile.m4 > Makefile; make ${MFLAGS} SEPFLAG=${SEPFLAG} all
	-cd aux; m4 Makefile.m4 > Makefile; make ${MFLAGS} SEPFLAG=${SEPFLAG}

install: $(ALL) $(DIRS)
	install -s -o root -m 4755	src/sendmail	$(SENDMAIL)
	install -c -o root -m 644	src/sendmail.sr	$(STRINGFILE)
	install -c -o root -m 644	lib/sendmail.hf	$(HELPFILE)
	install -c -o root -m 666	/dev/null	$(STATUSFILE)
	install -c -o root -m 644	/dev/null	$(FROZENFILE)
	rm -f					$(DESTDIR)/usr/bin/newaliases
	$(LN)			../sbin/sendmail $(DESTDIR)/usr/bin/newaliases
	rm -f					$(DESTDIR)/usr/bin/mailq
	$(LN)			../sbin/sendmail  $(DESTDIR)/usr/bin/mailq

src/sendmail:
	cd src; make ${MFLAGS} sendmail

aux/syslog:
	-cd aux; make ${MFLAGS} syslog

$(DESTDIR)/usr/spool/mqueue:
	-mkdir			$(DESTDIR)/usr/spool/mqueue
	-chmod	777		$(DESTDIR)/usr/spool/mqueue

clean:
	-cd src; make ${MFLAGS} clean
	-cd cf;  make ${MFLAGS} clean
	-cd aux; make ${MFLAGS} clean
	-cd lib; make ${MFLAGS} clean
	-cd doc; make ${MFLAGS} clean

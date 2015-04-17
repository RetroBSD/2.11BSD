#
# Public domain - 1996/10/24 - sms
#
#	@(#)Makefile	1.2 (2.11BSD) 1997/6/27
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

SUBDIR=	ac accton arff arp bad144 catman chown chroot config cron dev_mkdb \
	diskpart edquota flcopy gettable htable implog implogd inetd kgmon \
	lpr makewhatis mkhosts mklost+found mkproto named ntp pstat quot \
	quotaon repquota rmt rwhod rxformat sa sendmail sysgen syslogd timed \
	traceroute trpt update vipw

# This is broken and doesn't compile - thus it is not included in SUBDIR above.
BROKEN=	trsp

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} SEPFLAG=${SEPFLAG} all

FRC:

install: FRC
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

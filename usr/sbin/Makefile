#
# Public domain - 1996/10/26 - sms
#
#	@(#)Makefile	1.3 (2.11BSD) 1999/7/21
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i

SUBDIR= XNSrouted badsect clri dcheck dmesg dump dumpfs fsck gt307 icheck \
	ifconfig init mkfs mknod mkpasswd mount ncheck newfs \
	quotacheck reboot restor route routed savecore \
	shutdown slattach swapon testnet tunefs umount

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} SEPFLAG=${SEPFLAG} all

FRC:

install: FRC
	-for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done

clean:
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

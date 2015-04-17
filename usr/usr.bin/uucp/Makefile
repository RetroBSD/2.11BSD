#	Makefile	5.11.3 (2.11BSD GTE)	1996/11/17
#
# LIBS can be set to reference other object modules and libraries needed
# by uucico, such as -lunet (needed when UNET is defined).
#
# Common object files are assembled into a library {uulib.a}
#
ALL=	uucico uucp uux uuxqt uulog uuclean uuname uupoll uusnap \
	uuencode uudecode uusend uucpd uuq acucntrl
# Common object files placed in uulib.a
COMMON=	anlwrk.o anyread.o assert.o cfgets.o chkpth.o \
	cpmv.o expfile.o gename.o getargs.o getprm.o \
	getpwinfo.o gnamef.o gnxseq.o lastpart.o logent.o mailst.o \
	prefix.o setline.o subdir.o sysacct.o systat.o \
	ulockf.o uucpname.o versys.o xqt.o uucpdefs.o
SRCS=	${USRCS} ${OSRCS} ${LSRCS}
USRCS=	cico.c cntrl.c conn.c condevs.c gnsys.c pk0.c chksum.c pk1.c tio.c \
	fio.c gio.c imsg.c 
OSRCS=	uuclean.c uucp.c uulog.c uuname.c uupoll.c \
	uusend.c uusnap.c uux.c uuxqt.c uuq.c acucntrl.c
LSRCS=	anlwrk.c anyread.c assert.c cfgets.c chkpth.c \
	cpmv.c expfile.c \
	gename.c getargs.c getprm.c getpwinfo.c \
	gnamef.c gnxseq.c \
	lastpart.c logent.c mailst.c prefix.c \
	setline.c subdir.c sysacct.c systat.c ulockf.c \
	uucpdefs.c uucpname.c versys.c xqt.c
# Objects extracted from libc.a when building an overlaid uucico
LIBOBJS= gethnamadr.o getsent.o getsbyname.o res_init.o res_send.o \
	res_query.o res_mkquery.o res_comp.o

DESTDIR=

OWNER=	uucp
GROUP=	daemon
CFLAGS=	-O
LDFLAGS=
SEPFLAG= -i
LINTFLAGS= -chba
LIBS= 
SBINDIR=	${DESTDIR}/usr/sbin
LIBEXECDIR=	${DESTDIR}/usr/libexec
BIN=	${DESTDIR}/usr/bin
PUBDIR=	${DESTDIR}/usr/spool/uucppublic
SPOOL=	${DESTDIR}/usr/spool/uucp
XQTDIR=	${SPOOL}/XTMP
CORRUPT= ${SPOOL}/CORRUPT
AUDIT=	${SPOOL}/AUDIT
LCK= ${SPOOL}/LCK
LOG= ${SPOOL}/LOG
STST=	${SPOOL}/STST
# The six subdirs {not counting XTMP}.
HOSTNAME=`uuname -l | sed 's/\(.......\).*/\1/'`
SUBDIRS=C. D.${HOSTNAME}X D.${HOSTNAME} D. X. TM.

all:	${ALL} 

lint: ${SRCS}
	lint ${LINTFLAGS} ${USRCS} ${LSRCS}
	-for i in ${OSRCS} ; do \
		lint ${LINTFLAGS} $$i ${LSRCS} ;\
	done

print:	$(SRCS)
	vgrind $?
	vgrind -x index
	touch print

uucp:	uucp.o uulib.a
	${CC} -o uucp ${SEPFLAG} ${LDFLAGS} uucp.o uulib.a ${LIBS}

uux:	uux.o uulib.a
	${CC} -o uux ${SEPFLAG} ${LDFLAGS} uux.o uulib.a ${LIBS}

uuxqt:	uuxqt.o uulib.a
	${CC} -o uuxqt ${SEPFLAG} ${LDFLAGS} uuxqt.o uulib.a ${LIBS}

uucico:	cico.o cntrl.o conn.o condevs.o gnsys.o pk0.o chksum.o pk1.o tio.o \
	fio.o gio.o imsg.o uulib.a aculib/aculib.a
	-if egrep -s "^#define[ 	]BSDTCP" uucp.h; then \
	   ar x /lib/libc.a ${LIBOBJS}; \
	   ${CC} -o uucico ${SEPFLAG} ${LDFLAGS} cico.o cntrl.o condevs.o \
	            gnsys.o pk0.o chksum.o pk1.o tio.o fio.o gio.o imsg.o \
	            uulib.a aculib/aculib.a \
		    -Z ${LIBOBJS} \
		    -Z conn.o \
		    -Y ${LIBS} ; \
	else \
	 ${CC} -o uucico ${SEPFLAG} ${LDFLAGS} cico.o cntrl.o conn.o condevs.o \
	 gnsys.o pk0.o chksum.o pk1.o tio.o fio.o gio.o imsg.o \
	 uulib.a aculib/aculib.a ${LIBS} ; \
	fi

uulog:	uulog.o uulib.a
	${CC} -o uulog ${SEPFLAG} ${LDFLAGS} uulog.o uulib.a ${LIBS}

uuclean: uuclean.o uulib.a
	${CC} -o uuclean ${SEPFLAG} ${LDFLAGS} uuclean.o uulib.a ${LIBS}

uuname:	uuname.o uulib.a
	${CC} -o uuname ${SEPFLAG} ${LDFLAGS} uuname.o uulib.a ${LIBS}

uupoll:	uupoll.o uulib.a
	${CC} -o uupoll ${SEPFLAG} ${LDFLAGS} uupoll.o uulib.a ${LIBS}

uusnap:	uusnap.o
	${CC} -o uusnap ${SEPFLAG} ${LDFLAGS} uusnap.o ${LIBS}

uuencode: uuencode.o
	${CC} -o uuencode ${SEPFLAG} ${LDFLAGS} uuencode.o ${LIBS}

uudecode: uudecode.o
	${CC} -o uudecode ${SEPFLAG} ${LDFLAGS} uudecode.o ${LIBS}

uusend: uusend.o
	${CC} -o uusend ${SEPFLAG} ${LDFLAGS} uusend.o uulib.a ${LIBS}

uucpd: uucpd.o uucp.h
	${CC} -o uucpd ${SEPFLAG} ${LDFLAGS} uucpd.o ${LIBS}

uuq: uuq.o uulib.a
	${CC} -o uuq ${SEPFLAG} ${LDFLAGS} uuq.o uulib.a ${LIBS}

acucntrl: acucntrl.o
	${CC} -o acucntrl ${SEPFLAG} ${LDFLAGS} acucntrl.o ${LIBS}

mkdirs:
	-for i in ${SPOOL} ${PUBDIR} ${XQTDIR} ${CORRUPT} \
		${AUDIT} ${LCK} ${LOG} ${STST} ; do \
		rm -f $$i ; mkdir $$i ; \
		chmod 755 $$i; chown ${OWNER} $$i; chgrp ${GROUP} $$i ; \
	done
	chmod 777 ${PUBDIR} ${LCK}
	-cd ${LOG}; for i in uucico uux uuxqt uucp xferstats; do\
		rm -f $$i ; mkdir $$i ; \
		chmod 755 $$i; chown ${OWNER} $$i; chgrp ${GROUP} $$i ; \
	done
	-cd ${SPOOL}; for i in ${SUBDIRS} ; do \
		mkdir $$i; \
		chown ${OWNER} $$i; chgrp ${GROUP} $$i; chmod 755 $$i; \
	done

uulib.a: ${COMMON}
	rm -f $@
	ar cr $@ ${COMMON}
	ranlib $@

aculib/aculib.a:
	cd aculib; make ${MFLAGS} "CFLAGS=${CFLAGS}"

install: mkdirs ${ALL}
	for i in uucico uuclean; do \
		install -s -m 6111 -o ${OWNER} -g ${GROUP} $$i ${SBINDIR} ;\
	done
	for i in uucp uux uulog uuname uusnap uupoll uuq uusend; do \
		install -s -m 6111 -o ${OWNER} -g ${GROUP} $$i ${BIN} ;\
	done
	rm -f ${BIN}/ruusend
	ln ${BIN}/uusend ${BIN}/ruusend
	install -s -m 6110 -o ${OWNER} -g ${GROUP} uuxqt ${LIBEXECDIR}
	install -s -m 6110 -o root -g ${GROUP} acucntrl ${LIBEXECDIR}
	install -s -m 500 -o uucp uucpd ${LIBEXECDIR}
	@echo Dont forget to fork off /usr/libexec/uucpd in /etc/rc.local
	@echo if you are not running 'inetd'
	install -s uuencode ${BIN}
	install -s uudecode ${BIN}

clean:
	rm -f *.o ${ALL} uulib.a
	cd aculib; make ${MFLAGS} clean

depend:
	mkdep ${CFLAGS} *.c

#
# Steven Schultz - put in the public domain 1996/6/25.
#
#	@(#)Makefile	1.6 (2.11BSD) 1999/5/7
#
DESTDIR=
CFLAGS=	-O
SEPFLAG= -i
TAGSFILE=tags

SUBDIR=	acctd comsat ctimed fingerd ftpd getNAME getty identd makekey \
	popper rexecd rlogind rshd talkd tcpd telnetd tftpd

all:	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make SEPFLAG=${SEPFLAG} ${MFLAGS}

FRC:
	
install:
	-for i in ${SUBDIR}; do \
		(cd $$i; make SEPFLAG=${SEPFLAG} ${MFLAGS} \
			DESTDIR=${DESTDIR} install); done

tags:
	cd libc; make ${MFLAGS} TAGSFILE=../${TAGSFILE} tags

clean:
	rm -f a.out core *.s *.o
	-for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done

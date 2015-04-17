#
#  Sendmail
#  Copyright (c) 1983  Eric P. Allman
#  Berkeley, California
#
#  Copyright (c) 1983 Regents of the University of California.
#  All rights reserved.  The Berkeley software License Agreement
#  specifies the terms and conditions for redistribution.
#
#	@(#)Makefile.m4	5.10.6 (2.11BSD GTE) 1996/10/24
#
#
#  SENDMAIL Makefile.
#
#

# Uncomment these two lines to enable string extractions
CC= ./mkstrcc
XSTR=/usr/ucb/xstr
EXTRACT=extract.o

LIBS=	-ldbm
DESTDIR=

OBJS1=	conf.o main.o collect.o parseaddr.o alias.o deliver.o \
	savemail.o err.o readcf.o stab.o headers.o recipient.o \
	stats.o daemon.o usersmtp.o srvrsmtp.o queue.o \
	macro.o util.o clock.o trace.o envelope.o
OBJS2=	sysexits.o arpadate.o convtime.o
OBJS=	$(OBJS1) $(OBJS2) $(EXTRACT) Version.o str.o

SBASE=	conf.o parseaddr.o alias.o deliver.o headers.o \
	recipient.o srvrsmtp.o queue.o util.o \
	envelope.o sysexits.o convtime.o Version.o \
	$(EXTRACT) str.o
SOV1=	main.o readcf.o macro.o
SOV2=	daemon.o savemail.o usersmtp.o err.o clock.o stats.o trace.o stab.o \
	arpadate.o
SOV3=	collect.o

SRCS=	\
	conf.c deliver.c main.c parseaddr.c err.c alias.c savemail.c \
	sysexits.c util.c arpadate.c collect.c \
	macro.c headers.c readcf.c stab.c recipient.c stats.c daemon.c \
	usersmtp.c srvrsmtp.c queue.c clock.c trace.c envelope.c \
	convtime.c Version.c

ALL=	sendmail

O=	-O
COPTS=
CCONFIG=-I../include -DVMUNIX -DNOSCCS
CFLAGS=	$O $(COPTS) $(CCONFIG)
SEPFLAG=-i
LINT=	lint
XREF=	ctags -x
INSTALL=install -c -s -o root

ROOT=	root
OBJMODE=755

.c.o:
	$(CC) ${CFLAGS} -c $*.c

all: $(ALL)

sendmail: $(OBJS)
	ld $(SEPFLAG) $(COPTS) /lib/crt0.o -o sendmail \
		-Z $(SOV1) -Z $(SOV2) -Z $(SOV3) -Y $(SBASE) $(LIBS) -lstubs -lc
	chmod $(OBJMODE) sendmail
	size sendmail; ls -l sendmail

install: all
	$(INSTALL) -m 4755 -o root sendmail $(DESTDIR)/usr/sbin
	cp /dev/null $(DESTDIR)/etc/sendmail.fc
	-if [ -s sendmail.sr ]; then \
		install -c -o bin -m 644 sendmail.sr \
			$(DESTDIR)/usr/share/misc/sendmail.sr; \
	fi

tags: FRC
	ctags -a -t ${SRCS}

FRC:

$(OBJS1): sendmail.h
$(OBJS): conf.h
stats.o: mailstats.h

sendmail.h util.o: ../include/useful.h

#
#  Auxiliary support entries
#

clean:
	rm -f sendmail a.out XREF sendmail.cf
	rm -f sendmail.sr *.o strings version.c xs.c

print: $(SRCS)
	@ls -l | pr -h "sendmail directory"
	@$(XREF) *.c | pr -h "cross reference listing"
	@size *.o | pr -h "object code sizes"
	@pr Makefile *.m4 *.h *.[cs]

lint:
	$(LINT) $(CCONFIG) $(SRCS)

str.o:	strings
	${XSTR}
	cc -c xs.c
	mv xs.o str.o
	rm -f xs.c

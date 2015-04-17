# Compile time flags:
#	BSD4_2		- We're a 4.2 BSD UNIX (or later, includes Ultrix)
#				4.2BSD accounting
#	USG		- We're a USG (System III/V) UNIX
#				unreliable signals, strchr, missing bcopy,
#				might have lockf(2)
#	USELOG="file"	- Log transmission stats to this file
#	DECNET		- We're an Ultrix system with DECNET
#				(this implies BSD4_2)
#	EXCELAN		- We're a USG system with an EXCELAN ethernet board
#				to fake real networking with.
#
# these two are implied by EXCELAN:
#	NONETDB		- We don't have gethostbyname(3) and getservbyname(3)
#				and inet_ntoa(3) and inet_addr(3)
#	OLDSOCKET	- We use the old (pre 4.2 BSD) socket(2) args
#
# these are implied by BSD4_2:
#	SYSLOG		- We have syslog(3)
#	FTRUNCATE	- We have ftruncate(2)
#	RELSIG		- We have 4BSD's reliable signals
#				(although I use the signal(2) interface)

DEFS= -DBSD4_2
# -lresolv needed below for pyramids
LIBS=
CFLAGS= -O ${DEFS}
DESTDIR=/usr/lib/news
HFILES=nntpxmit.h get_tcp_conn.h llist.h sysexits.h nntp.h
SRC=nntpxmit.c remote.c llist.c get_tcp_conn.c
OBJ=nntpxmit.o remote.o llist.o get_tcp_conn.o
FILES=Makefile nntpxmit.1 ${SRC} ${HFILES} shlock.c nntpsend.csh nntpsend.sh nntp_awk

all: nntpxmit shlock

nntpxmit: ${OBJ}
	cc -o $@ ${OBJ} ${LIBS}
	@ls -las $@ ; size $@

nntpxmit.o: nntpxmit.c nntpxmit.h llist.h

get_tcp_conn.o: get_tcp_conn.c get_tcp_conn.h

remote.o: remote.c nntpxmit.h get_tcp_conn.h

llist.o: llist.c llist.h

shlock: shlock.c
	cc ${CFLAGS} -o shlock shlock.c

install: nntpxmit shlock
	cp nntpxmit shlock ${DESTDIR}
	cd ${DESTDIR}; chmod 755 nntpxmit shlock
	@echo "You'll have to edit nntpsend for local config."

lint:
	lint -hbx ${DEFS} ${SRC} ${LIBS}

clean:
	rm -f nntpxmit shlock ${OBJ}

dist: nntpxmit.tar

nntpxmit.tar: ${FILES}
	tar cvhf nntpxmit.tar ${FILES}

# two for Phil Lapsley
distrib: clean

check:;


#
# Makefile for NNTP server
#

SRVROBJ = main.o serve.o access.o access_inet.o access_dnet.o active.o \
	ahbs.o globals.o group.o help.o ihave.o list.o misc.o netaux.o \
	newgroups.o newnews.o nextlast.o ngmatch.o post.o parsit.o scandir.o \
	slave.o spawn.o strcasecmp.o subnet.o time.o xhdr.o fakesyslog.o \
	../common/version.o

SRVRSRC = main.c serve.c access.c access_inet.c access_dnet.c active.c \
	ahbs.c globals.c group.c help.c ihave.c list.c misc.c netaux.c \
	newgroups.c newnews.c nextlast.c ngmatch.c post.c parsit.c scandir.c \
	slave.c spawn.c strcasecmp.c subnet.c time.c xhdr.c fakesyslog.c \
	../common/version.c

SRVRINC = common.h ../common/conf.h ../common/nntp.h

SRCS	= ${SRVRSRC}

# -ldbm here if you've #define'ed DBM in ../common/conf.h
LIBS	=

CFLAGS	= -O

# Where nntpd is going to live

DESTDIR	= /news/lib

all:	nntpd

nntpd: ${SRVROBJ} ${SRVRINC}
	${CC} ${CFLAGS} -o nntpd ${SRVROBJ} ${LIBS}

${SRVROBJ}: ${SRVRINC}

install: nntpd
	cp nntpd ${DESTDIR}/nntpd
	chmod 711 ${DESTDIR}/nntpd

lint:
	lint ${SRVRSRC}

clean:
	-rm -f *.o nntpd make*.out a.out

distrib: clean
	rm -rf SCCS save tags

check:
	sccs check
	sccs prt -y *.[ch] > /dev/null

tags:	${SRVRSRC} ${SRVRINC}
	ctags ${SRVRSRC} ${SRVRINC}

# Dependencies

../common/version.o:
	${CC} ${CFLAGS} -c ../common/version.c
	mv version.o ../common/version.o

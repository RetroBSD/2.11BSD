#
# Makefile				Makefile for the Pidentd daemon
#
# This program is in the public domain and may be used freely by anyone
# who wants to. 
#
# Last update: 13 December 1996 - 2.11BSD specific version created.
#
# Please send bug fixes/bug reports to: Peter Eriksson <pen@lysator.liu.se>

# Modify it below to suit your particular system, or specify it
# on the command line (like "make DESTROOT=/usr/wheel sunos4")
#
DESTDIR=

BINDIR=$(DESTDIR)/usr/sbin
EXECDIR=$(DESTDIR)/usr/libexec
CONFDIR=$(DESTDIR)/etc
MANDIR=$(DESTDIR)/usr/man/cat8

# Global compile-time and link-time options
#
# Please note the the INCLUDE_CRYPT option needs a DES library. You can
# find a free one on ftp.lysator.liu.se in pub/libraries or on
# ftp.funet.fi in pub/crypt/libdes.
#
# GDEFS=-DINCLUDE_EXTENSIONS -DINCLUDE_PROXY -DINCLUDE_CRYPT \
#	-DSTRONG_LOG -DALLOW_FORMAT \

GDEFS=-DSTRONG_LOG -DDPATH_CONFIG='\"$(CONFDIR)/identd.conf\"' \
	-DPATH_DESKEY='\"$(CONFDIR)/identd.key\"'

# GLIBS=-lident -ldes
GLIBS=

CFLAGS=-O -DNO_KVM
LDFLAGS=-i

all: identd identconn identd.0 idecrypt.0

#
# End of system-type definitions --------------------------------------
#
identd: 
	@(cd src ; make CC="$(CC)" LDFLAGS="$(LDFLAGS)" LIBS="$(GLIBS)" CFLAGS="$(CFLAGS) $(GDEFS)" KSRC=2.11bsd identd)


identd.8: identd.man
	sed -e 's,xDESTDIRx,$(EXECDIR),g' -e 's,xCONFDIRx,$(CONFDIR),g' $? > $@

identd.0: identd.8
	/usr/man/manroff identd.8 > identd.0

idecrypt.0: idecrypt.man
	/usr/man/manroff idecrypt.man > idecrypt.0

identconn: identconn.sh
	sed -e 's,xDESTROOTx,$(DESTROOT),g' $? > $@

install: identd identd.0 identconn itest idecrypt
	install -m 644 identd.0 $(MANDIR)/identd.0
	install -m 755 identd $(EXECDIR)/identd
	install -m 755 identconn $(BINDIR)/identconn
	install -m 644 idecrypt.0  $(MANDIR)/idecrypt.0
	install -m 755 idecrypt $(BINDIR)/idecrypt
	install -o root -g kmem -m 2555 itest ${BINDIR}/itest

tests:
	(cd testdir ; make)

clean:
	-rm -f identd idecrypt itest identconn *.o *.0
	(cd src ; make clean)
	(cd testdir ; make clean)

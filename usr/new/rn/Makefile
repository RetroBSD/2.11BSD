# $Header: Makefile.SH,v 4.3.1.2 85/05/13 17:21:18 lwall Exp $
#
# $Log:	Makefile.SH,v $
# Revision 4.3.1.2  85/05/13  17:21:18  lwall
# System V make needs space before line-continuing backslash.
# 
# Revision 4.3.1.1  85/05/10  11:30:15  lwall
# Branch for patches.
# 
# Revision 4.3  85/05/01  11:33:26  lwall
# Baseline for release with 4.3bsd.
# 

CC = cc
rnbin = /news/bin
rnlib = /news/lib
mansrc = /usr/man/manl
manext = l
NNTPSRC = ../nntp
CFLAGS = -i -O
LDFLAGS = -i
NDIRC = 
NDIRO = 

libs =  -ltermlib 

public = rn newsetup newsgroups Pnews Rnmail
private = norm.saver mbox.saver ng.help art.help pager.help subs.help makedir filexp Pnews.header getactive
manpages = rn.1 Pnews.1 Rnmail.1 newsetup.1 newsgroups.1
util = Makefile makedepend newsnews

h1 = addng.h art.h artio.h artsrch.h backpage.h bits.h cheat.h common.h
h2 = final.h head.h help.h init.h intrp.h kfile.h last.h ndir.h ng.h
h3 = ngdata.h ngsrch.h ngstuff.h only.h rcln.h rcstuff.h
h4 = respond.h rn.h search.h sw.h term.h util.h

h = $(h1) $(h2) $(h3) $(h4)

c1 = addng.c art.c artio.c artsrch.c backpage.c bits.c cheat.c
c2 = final.c head.c help.c init.c intrp.c kfile.c last.c $(NDIRC) ng.c
c3 = ngdata.c ngsrch.c ngstuff.c only.c rcln.c rcstuff.c
c4 = respond.c rn.c search.c sw.c term.c util.c $(NNTPSRC)/common/clientlib.c

c = $(c1) $(c2) $(c3) $(c4)

obj1 = addng.o art.o artio.o artsrch.o backpage.o bits.o cheat.o
obj2 = final.o head.o help.o init.o intrp.o kfile.o last.o $(NDIRO) ng.o
obj3 = ngdata.o ngsrch.o ngstuff.o only.o rcln.o rcstuff.o
obj4 = respond.o rn.o search.o sw.o term.o util.o $(NNTPSRC)/common/clientlib.o

obj = $(obj1) $(obj2) $(obj3) $(obj4)

base = art.o artio.o artsrch.o backpage.o bits.o cheat.o \
	head.o intrp.o ng.o rn.o $(NNTPSRC)/common/clientlib.o
ov1 = addng.o final.o help.o init.o kfile.o last.o $(NDIRO) ngdata.o ngsrch.o \
	ngstuff.o only.o rcln.o rcstuff.o
ov2 = respond.o search.o sw.o term.o util.o

lintflags = -phbvxac

add1 = Makefile.old Pnews Rnmail art.help
add2 = bsd config.h config.sh eunice filexp
add3 = loc makedepend makedir mbox.saver newsetup
add4 = newsgroups newsnews ng.help norm.saver pager.help
add5 = pdp11 rn subs.help usg v7

addedbyconf = $(add1) $(add2) $(add3) $(add4) $(add5)

# grrr
SHELL = /bin/sh

.c.o:
	$(CC) -c $(CFLAGS) $*.c

all: $(public) $(private) $(util)
	touch all

rn: $(obj)
#	$(CC) $(LDFLAGS) $(obj) $(libs) -o rn
	$(CC) $(LDFLAGS) $(base) -Z $(ov1) -Z $(ov2) -Y $(libs) -o rn


getactive: getactive.o $(NNTPSRC)/common/clientlib.o
	$(CC) $(LDFLAGS) getactive.o $(NNTPSRC)/common/clientlib.o -o getactive

# Eek! This is gross.
$(NNTPSRC)/common/clientlib.o:
	$(CC) -c $(CFLAGS) $(NNTPSRC)/common/clientlib.c
	mv clientlib.o $(NNTPSRC)/common/clientlib.o

# if a .h file depends on another .h file...
$(h):
	touch $@

install: rn
# won't work with csh
	export PATH || exit 1
	- mv $(rnbin)/rn $(rnbin)/rn.old
	- ln -s $(rnbin)/rn $(rnbin)/rrn
	- if test `pwd` != $(rnbin); then cp $(public) $(rnbin); fi
	cd $(rnbin); chmod 755 $(public)
	chmod 755 makedir
	- ./makedir `./filexp $(rnlib)`
	- if test `pwd` != `./filexp $(rnlib)`; then cp INIT $(private) `./filexp $(rnlib)`; fi
	cd `./filexp $(rnlib)`; chmod 755 $(private)
	- if test ! -f `./filexp $(rnlib)/newsnews`; then cp newsnews `./filexp $(rnlib)`; fi
	- if test `pwd` != $(mansrc); then \
for page in $(manpages); do \
cp $$page $(mansrc)/`basename $$page .1`.$(manext); \
done; \
fi

clean:
	rm -f *.o rn core getactive

realclean:
	rm -f rn *.o core $(addedbyconf) $(NNTPSRC)/common/clientlib.o

# The following lint has practically everything turned on.  Unfortunately,
# you have to wade through a lot of mumbo jumbo that can't be suppressed.
# If the source file has a /*NOSTRICT*/ somewhere, ignore the lint message
# for that spot.

lint:
	lint $(lintflags) $(defs) $(c) > rn.fuzz

depend:
	makedepend

# AUTOMATICALLY GENERATED MAKE DEPENDENCIES--PUT NOTHING BELOW THIS LINE
# If this runs make out of memory, delete /usr/include lines.
addng.o: /usr/include/ctype.h
addng.o: /usr/include/errno.h
addng.o: /usr/include/fcntl.h
addng.o: /usr/include/sgtty.h
addng.o: /usr/include/signal.h
addng.o: /usr/include/stdio.h
addng.o: /usr/include/sys/ioctl.h
addng.o: /usr/include/sys/stat.h
addng.o: /usr/include/sys/ttychars.h
addng.o: /usr/include/sys/ttydev.h
addng.o: /usr/include/sys/types.h
addng.o: EXTERN.h
addng.o: INTERN.h
addng.o: addng.c
addng.o: addng.h
addng.o: common.h
addng.o: config.h
addng.o: intrp.h
addng.o: last.h
addng.o: ngdata.h
addng.o: only.h
addng.o: rcstuff.h
addng.o: rn.h
addng.o: search.h
addng.o: util.h
art.o: /usr/include/ctype.h
art.o: /usr/include/errno.h
art.o: /usr/include/fcntl.h
art.o: /usr/include/sgtty.h
art.o: /usr/include/signal.h
art.o: /usr/include/stdio.h
art.o: /usr/include/sys/ioctl.h
art.o: /usr/include/sys/stat.h
art.o: /usr/include/sys/ttychars.h
art.o: /usr/include/sys/ttydev.h
art.o: /usr/include/sys/types.h
art.o: EXTERN.h
art.o: INTERN.h
art.o: art.c
art.o: art.h
art.o: artio.h
art.o: artstate.h
art.o: backpage.h
art.o: bits.h
art.o: cheat.h
art.o: common.h
art.o: config.h
art.o: final.h
art.o: head.h
art.o: help.h
art.o: intrp.h
art.o: ng.h
art.o: ngstuff.h
art.o: rcstuff.h
art.o: rn.h
art.o: search.h
art.o: sw.h
art.o: term.h
art.o: util.h
artio.o: ../nntp/common/response_codes.h
artio.o: /usr/include/ctype.h
artio.o: /usr/include/errno.h
artio.o: /usr/include/fcntl.h
artio.o: /usr/include/sgtty.h
artio.o: /usr/include/signal.h
artio.o: /usr/include/stdio.h
artio.o: /usr/include/sys/ioctl.h
artio.o: /usr/include/sys/stat.h
artio.o: /usr/include/sys/ttychars.h
artio.o: /usr/include/sys/ttydev.h
artio.o: /usr/include/sys/types.h
artio.o: EXTERN.h
artio.o: INTERN.h
artio.o: artio.c
artio.o: artio.h
artio.o: common.h
artio.o: config.h
artio.o: server.h
artsrch.o: /usr/include/ctype.h
artsrch.o: /usr/include/errno.h
artsrch.o: /usr/include/fcntl.h
artsrch.o: /usr/include/sgtty.h
artsrch.o: /usr/include/signal.h
artsrch.o: /usr/include/stdio.h
artsrch.o: /usr/include/sys/ioctl.h
artsrch.o: /usr/include/sys/stat.h
artsrch.o: /usr/include/sys/ttychars.h
artsrch.o: /usr/include/sys/ttydev.h
artsrch.o: /usr/include/sys/types.h
artsrch.o: EXTERN.h
artsrch.o: INTERN.h
artsrch.o: artio.h
artsrch.o: artsrch.c
artsrch.o: artsrch.h
artsrch.o: bits.h
artsrch.o: cheat.h
artsrch.o: common.h
artsrch.o: config.h
artsrch.o: final.h
artsrch.o: head.h
artsrch.o: intrp.h
artsrch.o: kfile.h
artsrch.o: ng.h
artsrch.o: search.h
artsrch.o: term.h
artsrch.o: util.h
backpage.o: /usr/include/ctype.h
backpage.o: /usr/include/errno.h
backpage.o: /usr/include/fcntl.h
backpage.o: /usr/include/sgtty.h
backpage.o: /usr/include/signal.h
backpage.o: /usr/include/stdio.h
backpage.o: /usr/include/sys/ioctl.h
backpage.o: /usr/include/sys/stat.h
backpage.o: /usr/include/sys/ttychars.h
backpage.o: /usr/include/sys/ttydev.h
backpage.o: /usr/include/sys/types.h
backpage.o: EXTERN.h
backpage.o: INTERN.h
backpage.o: backpage.c
backpage.o: backpage.h
backpage.o: common.h
backpage.o: config.h
backpage.o: final.h
backpage.o: intrp.h
bits.o: /usr/include/ctype.h
bits.o: /usr/include/errno.h
bits.o: /usr/include/fcntl.h
bits.o: /usr/include/sgtty.h
bits.o: /usr/include/signal.h
bits.o: /usr/include/stdio.h
bits.o: /usr/include/sys/ioctl.h
bits.o: /usr/include/sys/stat.h
bits.o: /usr/include/sys/ttychars.h
bits.o: /usr/include/sys/ttydev.h
bits.o: /usr/include/sys/types.h
bits.o: EXTERN.h
bits.o: INTERN.h
bits.o: artio.h
bits.o: bits.c
bits.o: bits.h
bits.o: cheat.h
bits.o: common.h
bits.o: config.h
bits.o: final.h
bits.o: head.h
bits.o: intrp.h
bits.o: kfile.h
bits.o: ng.h
bits.o: ngdata.h
bits.o: rcln.h
bits.o: rcstuff.h
bits.o: rn.h
bits.o: util.h
cheat.o: /usr/include/ctype.h
cheat.o: /usr/include/errno.h
cheat.o: /usr/include/fcntl.h
cheat.o: /usr/include/sgtty.h
cheat.o: /usr/include/signal.h
cheat.o: /usr/include/stdio.h
cheat.o: /usr/include/sys/ioctl.h
cheat.o: /usr/include/sys/stat.h
cheat.o: /usr/include/sys/ttychars.h
cheat.o: /usr/include/sys/ttydev.h
cheat.o: /usr/include/sys/types.h
cheat.o: EXTERN.h
cheat.o: INTERN.h
cheat.o: artio.h
cheat.o: artsrch.h
cheat.o: bits.h
cheat.o: cheat.c
cheat.o: cheat.h
cheat.o: common.h
cheat.o: config.h
cheat.o: head.h
cheat.o: intrp.h
cheat.o: ng.h
cheat.o: search.h
cheat.o: term.h
extract.o: /usr/include/stdio.h
extract.o: extract.c
final.o: /usr/include/ctype.h
final.o: /usr/include/errno.h
final.o: /usr/include/fcntl.h
final.o: /usr/include/sgtty.h
final.o: /usr/include/signal.h
final.o: /usr/include/stdio.h
final.o: /usr/include/sys/ioctl.h
final.o: /usr/include/sys/stat.h
final.o: /usr/include/sys/ttychars.h
final.o: /usr/include/sys/ttydev.h
final.o: /usr/include/sys/types.h
final.o: EXTERN.h
final.o: INTERN.h
final.o: artio.h
final.o: bits.h
final.o: common.h
final.o: config.h
final.o: final.c
final.o: final.h
final.o: init.h
final.o: last.h
final.o: ng.h
final.o: ngdata.h
final.o: rcstuff.h
final.o: term.h
final.o: util.h
getactive.o: ../nntp/common/response_codes.h
getactive.o: /usr/include/stdio.h
getactive.o: EXTERN.h
getactive.o: config.h
getactive.o: getactive.c
getactive.o: server.h
head.o: ../nntp/common/response_codes.h
head.o: /usr/include/ctype.h
head.o: /usr/include/errno.h
head.o: /usr/include/fcntl.h
head.o: /usr/include/sgtty.h
head.o: /usr/include/signal.h
head.o: /usr/include/stdio.h
head.o: /usr/include/sys/ioctl.h
head.o: /usr/include/sys/stat.h
head.o: /usr/include/sys/ttychars.h
head.o: /usr/include/sys/ttydev.h
head.o: /usr/include/sys/types.h
head.o: EXTERN.h
head.o: INTERN.h
head.o: artio.h
head.o: bits.h
head.o: common.h
head.o: config.h
head.o: head.c
head.o: head.h
head.o: server.h
head.o: util.h
help.o: /usr/include/ctype.h
help.o: /usr/include/errno.h
help.o: /usr/include/fcntl.h
help.o: /usr/include/sgtty.h
help.o: /usr/include/signal.h
help.o: /usr/include/stdio.h
help.o: /usr/include/sys/ioctl.h
help.o: /usr/include/sys/stat.h
help.o: /usr/include/sys/ttychars.h
help.o: /usr/include/sys/ttydev.h
help.o: /usr/include/sys/types.h
help.o: EXTERN.h
help.o: INTERN.h
help.o: common.h
help.o: config.h
help.o: help.c
help.o: help.h
help.o: rn.h
help.o: term.h
init.o: ../nntp/common/response_codes.h
init.o: /usr/include/ctype.h
init.o: /usr/include/errno.h
init.o: /usr/include/fcntl.h
init.o: /usr/include/sgtty.h
init.o: /usr/include/signal.h
init.o: /usr/include/stdio.h
init.o: /usr/include/sys/ioctl.h
init.o: /usr/include/sys/stat.h
init.o: /usr/include/sys/ttychars.h
init.o: /usr/include/sys/ttydev.h
init.o: /usr/include/sys/types.h
init.o: EXTERN.h
init.o: INTERN.h
init.o: addng.h
init.o: art.h
init.o: artio.h
init.o: artsrch.h
init.o: backpage.h
init.o: bits.h
init.o: cheat.h
init.o: common.h
init.o: config.h
init.o: final.h
init.o: head.h
init.o: help.h
init.o: init.c
init.o: init.h
init.o: intrp.h
init.o: kfile.h
init.o: last.h
init.o: ng.h
init.o: ngdata.h
init.o: ngsrch.h
init.o: ngstuff.h
init.o: only.h
init.o: rcln.h
init.o: rcstuff.h
init.o: respond.h
init.o: rn.h
init.o: search.h
init.o: server.h
init.o: sw.h
init.o: term.h
init.o: util.h
intrp.o: /usr/include/ctype.h
intrp.o: /usr/include/errno.h
intrp.o: /usr/include/fcntl.h
intrp.o: /usr/include/sgtty.h
intrp.o: /usr/include/signal.h
intrp.o: /usr/include/stdio.h
intrp.o: /usr/include/sys/ioctl.h
intrp.o: /usr/include/sys/stat.h
intrp.o: /usr/include/sys/ttychars.h
intrp.o: /usr/include/sys/ttydev.h
intrp.o: /usr/include/sys/types.h
intrp.o: EXTERN.h
intrp.o: INTERN.h
intrp.o: artio.h
intrp.o: artsrch.h
intrp.o: bits.h
intrp.o: common.h
intrp.o: config.h
intrp.o: final.h
intrp.o: head.h
intrp.o: intrp.c
intrp.o: intrp.h
intrp.o: ng.h
intrp.o: rcstuff.h
intrp.o: respond.h
intrp.o: rn.h
intrp.o: search.h
intrp.o: term.h
intrp.o: util.h
kfile.o: /usr/include/ctype.h
kfile.o: /usr/include/errno.h
kfile.o: /usr/include/fcntl.h
kfile.o: /usr/include/sgtty.h
kfile.o: /usr/include/signal.h
kfile.o: /usr/include/stdio.h
kfile.o: /usr/include/sys/ioctl.h
kfile.o: /usr/include/sys/stat.h
kfile.o: /usr/include/sys/ttychars.h
kfile.o: /usr/include/sys/ttydev.h
kfile.o: /usr/include/sys/types.h
kfile.o: EXTERN.h
kfile.o: INTERN.h
kfile.o: artsrch.h
kfile.o: bits.h
kfile.o: common.h
kfile.o: config.h
kfile.o: intrp.h
kfile.o: kfile.c
kfile.o: kfile.h
kfile.o: ng.h
kfile.o: ngstuff.h
kfile.o: rcstuff.h
kfile.o: rn.h
kfile.o: search.h
kfile.o: term.h
kfile.o: util.h
kitlists.o: /usr/include/stdio.h
kitlists.o: kitlists.c
last.o: /usr/include/ctype.h
last.o: /usr/include/errno.h
last.o: /usr/include/fcntl.h
last.o: /usr/include/sgtty.h
last.o: /usr/include/signal.h
last.o: /usr/include/stdio.h
last.o: /usr/include/sys/ioctl.h
last.o: /usr/include/sys/stat.h
last.o: /usr/include/sys/ttychars.h
last.o: /usr/include/sys/ttydev.h
last.o: /usr/include/sys/types.h
last.o: EXTERN.h
last.o: INTERN.h
last.o: common.h
last.o: config.h
last.o: intrp.h
last.o: last.c
last.o: last.h
last.o: rn.h
last.o: util.h
ndir.o: /usr/include/ctype.h
ndir.o: /usr/include/errno.h
ndir.o: /usr/include/fcntl.h
ndir.o: /usr/include/sgtty.h
ndir.o: /usr/include/signal.h
ndir.o: /usr/include/stdio.h
ndir.o: /usr/include/sys/dir.h
ndir.o: /usr/include/sys/ioctl.h
ndir.o: /usr/include/sys/stat.h
ndir.o: /usr/include/sys/ttychars.h
ndir.o: /usr/include/sys/ttydev.h
ndir.o: /usr/include/sys/types.h
ndir.o: EXTERN.h
ndir.o: INTERN.h
ndir.o: common.h
ndir.o: config.h
ndir.o: ndir.c
ndir.o: ndir.h
ng.o: ../nntp/common/response_codes.h
ng.o: /usr/include/ctype.h
ng.o: /usr/include/errno.h
ng.o: /usr/include/fcntl.h
ng.o: /usr/include/sgtty.h
ng.o: /usr/include/signal.h
ng.o: /usr/include/stdio.h
ng.o: /usr/include/sys/ioctl.h
ng.o: /usr/include/sys/stat.h
ng.o: /usr/include/sys/ttychars.h
ng.o: /usr/include/sys/ttydev.h
ng.o: /usr/include/sys/types.h
ng.o: EXTERN.h
ng.o: INTERN.h
ng.o: art.h
ng.o: artio.h
ng.o: artsrch.h
ng.o: artstate.h
ng.o: backpage.h
ng.o: bits.h
ng.o: cheat.h
ng.o: common.h
ng.o: config.h
ng.o: final.h
ng.o: head.h
ng.o: help.h
ng.o: intrp.h
ng.o: kfile.h
ng.o: last.h
ng.o: ng.c
ng.o: ng.h
ng.o: ngdata.h
ng.o: ngstuff.h
ng.o: rcln.h
ng.o: rcstuff.h
ng.o: respond.h
ng.o: rn.h
ng.o: search.h
ng.o: server.h
ng.o: term.h
ng.o: util.h
ngdata.o: ../nntp/common/response_codes.h
ngdata.o: /usr/include/ctype.h
ngdata.o: /usr/include/errno.h
ngdata.o: /usr/include/fcntl.h
ngdata.o: /usr/include/sgtty.h
ngdata.o: /usr/include/signal.h
ngdata.o: /usr/include/stdio.h
ngdata.o: /usr/include/sys/dir.h
ngdata.o: /usr/include/sys/ioctl.h
ngdata.o: /usr/include/sys/stat.h
ngdata.o: /usr/include/sys/ttychars.h
ngdata.o: /usr/include/sys/ttydev.h
ngdata.o: /usr/include/sys/types.h
ngdata.o: EXTERN.h
ngdata.o: INTERN.h
ngdata.o: common.h
ngdata.o: config.h
ngdata.o: final.h
ngdata.o: intrp.h
ngdata.o: ndir.h
ngdata.o: ngdata.c
ngdata.o: ngdata.h
ngdata.o: rcln.h
ngdata.o: rcstuff.h
ngdata.o: rn.h
ngdata.o: server.h
ngsrch.o: /usr/include/ctype.h
ngsrch.o: /usr/include/errno.h
ngsrch.o: /usr/include/fcntl.h
ngsrch.o: /usr/include/sgtty.h
ngsrch.o: /usr/include/signal.h
ngsrch.o: /usr/include/stdio.h
ngsrch.o: /usr/include/sys/ioctl.h
ngsrch.o: /usr/include/sys/stat.h
ngsrch.o: /usr/include/sys/ttychars.h
ngsrch.o: /usr/include/sys/ttydev.h
ngsrch.o: /usr/include/sys/types.h
ngsrch.o: EXTERN.h
ngsrch.o: INTERN.h
ngsrch.o: common.h
ngsrch.o: config.h
ngsrch.o: final.h
ngsrch.o: ngsrch.c
ngsrch.o: ngsrch.h
ngsrch.o: rcln.h
ngsrch.o: rcstuff.h
ngsrch.o: rn.h
ngsrch.o: search.h
ngsrch.o: term.h
ngsrch.o: util.h
ngstuff.o: /usr/include/ctype.h
ngstuff.o: /usr/include/errno.h
ngstuff.o: /usr/include/fcntl.h
ngstuff.o: /usr/include/sgtty.h
ngstuff.o: /usr/include/signal.h
ngstuff.o: /usr/include/stdio.h
ngstuff.o: /usr/include/sys/ioctl.h
ngstuff.o: /usr/include/sys/stat.h
ngstuff.o: /usr/include/sys/ttychars.h
ngstuff.o: /usr/include/sys/ttydev.h
ngstuff.o: /usr/include/sys/types.h
ngstuff.o: EXTERN.h
ngstuff.o: INTERN.h
ngstuff.o: bits.h
ngstuff.o: cheat.h
ngstuff.o: common.h
ngstuff.o: config.h
ngstuff.o: final.h
ngstuff.o: head.h
ngstuff.o: intrp.h
ngstuff.o: ng.h
ngstuff.o: ngstuff.c
ngstuff.o: ngstuff.h
ngstuff.o: sw.h
ngstuff.o: term.h
ngstuff.o: util.h
only.o: /usr/include/ctype.h
only.o: /usr/include/errno.h
only.o: /usr/include/fcntl.h
only.o: /usr/include/sgtty.h
only.o: /usr/include/signal.h
only.o: /usr/include/stdio.h
only.o: /usr/include/sys/ioctl.h
only.o: /usr/include/sys/stat.h
only.o: /usr/include/sys/ttychars.h
only.o: /usr/include/sys/ttydev.h
only.o: /usr/include/sys/types.h
only.o: EXTERN.h
only.o: INTERN.h
only.o: common.h
only.o: config.h
only.o: final.h
only.o: ngsrch.h
only.o: only.c
only.o: only.h
only.o: search.h
only.o: util.h
rcln.o: /usr/include/ctype.h
rcln.o: /usr/include/errno.h
rcln.o: /usr/include/fcntl.h
rcln.o: /usr/include/sgtty.h
rcln.o: /usr/include/signal.h
rcln.o: /usr/include/stdio.h
rcln.o: /usr/include/sys/ioctl.h
rcln.o: /usr/include/sys/stat.h
rcln.o: /usr/include/sys/ttychars.h
rcln.o: /usr/include/sys/ttydev.h
rcln.o: /usr/include/sys/types.h
rcln.o: EXTERN.h
rcln.o: INTERN.h
rcln.o: common.h
rcln.o: config.h
rcln.o: ngdata.h
rcln.o: rcln.c
rcln.o: rcln.h
rcln.o: rcstuff.h
rcln.o: util.h
rcstuff.o: ../nntp/common/response_codes.h
rcstuff.o: /usr/include/ctype.h
rcstuff.o: /usr/include/errno.h
rcstuff.o: /usr/include/fcntl.h
rcstuff.o: /usr/include/sgtty.h
rcstuff.o: /usr/include/signal.h
rcstuff.o: /usr/include/stdio.h
rcstuff.o: /usr/include/sys/ioctl.h
rcstuff.o: /usr/include/sys/stat.h
rcstuff.o: /usr/include/sys/ttychars.h
rcstuff.o: /usr/include/sys/ttydev.h
rcstuff.o: /usr/include/sys/types.h
rcstuff.o: EXTERN.h
rcstuff.o: INTERN.h
rcstuff.o: common.h
rcstuff.o: config.h
rcstuff.o: final.h
rcstuff.o: intrp.h
rcstuff.o: ngdata.h
rcstuff.o: only.h
rcstuff.o: rcln.h
rcstuff.o: rcstuff.c
rcstuff.o: rcstuff.h
rcstuff.o: rn.h
rcstuff.o: search.h
rcstuff.o: server.h
rcstuff.o: term.h
rcstuff.o: util.h
respond.o: /usr/include/ctype.h
respond.o: /usr/include/errno.h
respond.o: /usr/include/fcntl.h
respond.o: /usr/include/sgtty.h
respond.o: /usr/include/signal.h
respond.o: /usr/include/stdio.h
respond.o: /usr/include/sys/ioctl.h
respond.o: /usr/include/sys/stat.h
respond.o: /usr/include/sys/ttychars.h
respond.o: /usr/include/sys/ttydev.h
respond.o: /usr/include/sys/types.h
respond.o: EXTERN.h
respond.o: INTERN.h
respond.o: artio.h
respond.o: common.h
respond.o: config.h
respond.o: final.h
respond.o: head.h
respond.o: intrp.h
respond.o: ng.h
respond.o: respond.c
respond.o: respond.h
respond.o: rn.h
respond.o: term.h
respond.o: util.h
rn.o: /usr/include/ctype.h
rn.o: /usr/include/errno.h
rn.o: /usr/include/fcntl.h
rn.o: /usr/include/sgtty.h
rn.o: /usr/include/signal.h
rn.o: /usr/include/stdio.h
rn.o: /usr/include/sys/ioctl.h
rn.o: /usr/include/sys/stat.h
rn.o: /usr/include/sys/ttychars.h
rn.o: /usr/include/sys/ttydev.h
rn.o: /usr/include/sys/types.h
rn.o: EXTERN.h
rn.o: INTERN.h
rn.o: addng.h
rn.o: common.h
rn.o: config.h
rn.o: final.h
rn.o: help.h
rn.o: init.h
rn.o: intrp.h
rn.o: last.h
rn.o: ng.h
rn.o: ngdata.h
rn.o: ngsrch.h
rn.o: only.h
rn.o: rcln.h
rn.o: rcstuff.h
rn.o: rn.c
rn.o: rn.h
rn.o: search.h
rn.o: sw.h
rn.o: term.h
rn.o: util.h
search.o: /usr/include/ctype.h
search.o: /usr/include/errno.h
search.o: /usr/include/fcntl.h
search.o: /usr/include/sgtty.h
search.o: /usr/include/signal.h
search.o: /usr/include/stdio.h
search.o: /usr/include/sys/ioctl.h
search.o: /usr/include/sys/stat.h
search.o: /usr/include/sys/ttychars.h
search.o: /usr/include/sys/ttydev.h
search.o: /usr/include/sys/types.h
search.o: EXTERN.h
search.o: INTERN.h
search.o: common.h
search.o: config.h
search.o: search.c
search.o: search.h
search.o: util.h
sw.o: /usr/include/ctype.h
sw.o: /usr/include/errno.h
sw.o: /usr/include/fcntl.h
sw.o: /usr/include/sgtty.h
sw.o: /usr/include/signal.h
sw.o: /usr/include/stdio.h
sw.o: /usr/include/sys/ioctl.h
sw.o: /usr/include/sys/stat.h
sw.o: /usr/include/sys/ttychars.h
sw.o: /usr/include/sys/ttydev.h
sw.o: /usr/include/sys/types.h
sw.o: EXTERN.h
sw.o: INTERN.h
sw.o: common.h
sw.o: config.h
sw.o: head.h
sw.o: intrp.h
sw.o: ng.h
sw.o: only.h
sw.o: search.h
sw.o: sw.c
sw.o: sw.h
sw.o: term.h
sw.o: util.h
term.o: /usr/include/ctype.h
term.o: /usr/include/errno.h
term.o: /usr/include/fcntl.h
term.o: /usr/include/sgtty.h
term.o: /usr/include/signal.h
term.o: /usr/include/stdio.h
term.o: /usr/include/sys/ioctl.h
term.o: /usr/include/sys/stat.h
term.o: /usr/include/sys/ttychars.h
term.o: /usr/include/sys/ttydev.h
term.o: /usr/include/sys/types.h
term.o: EXTERN.h
term.o: INTERN.h
term.o: cheat.h
term.o: common.h
term.o: config.h
term.o: final.h
term.o: help.h
term.o: intrp.h
term.o: term.c
term.o: term.h
term.o: util.h
util.o: /usr/include/ctype.h
util.o: /usr/include/errno.h
util.o: /usr/include/fcntl.h
util.o: /usr/include/sgtty.h
util.o: /usr/include/signal.h
util.o: /usr/include/stdio.h
util.o: /usr/include/sys/dir.h
util.o: /usr/include/sys/ioctl.h
util.o: /usr/include/sys/stat.h
util.o: /usr/include/sys/ttychars.h
util.o: /usr/include/sys/ttydev.h
util.o: /usr/include/sys/types.h
util.o: EXTERN.h
util.o: INTERN.h
util.o: common.h
util.o: config.h
util.o: final.h
util.o: ndir.h
util.o: util.c
util.o: util.h
Makefile: Makefile.SH config.sh ; /bin/sh Makefile.SH
Pnews: Pnews.SH config.sh ; /bin/sh Pnews.SH
Rnmail: Rnmail.SH config.sh ; /bin/sh Rnmail.SH
art.help: art.help.SH config.sh ; /bin/sh art.help.SH
makedepend: makedepend.SH config.sh ; /bin/sh makedepend.SH
makedir: makedir.SH config.sh ; /bin/sh makedir.SH
mbox.saver: mbox.saver.SH config.sh ; /bin/sh mbox.saver.SH
newsetup: newsetup.SH config.sh ; /bin/sh newsetup.SH
newsgroups: newsgroups.SH config.sh ; /bin/sh newsgroups.SH
newsnews: newsnews.SH config.sh ; /bin/sh newsnews.SH
ng.help: ng.help.SH config.sh ; /bin/sh ng.help.SH
norm.saver: norm.saver.SH config.sh ; /bin/sh norm.saver.SH
pager.help: pager.help.SH config.sh ; /bin/sh pager.help.SH
subs.help: subs.help.SH config.sh ; /bin/sh subs.help.SH
# WARNING: Put nothing here or make depend will gobble it up!

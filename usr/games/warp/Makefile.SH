case $CONFIG in
'')
    if test ! -f config.sh; then
	ln ../config.sh . || \
	ln ../../config.sh . || \
	ln ../../../config.sh . || \
	(echo "Can't find config.sh."; exit 1)
    fi
    . ./config.sh
    ;;
esac
echo "Extracting Makefile (with variable substitutions)"
: This section of the file will have variable substitutions done on it.
: Move anything that needs config subs from !NO!SUBS! section to !GROK!THIS!.
: Protect any dollar signs and backticks that you do not want interpreted
: by putting a backslash in front.
$spitshell >Makefile <<!GROK!THIS!
# $Header: Makefile.SH,v 1.1 87/07/03 01:45:30 games Exp $
#
# Revision 7.0.1.1a 87/26/08  03:45:03	games
# Fix minor shell continuation bug and put in a pdp-11 overlay scheme.
#
# Revision 7.0.1.1  86/12/12  16:54:03  lwall
# Frozen for net release.
# 
# Revision 7.0  86/10/08  15:11:42  lwall
# Split into separate files.  Added amoebas and pirates.
# 
#
CC = $cc
bin = $bin
mansrc = $mansrc
CFLAGS = $ccflags -O
LDFLAGS = $ldflags
CHOWNER = $chowner
privlib = $privlib
NDIRC = $ndirc
NDIRO = $ndiro
LARGE = $large $split
SMALL = $small

libs = $ndirlib $termlib $jobslib $libnm -lm
!GROK!THIS!

: In the following dollars and backticks do not need the extra backslash.
$spitshell >>Makefile <<'!NO!SUBS!'

public = warp
private = warp.doc smap.0 smap.1 smap.2 smap.3 smap.4 smap.5 smap.6 smap.7
manpages = warp.man
util = Makefile makedepend makedir smap.0 smap.1 smap.2 smap.3 smap.4 smap.5 smap.6 smap.7

sh = Makefile.SH makedepend.SH makedir.SH

h1 = bang.h init.h intrp.h move.h ndir.h
h2 = patchlevel.h object.h play.h score.h
h3 = sig.h term.h them.h us.h
h4 = util.h version.h warp.h weapon.h

h = $(h1) $(h2) $(h3) $(h4)

c1 = bang.c init.c intrp.c move.c
c2 = object.c play.c score.c
c3 = sig.c term.c them.c us.c
c4 = util.c version.c warp.c weapon.c

c = $(c1) $(c2) $(c3) $(c4) $(NDIRC)

obj1 = bang.o init.o intrp.o move.o
obj2 = object.o play.o score.o
obj3 = sig.o term.o them.o us.o
obj4 = util.o version.o warp.o weapon.o

obj = $(obj1) $(obj2) $(obj3) $(obj4) $(NDIRO)

lintflags = -phbvxac

add1 = Makefile.old 
add2 = bsd config.h config.sh eunice filexp
add3 = loc makedepend 
add4 = 
add5 = pdp11 warp usg v7

addedbyconf = $(add1) $(add2) $(add3) $(add4) $(add5)

# grrr
SHELL = /bin/sh

.c.o:
	$(CC) -c $(CFLAGS) $(LARGE) $*.c

all: $(public) $(private) $(util)
	touch all

warp: $(obj)
#	$(CC) $(LDFLAGS) $(obj) $(LARGE) $(libs) -o warp
	ld -X -i /lib/crt0.o $(obj2) $(obj4) init.o intrp.o \
		-Z sig.o term.o \
		-Z them.o us.o \
		-Z bang.o move.o \
		-Y $(libs) -lc -o warp


# if a .h file depends on another .h file...
$(h):
	touch $@

install: $(public) $(private) $(util)
# won't work with csh
	export PATH || exit 1
	- mv $(bin)/warp $(bin)/warp.old
	- if test `pwd` != $(bin); then cp $(public) $(bin); fi
	- cd $(bin); chmod 755 $(public); chown '$(CHOWNER)' $(public)
	- chmod 4711 $(bin)/warp
	- chmod 755 makedir
	- ./makedir `./filexp $(privlib)`
	- \
if test `pwd` != `./filexp $(privlib)`; then \
cp $(private) `./filexp $(privlib)`; \
fi
	- \
cd `./filexp $(privlib)`; \
chmod 755 $(private); \
chown '$(CHOWNER)' . $(private); \
cp /dev/null save.blurfl
	- \
if test ! -f `./filexp $(privlib)/warp.news`; then \
cp warp.news `./filexp $(privlib)`; \
fi
	- \
if test `pwd` != $(mansrc); then \
for page in $(manpages); do \
/usr/man/manroff $$page > $(mansrc)/`basename $$page .man`.0; \
chown bin.bin $(mansrc)/`basename $$page .man`.0; \
chmod 444 $(mansrc)/`basename $$page .man`.0; \
done; \
fi

clean:
	rm -f core *.o warp smap.* all sm

realclean:
	rm -f warp *.o core $(addedbyconf) $(util)

# The following lint has practically everything turned on.  Unfortunately,
# you have to wade through a lot of mumbo jumbo that can't be suppressed.
# If the source file has a /*NOSTRICT*/ somewhere, ignore the lint message
# for that spot.

lint:
	lint $(lintflags) $(defs) $(c) > warp.fuzz

smap.0: smp.0 sm
	./sm <smp.0 >smap.0
smap.1: smp.1 sm
	./sm <smp.1 >smap.1
smap.2: smp.2 sm
	./sm <smp.2 >smap.2
smap.3: smp.3 sm
	./sm <smp.3 >smap.3
smap.4: smp.4 sm
	./sm <smp.4 >smap.4
smap.5: smp.5 sm
	./sm <smp.5 >smap.5
smap.6: smp.6 sm
	./sm <smp.6 >smap.6
smap.7: smp.7 sm
	./sm <smp.7 >smap.7
sm: sm.c
	cc sm.c $(SMALL) -o sm

depend: makedepend
	/bin/sh ./makedepend

clist:
	echo $(c) | tr ' ' '\012' >.clist

hlist:
	echo $(h) | tr ' ' '\012' >.hlist

shlist:
	echo $(sh) | tr ' ' '\012' >.shlist

# AUTOMATICALLY GENERATED MAKE DEPENDENCIES--PUT NOTHING BELOW THIS LINE
$(obj):
	@ echo "You haven't done a "'"make depend" yet!'; exit 1
makedepend: makedepend.SH
	/bin/sh ./makedepend.SH
!NO!SUBS!
$eunicefix Makefile

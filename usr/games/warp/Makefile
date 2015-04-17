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
CC = cc
bin = /usr/games
mansrc = /tmp
CFLAGS = -O -O
LDFLAGS = 
CHOWNER = bin
privlib = /usr/games/lib/warp
NDIRC = 
NDIRO = 
LARGE =  -i
SMALL = 

libs =  -ltermlib  /usr/lib/libtermlib.a -lm

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
# If this runs make out of memory, delete /usr/include lines.
bang.o: /usr/include/ctype.h
bang.o: /usr/include/errno.h
bang.o: /usr/include/fcntl.h
bang.o: /usr/include/pwd.h
bang.o: /usr/include/sgtty.h
bang.o: /usr/include/signal.h
bang.o: /usr/include/stdio.h
bang.o: /usr/include/sys/ioctl.h
bang.o: /usr/include/sys/stat.h
bang.o: /usr/include/sys/timeb.h
bang.o: /usr/include/sys/ttychars.h
bang.o: /usr/include/sys/ttydev.h
bang.o: /usr/include/sys/types.h
bang.o: EXTERN.h
bang.o: INTERN.h
bang.o: bang.c
bang.o: bang.h
bang.o: config.h
bang.o: move.h
bang.o: object.h
bang.o: sig.h
bang.o: term.h
bang.o: them.h
bang.o: warp.h
init.o: /usr/include/ctype.h
init.o: /usr/include/errno.h
init.o: /usr/include/fcntl.h
init.o: /usr/include/pwd.h
init.o: /usr/include/sgtty.h
init.o: /usr/include/signal.h
init.o: /usr/include/stdio.h
init.o: /usr/include/sys/ioctl.h
init.o: /usr/include/sys/stat.h
init.o: /usr/include/sys/timeb.h
init.o: /usr/include/sys/ttychars.h
init.o: /usr/include/sys/ttydev.h
init.o: /usr/include/sys/types.h
init.o: EXTERN.h
init.o: INTERN.h
init.o: bang.h
init.o: config.h
init.o: init.c
init.o: init.h
init.o: move.h
init.o: object.h
init.o: play.h
init.o: score.h
init.o: term.h
init.o: them.h
init.o: us.h
init.o: util.h
init.o: warp.h
init.o: weapon.h
intrp.o: /usr/include/ctype.h
intrp.o: /usr/include/errno.h
intrp.o: /usr/include/fcntl.h
intrp.o: /usr/include/pwd.h
intrp.o: /usr/include/sgtty.h
intrp.o: /usr/include/signal.h
intrp.o: /usr/include/stdio.h
intrp.o: /usr/include/sys/ioctl.h
intrp.o: /usr/include/sys/stat.h
intrp.o: /usr/include/sys/timeb.h
intrp.o: /usr/include/sys/ttychars.h
intrp.o: /usr/include/sys/ttydev.h
intrp.o: /usr/include/sys/types.h
intrp.o: EXTERN.h
intrp.o: INTERN.h
intrp.o: config.h
intrp.o: intrp.c
intrp.o: intrp.h
intrp.o: sig.h
intrp.o: term.h
intrp.o: util.h
intrp.o: warp.h
move.o: /usr/include/ctype.h
move.o: /usr/include/errno.h
move.o: /usr/include/fcntl.h
move.o: /usr/include/pwd.h
move.o: /usr/include/sgtty.h
move.o: /usr/include/signal.h
move.o: /usr/include/stdio.h
move.o: /usr/include/sys/ioctl.h
move.o: /usr/include/sys/stat.h
move.o: /usr/include/sys/timeb.h
move.o: /usr/include/sys/ttychars.h
move.o: /usr/include/sys/ttydev.h
move.o: /usr/include/sys/types.h
move.o: EXTERN.h
move.o: INTERN.h
move.o: bang.h
move.o: config.h
move.o: move.c
move.o: move.h
move.o: object.h
move.o: play.h
move.o: score.h
move.o: term.h
move.o: them.h
move.o: us.h
move.o: util.h
move.o: warp.h
move.o: weapon.h
object.o: /usr/include/ctype.h
object.o: /usr/include/errno.h
object.o: /usr/include/fcntl.h
object.o: /usr/include/pwd.h
object.o: /usr/include/sgtty.h
object.o: /usr/include/signal.h
object.o: /usr/include/stdio.h
object.o: /usr/include/sys/ioctl.h
object.o: /usr/include/sys/stat.h
object.o: /usr/include/sys/timeb.h
object.o: /usr/include/sys/ttychars.h
object.o: /usr/include/sys/ttydev.h
object.o: /usr/include/sys/types.h
object.o: EXTERN.h
object.o: INTERN.h
object.o: config.h
object.o: object.c
object.o: object.h
object.o: warp.h
play.o: /usr/include/ctype.h
play.o: /usr/include/errno.h
play.o: /usr/include/fcntl.h
play.o: /usr/include/pwd.h
play.o: /usr/include/sgtty.h
play.o: /usr/include/signal.h
play.o: /usr/include/stdio.h
play.o: /usr/include/sys/ioctl.h
play.o: /usr/include/sys/stat.h
play.o: /usr/include/sys/timeb.h
play.o: /usr/include/sys/ttychars.h
play.o: /usr/include/sys/ttydev.h
play.o: /usr/include/sys/types.h
play.o: EXTERN.h
play.o: INTERN.h
play.o: bang.h
play.o: config.h
play.o: move.h
play.o: object.h
play.o: play.c
play.o: play.h
play.o: score.h
play.o: term.h
play.o: them.h
play.o: us.h
play.o: util.h
play.o: warp.h
play.o: weapon.h
score.o: /usr/include/ctype.h
score.o: /usr/include/errno.h
score.o: /usr/include/fcntl.h
score.o: /usr/include/pwd.h
score.o: /usr/include/sgtty.h
score.o: /usr/include/signal.h
score.o: /usr/include/stdio.h
score.o: /usr/include/sys/ioctl.h
score.o: /usr/include/sys/stat.h
score.o: /usr/include/sys/timeb.h
score.o: /usr/include/sys/ttychars.h
score.o: /usr/include/sys/ttydev.h
score.o: /usr/include/sys/types.h
score.o: EXTERN.h
score.o: INTERN.h
score.o: config.h
score.o: intrp.h
score.o: object.h
score.o: play.h
score.o: score.c
score.o: score.h
score.o: sig.h
score.o: term.h
score.o: us.h
score.o: util.h
score.o: warp.h
score.o: weapon.h
sig.o: /usr/include/ctype.h
sig.o: /usr/include/errno.h
sig.o: /usr/include/fcntl.h
sig.o: /usr/include/pwd.h
sig.o: /usr/include/sgtty.h
sig.o: /usr/include/signal.h
sig.o: /usr/include/stdio.h
sig.o: /usr/include/sys/ioctl.h
sig.o: /usr/include/sys/stat.h
sig.o: /usr/include/sys/timeb.h
sig.o: /usr/include/sys/ttychars.h
sig.o: /usr/include/sys/ttydev.h
sig.o: /usr/include/sys/types.h
sig.o: EXTERN.h
sig.o: INTERN.h
sig.o: config.h
sig.o: play.h
sig.o: score.h
sig.o: sig.c
sig.o: sig.h
sig.o: term.h
sig.o: util.h
sig.o: warp.h
term.o: /usr/include/ctype.h
term.o: /usr/include/errno.h
term.o: /usr/include/fcntl.h
term.o: /usr/include/pwd.h
term.o: /usr/include/sgtty.h
term.o: /usr/include/signal.h
term.o: /usr/include/stdio.h
term.o: /usr/include/sys/ioctl.h
term.o: /usr/include/sys/stat.h
term.o: /usr/include/sys/timeb.h
term.o: /usr/include/sys/ttychars.h
term.o: /usr/include/sys/ttydev.h
term.o: /usr/include/sys/types.h
term.o: EXTERN.h
term.o: INTERN.h
term.o: bang.h
term.o: config.h
term.o: intrp.h
term.o: object.h
term.o: play.h
term.o: score.h
term.o: sig.h
term.o: term.c
term.o: term.h
term.o: us.h
term.o: util.h
term.o: warp.h
term.o: weapon.h
them.o: /usr/include/ctype.h
them.o: /usr/include/errno.h
them.o: /usr/include/fcntl.h
them.o: /usr/include/pwd.h
them.o: /usr/include/sgtty.h
them.o: /usr/include/signal.h
them.o: /usr/include/stdio.h
them.o: /usr/include/sys/ioctl.h
them.o: /usr/include/sys/stat.h
them.o: /usr/include/sys/timeb.h
them.o: /usr/include/sys/ttychars.h
them.o: /usr/include/sys/ttydev.h
them.o: /usr/include/sys/types.h
them.o: EXTERN.h
them.o: INTERN.h
them.o: bang.h
them.o: config.h
them.o: move.h
them.o: object.h
them.o: score.h
them.o: term.h
them.o: them.c
them.o: them.h
them.o: us.h
them.o: util.h
them.o: warp.h
them.o: weapon.h
us.o: /usr/include/ctype.h
us.o: /usr/include/errno.h
us.o: /usr/include/fcntl.h
us.o: /usr/include/pwd.h
us.o: /usr/include/sgtty.h
us.o: /usr/include/signal.h
us.o: /usr/include/stdio.h
us.o: /usr/include/sys/ioctl.h
us.o: /usr/include/sys/stat.h
us.o: /usr/include/sys/timeb.h
us.o: /usr/include/sys/ttychars.h
us.o: /usr/include/sys/ttydev.h
us.o: /usr/include/sys/types.h
us.o: EXTERN.h
us.o: INTERN.h
us.o: bang.h
us.o: config.h
us.o: object.h
us.o: play.h
us.o: sig.h
us.o: term.h
us.o: us.c
us.o: us.h
us.o: util.h
us.o: warp.h
us.o: weapon.h
util.o: /usr/include/ctype.h
util.o: /usr/include/errno.h
util.o: /usr/include/fcntl.h
util.o: /usr/include/pwd.h
util.o: /usr/include/sgtty.h
util.o: /usr/include/signal.h
util.o: /usr/include/stdio.h
util.o: /usr/include/sys/dir.h
util.o: /usr/include/sys/ioctl.h
util.o: /usr/include/sys/stat.h
util.o: /usr/include/sys/timeb.h
util.o: /usr/include/sys/ttychars.h
util.o: /usr/include/sys/ttydev.h
util.o: /usr/include/sys/types.h
util.o: EXTERN.h
util.o: INTERN.h
util.o: config.h
util.o: ndir.h
util.o: object.h
util.o: sig.h
util.o: term.h
util.o: util.c
util.o: util.h
util.o: warp.h
version.o: INTERN.h
version.o: patchlevel.h
version.o: version.c
version.o: version.h
warp.o: /usr/include/ctype.h
warp.o: /usr/include/errno.h
warp.o: /usr/include/fcntl.h
warp.o: /usr/include/pwd.h
warp.o: /usr/include/sgtty.h
warp.o: /usr/include/signal.h
warp.o: /usr/include/stdio.h
warp.o: /usr/include/sys/ioctl.h
warp.o: /usr/include/sys/stat.h
warp.o: /usr/include/sys/timeb.h
warp.o: /usr/include/sys/ttychars.h
warp.o: /usr/include/sys/ttydev.h
warp.o: /usr/include/sys/types.h
warp.o: EXTERN.h
warp.o: INTERN.h
warp.o: bang.h
warp.o: config.h
warp.o: init.h
warp.o: intrp.h
warp.o: move.h
warp.o: object.h
warp.o: play.h
warp.o: score.h
warp.o: sig.h
warp.o: term.h
warp.o: them.h
warp.o: us.h
warp.o: util.h
warp.o: version.h
warp.o: warp.c
warp.o: warp.h
warp.o: weapon.h
weapon.o: /usr/include/ctype.h
weapon.o: /usr/include/errno.h
weapon.o: /usr/include/fcntl.h
weapon.o: /usr/include/pwd.h
weapon.o: /usr/include/sgtty.h
weapon.o: /usr/include/signal.h
weapon.o: /usr/include/stdio.h
weapon.o: /usr/include/sys/ioctl.h
weapon.o: /usr/include/sys/stat.h
weapon.o: /usr/include/sys/timeb.h
weapon.o: /usr/include/sys/ttychars.h
weapon.o: /usr/include/sys/ttydev.h
weapon.o: /usr/include/sys/types.h
weapon.o: EXTERN.h
weapon.o: INTERN.h
weapon.o: bang.h
weapon.o: config.h
weapon.o: move.h
weapon.o: object.h
weapon.o: score.h
weapon.o: sig.h
weapon.o: term.h
weapon.o: them.h
weapon.o: us.h
weapon.o: util.h
weapon.o: warp.h
weapon.o: weapon.c
weapon.o: weapon.h
Makefile: Makefile.SH config.sh ; /bin/sh Makefile.SH
makedepend: makedepend.SH config.sh ; /bin/sh makedepend.SH
makedir: makedir.SH config.sh ; /bin/sh makedir.SH
# WARNING: Put nothing here or make depend will gobble it up!

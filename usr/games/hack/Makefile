#	#(@)	Makefile	1.2 (2.11BSD)	1997/4/28
#
# Hack Overlay Makefile.

# on some systems the termcap library is in -ltermcap
TERMLIB = -ltermlib

# make hack
GAME = hack
BINDIR = /usr/games
LIBDIR = /usr/games/lib/hackdir
DESTDIR =
GAMEUID = daemon
CFLAGS = -O

HACKCSRC = \
	hack.Decl.c\
	hack.apply.c hack.bones.c hack.c hack.cmd.c hack.do.c\
	hack.do_name.c hack.do_wear.c hack.dog.c hack.eat.c hack.end.c\
	hack.engrave.c hack.fight.c hack.invent.c hack.ioctl.c\
	hack.lev.c hack.main.c hack.makemon.c hack.mhitu.c\
	hack.mklev.c hack.mkmaze.c hack.mkobj.c hack.mkshop.c\
	hack.mon.c hack.monst.c hack.o_init.c hack.objnam.c\
	hack.options.c hack.pager.c hack.potion.c hack.pri.c\
	hack.read.c hack.rip.c hack.rumors.c hack.save.c\
	hack.search.c hack.shk.c hack.shknam.c hack.steal.c\
	hack.termcap.c hack.timeout.c hack.topl.c\
	hack.track.c hack.trap.c hack.tty.c hack.unix.c\
	hack.u_init.c hack.vault.c\
	hack.wield.c hack.wizard.c hack.worm.c hack.worn.c hack.zap.c\
	hack.version.c rnd.c alloc.c

CSOURCES = $(HACKCSRC) makedefs.c

HSOURCES = hack.h hack.mfndpos.h config.h\
	def.edog.h def.eshk.h def.flag.h def.func_tab.h def.gold.h\
	def.mkroom.h\
	def.monst.h def.obj.h def.objclass.h def.objects.h\
	def.permonst.h def.rm.h def.trap.h def.wseg.h

SOURCES = $(CSOURCES) $(HSOURCES)

AUX = data help hh rumors hack.6 hack.sh

# strings.o must be last in HOBJ since it can change when other files compile.

HOBJ =  \
	hack.Decl.o hack.apply.o hack.bones.o hack.o hack.cmd.o hack.do.o\
	hack.do_name.o hack.do_wear.o hack.dog.o hack.eat.o hack.end.o\
	hack.engrave.o hack.fight.o hack.invent.o hack.ioctl.o\
	hack.lev.o hack.main.o hack.makemon.o hack.mhitu.o hack.mklev.o\
	hack.mkmaze.o hack.mkobj.o hack.mkshop.o hack.mon.o\
	hack.monst.o hack.o_init.o hack.objnam.o hack.options.o\
	hack.pager.o hack.potion.o hack.pri.o\
	hack.read.o hack.rip.o hack.rumors.o hack.save.o\
	hack.search.o hack.shk.o hack.shknam.o hack.steal.o\
	hack.termcap.o hack.timeout.o hack.topl.o\
	hack.track.o hack.trap.o\
	hack.tty.o hack.unix.o hack.u_init.o hack.vault.o hack.wield.o\
	hack.wizard.o hack.worm.o hack.worn.o hack.zap.o\
	hack.version.o rnd.o alloc.o\
	strings.o

# Hack overlay structure (PDP-11/{44,45,70})
#
#   The BASE (should) occupy 5 segments, but extreme care has to be taken when
# any code is changed as those five segments have been filled to within bytes
# of that limit.  Each of the overlays occupies 3 segments with similar
# restrictions on changing code (most have been filled to within a few hundred
# bytes).  The wise and cautious programmer would be well advised to use
# size(1) heavily ...
#
BASE =	strings.o hack.monst.o hack.Decl.o\
	hack.invent.o hack.dog.o hack.mon.o\
	hack.engrave.o rnd.o hack.track.o

OV1 =	hack.shk.o hack.topl.o hack.o hack.end.o hack.trap.o

OV2 =	hack.do_wear.o hack.main.o hack.eat.o hack.timeout.o hack.vault.o\
	hack.wizard.o hack.topl.o hack.tty.o\
	hack.do.o hack.search.o alloc.o\
	hack.cmd.o hack.termcap.o hack.unix.o hack.ioctl.o

OV3 =	hack.do_name.o hack.fight.o hack.mkobj.o hack.o_init.o hack.mhitu.o\
	hack.makemon.o hack.worn.o hack.rumors.o hack.objnam.o hack.pri.o

OV4 =	hack.zap.o hack.read.o hack.apply.o hack.potion.o\
	hack.options.o hack.pager.o hack.wield.o

OV5 =	hack.save.o hack.lev.o hack.mklev.o hack.mkshop.o hack.mkmaze.o\
	hack.shknam.o hack.u_init.o\
	hack.rip.o hack.bones.o hack.worm.o hack.steal.o hack.version.o

# Crypt *MUST be listed BEFORE $(HOBJ)
$(GAME): crypt $(HOBJ)
	@echo "Loading ..."
	@ld -i -X -x -o $(GAME) /lib/crt0.o $(BASE)\
	  -Z $(OV1)  -Z $(OV2)  -Z $(OV3)  -Z $(OV4) -Z $(OV5) -Y\
	  $(TERMLIB) -lc
	@echo "Load complete"

# The hack sources are run through several filters before they actually reach
# the compiler.  The main goal of the filtration process is to cut back the
# size of hack's data space (which normally exceeds 64K and therefore can't be
# loaded on a PDP-11).
#
# The filter crypt will read through C source files using a string translation
# table to translate long word(s) into shorter words in C strings.  Xstr
# collects the strings found in a program into a string database, replacing
# multiple occurences of the same string by references to one string in the
# database.  For more information on these filters, see crypt.c and xstr(1).
# Crypt is really crufty and slow, but the goal was to get something out the
# door ...
.c.o:
	cc -E $*.c | ./crypt | xstr -c -
	cc -c ${CFLAGS} x.c
	mv x.o $*.o

rnd.o:		rnd.c
alloc.o:	alloc.c

strings.o:	strings
	xstr
	cc -c ${CFLAGS} xs.c
	mv xs.o strings.o
	rm xs.c

crypt:	crypt.c
	cc -o crypt crypt.c

clean:
	rm -f *.o x.c xs.c strings crypt makedefs

makedefs:	makedefs.c
	cc -o makedefs makedefs.c

hack.onames.h:	makedefs def.objects.h
	./makedefs > hack.onames.h

lint:
# lint cannot have -p here because (i) capitals are meaningful:
# [Ww]izard, (ii) identifiers may coincide in the first six places:
# doweararm() versus dowearring().
# _flsbuf comes from <stdio.h>, a bug in the system libraries.
	@echo lint -axbh -DLINT ...
	@lint -axbh -DLINT $(HACKCSRC) | sed '/_flsbuf/d'


diff:
	@- for i in $(SOURCES) $(AUX) ; do \
		cmp -s $$i $D/$$i || \
		( echo diff $D/$$i $$i ; diff $D/$$i $$i ; echo ) ; done

distribution: Makefile
	@- for i in READ_ME $(SOURCES) $(AUX) Makefile date.h hack.onames.h\
		; do \
		cmp -s $$i $D/$$i || \
		( echo cp $$i $D ; cp $$i $D ) ; done
# the distribution directory also contains the empty files
# perm and record, and the informative files Porting and Bugs.


install: $(GAME)
	install -o $(GAMEUID) -m 4711 $(GAME) $(DESTDIR)$(BINDIR)/$(GAME)
	-rm -rf $(DESTDIR)/$(LIBDIR)
	-mkdir -p ${DESTDIR}/${LIBDIR}
	chown daemon ${DESTDIR}/${LIBDIR}
	chmod 700 ${DESTDIR}/${LIBDIR}

#   The make node `depend' has been commented out as the current dependency
# list is [now] not as simple as it used to be ...  The reason for this is
# because xstr(1) screws up on a number of the hack sources for one reason or
# another (see below) and those sources need to have special compile commands
# that run the sources through all the filters except xstr.
#
#   Xstr is unable to handle the construct `char  blah[] = "blah"' because it
# translates it into `char blah[] = &xstr[blah]'.  In one particular case, we
# can't run xstr on hack.rip.c because it translates the multiple lines of the
# tombstone into one line which screws up the tombstone.
#
#depend:
# For the moment we are lazy and disregard /usr/include files because
# the sources contain them conditionally. Perhaps we should use cpp.
#		( /bin/grep '^#[ 	]*include' $$i | sed -n \
#			-e 's,<\(.*\)>,"/usr/include/\1",' \
#
#	for i in ${CSOURCES}; do \
#		( /bin/grep '^#[ 	]*include[ 	]*"' $$i | sed -n \
#			-e 's/[^"]*"\([^"]*\)".*/\1/' \
#			-e H -e '$$g' -e '$$s/\n/ /g' \
#			-e '$$s/.*/'$$i': &/' -e '$$s/\.c:/.o:/p' \
#			>> makedep); done
#	for i in ${HSOURCES}; do \
#		( /bin/grep '^#[ 	]*include[ 	]*"' $$i | sed -n \
#			-e 's/[^"]*"\([^"]*\)".*/\1/' \
#			-e H -e '$$g' -e '$$s/\n/ /g' \
#			-e '$$s/.*/'$$i': &\
#				touch '$$i/p \
#			>> makedep); done
#	@echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
#	@echo '$$r makedep' >>eddep
#	@echo 'w' >>eddep
#	@cp Makefile Makefile.bak
#	ed - Makefile < eddep
#	@rm -f eddep makedep
#	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
#	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
#	@echo '# see make depend above' >> Makefile
#	- diff Makefile Makefile.bak
#	@rm -f Makefile.bak

# DO NOT DELETE THIS LINE

hack.Decl.o:  hack.h def.mkroom.h
	cc -E hack.Decl.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.Decl.o
hack.apply.o:  hack.h def.edog.h def.mkroom.h
hack.bones.o:  hack.h
	cc -E hack.bones.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.bones.o
hack.o:  hack.h
hack.cmd.o:  hack.h def.func_tab.h
	cc -E hack.cmd.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.cmd.o
hack.do.o:  hack.h
hack.do_name.o:  hack.h
hack.do_wear.o:  hack.h
hack.dog.o:  hack.h hack.mfndpos.h def.edog.h def.mkroom.h
hack.eat.o:  hack.h
	cc -E hack.eat.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.eat.o
hack.end.o:  hack.h
hack.engrave.o:  hack.h
hack.fight.o:  hack.h
	cc -E hack.fight.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.fight.o
hack.invent.o:  hack.h def.wseg.h
hack.ioctl.o:  config.h
hack.lev.o:  hack.h def.mkroom.h def.wseg.h
hack.main.o:  hack.h
	cc -E hack.main.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.main.o
hack.makemon.o:  hack.h
hack.mhitu.o:  hack.h
hack.mklev.o:  hack.h def.mkroom.h
hack.mkmaze.o:  hack.h def.mkroom.h
hack.mkobj.o:  hack.h
	cc -E hack.mkobj.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.mkobj.o
hack.mkshop.o:  hack.h def.mkroom.h def.eshk.h
hack.mon.o:  hack.h hack.mfndpos.h
hack.monst.o:  hack.h def.eshk.h
hack.o_init.o:  config.h def.objects.h hack.onames.h
hack.objnam.o:  hack.h
hack.options.o:  config.h hack.h
hack.pager.o:  hack.h
hack.potion.o:  hack.h
hack.pri.o:  hack.h def.wseg.h
hack.read.o:  hack.h
hack.rip.o:  hack.h
	cc -E hack.rip.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.rip.o
hack.rumors.o:  config.h
hack.save.o:  hack.h
hack.search.o:  hack.h
hack.shk.o:  hack.h hack.mfndpos.h def.mkroom.h def.eshk.h
hack.shknam.o:  hack.h
hack.steal.o:  hack.h
hack.termcap.o:  config.h def.flag.h
hack.timeout.o:  hack.h
hack.topl.o:  hack.h
hack.track.o:  hack.h
hack.trap.o:  hack.h def.mkroom.h
	cc -E hack.trap.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.trap.o
hack.tty.o:  hack.h
hack.unix.o:  hack.h def.mkroom.h
hack.u_init.o:  hack.h
hack.vault.o:  hack.h def.mkroom.h
hack.wield.o:  hack.h
hack.wizard.o:  hack.h
	cc -E hack.wizard.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.wizard.o
hack.worm.o:  hack.h def.wseg.h
hack.worn.o:  hack.h
hack.zap.o:  hack.h
hack.version.o:  date.h
	cc -E hack.version.c | ./crypt | sed 's/^# \([0-9]*\)/#line \1/' > x.c
	cc -c ${CFLAGS} x.c
	mv x.o hack.version.o
hack.h:  config.h def.objclass.h def.monst.h def.gold.h def.trap.h def.obj.h def.flag.h def.rm.h def.permonst.h hack.onames.h
			touch hack.h
def.objects.h:  config.h def.objclass.h
			touch def.objects.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above

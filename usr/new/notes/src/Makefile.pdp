#	Makefile for University of Illinois Notesfiles
#
# $Header: /usr/local/src/usenet/notes/src/RCS/Makefile,v 1.10 88/11/30 11:02:46 paul Exp $
#
#	Ray Essick
#	214 DCL
#	University of Illinois at Urbana-Champaign
#	1304 W. Springfield
#	Urbana, Illinois 61801
#	217-333-7937
#

BIN = /usr/local/notes
MSTDIR = /usr/spool/notes
ARCHDIR = /usr/spool/oldnotes
NET = /usr/local/notes
AUTOSEQ = autoseq
NOTES = notes
NOTESUID = 133
NOTESGRP = notes
ANON = nobody
ANONUID = 32767
LIBDIR = /usr/local/lib
CC = cc
CFLAGS = -DMSTDIR=\"$(MSTDIR)\" -DARCHDIR=\"$(ARCHDIR)\" \
	-DNOTES=\"$(NOTES)\" -DNOTESUID=$(NOTESUID) -DNOTESGRP=\"$(NOTESGRP)\" \
	-DANON=\"$(ANON)\" -DANONUID=$(ANONUID) \
	-DBIN=\"$(BIN)\" -O
CCFLAGS = -DMSTDIR='\"$(MSTDIR)\"' -DARCHDIR='\"$(ARCHDIR)\"' \
	-DNOTES='\"$(NOTES)\"' -DNOTESUID=$(NOTESUID) -DNOTESGRP='\"$(NOTESGRP)\"' \
	-DANON='\"$(ANON)\"' -DANONUID=$(ANONUID) \
	-DBIN='\"$(BIN)\"' -O
LFLAGS = -i
#	-n is shared, read-only text segment
TERMCAP = -ltermcap
# Evidently on System V (and other USG systems?) the termcap routines
# are in /usr/lib/libcurses.a

#
#	The following probably don't need to be modified
#
UTILITY = $(MSTDIR)/.utilities
SEQUENCER = $(MSTDIR)/.sequencer
LOCKS = $(MSTDIR)/.locks
ALIASES = $(UTILITY)/net.aliases
UNIQID = $(MSTDIR)/.SEQ

# directory where the "release" copy is kept
DISTDIR= p:/usr/src/uiuc/notesfiles
DCP=	rcp
RCSID = 1.7.0
RCSSTATE = Rel
RCSIFLAGS = -r$(RCSID) -s$(RCSSTATE)
RCSOFLAGS = -r$(RCSID) -s$(RCSSTATE)

#
#	Define some collections of routines so that things will
#	be easy to maintain
#

MISCFILES = Makefile Distfile rinstall getdate.y
HFILES = canon.h doc.h net.h newsgate.h parms.h structs.h dump.h
HLPFILES = access.help dir.help index.help lim.help read.help
CFILES = access.c adnote.c adresp.c archiver.c asearch.c buildnf.c check.c \
	checknotes.c compress.c control.c cursor.c datein.c delete.c \
	dmpnote.c dmpresp.c dropt.c dsply.c find.c gdate.c getnet.c \
	gname.c gtext.c gtime.c help.c index.c ldmisc.c limindx.c loadem.c \
	lprnote.c lrsp.c mailit.c main.c misc.c miscio.c mknf.c newsdump.c \
	newsgroup.c newsinput.c newsout.c newsoutput.c next.c nfalias.c \
	nfarchive.c nfcomment.c nfmail.c nfpipe.c nfprint.c nfrcv.c nfsend.c \
	nfstats.c nfxmit.c note.c pagein.c pageout.c pattern.c permit.c \
	perms.c preptxt.c prtind.c readem.c recsio.c resp.c rmnf.c \
	savtxt.c startup.c talkto.c times.c tsearch.c \
	expand.c newspath.c nfaccess.c acssort.c nfabort.c \
	nfdump.c dump.c nfload.c loadnf.c parse.c loadvars.c \
	bnewshead.c loadproto1.c ftime.c parsepath.c pagemove.c \
	nftimestamp.c nfadmin.c strcasecmp.c
USERCMDS = main nfpipe nfprint nfstats checknotes
NETCMDS = nfxmit nfrcv newsinput newsoutput
UTILCMDS = mknf rmnf nfadmin nfarchive nfaccess nfmail nftimestamp
MISCCMDS = nfload nfdump
CMDS =	$(USERCMDS) $(NETCMDS) $(UTILCMDS) $(MISCCMDS)

#
#	Objects (other than the mainline) to load with each program
#

LIBOBJS	= nfabort.o nfcomment.o
#
NFDUMP	= dump.o dmpnote.o dmpresp.o check.o recsio.o note.o resp.o \
	  lrsp.o startup.o misc.o miscio.o pageout.o gname.o perms.o \
	  permit.o gtime.o cursor.o strcasecmp.o
NFLOAD	= loadvars.o loadnf.o buildnf.o recsio.o gname.o gtime.o \
	  startup.o misc.o miscio.o cursor.o loadem.o parse.o acssort.o \
	  check.o perms.o permit.o note.o ldmisc.o find.o resp.o \
	  lrsp.o datein.o getdate.o ftime.o strcasecmp.o

all bin:	user network utility
	@echo notesfile binaries up-to-date

clean:
	rm -f *.o $(CMDS) libnfcom.a
#	DO NOT remove "spool".
	rm -f print
	rm -f Make.errs

sizes:
	size $(CMDS)

help:
	@echo type make base as su to make notes directories.
	@echo type make boot to compile and install everything.
	@echo type make install to compile and install code and man pages.
	@echo type make bin to merely make binaries.
	@echo for more info, see the Notesfile Reference Manual.
	@echo a good first time sequence is make base then make boot.

user:
	make -f Makefile.user CFLAGS="${CCFLAGS}" LFLAGS="${LFLAGS}" \
		TERMCAP="${TERMCAP}" $(USERCMDS)

network:
	make -f Makefile.net CFLAGS="${CCFLAGS}" LFLAGS="${LFLAGS}" \
		TERMCAP="${TERMCAP}" $(NETCMDS)

utility:
	make -f Makefile.util CFLAGS="${CCFLAGS}" LFLAGS="${LFLAGS}" \
		TERMCAP="${TERMCAP}" $(UTILCMDS)

#	make base should only be run once!
#	This set of commands sets up the required files down in the
#	binary directories. It's main purpose is to do the several
#	installation commands that might just have to be super-user.

base:
	-mkdir $(MSTDIR) $(ARCHDIR)
	-chown $(NOTES) $(MSTDIR) $(ARCHDIR)
	-chgrp $(NOTESGRP) $(MSTDIR) $(ARCHDIR)
	(cd $(BIN); touch notes nfprint nfstats checknotes nfpipe)
	-(cd $(BIN); chown $(NOTES) notes nfprint nfstats nfpipe checknotes)
	-(cd $(BIN); chgrp $(NOTESGRP) notes nfprint nfstats nfpipe checknotes)
#	establish "aliases" for auto-sequencing entry points
#	make more links with other names if you want them.
	- ln -s $(BIN)/notes $(BIN)/$(AUTOSEQ)
	-(cd $(NET); touch nfxmit nfrcv)
	-(cd $(NET); chown $(NOTES) nfxmit nfrcv)
	-(cd $(NET); chgrp $(NOTESGRP) nfxmit nfrcv)
	touch $(LIBDIR)/libnfcom.a
	-chown $(NOTES) $(LIBDIR)/libnfcom.a
	-chgrp $(NOTESGRP) $(LIBDIR)/libnfcom.a
	@echo base generation done

boot:	spool $(UNIQID) placelib bin install
#	make the nfgripes file and open it
	-./mknf -o nfgripes
	-./mknf -on nfmaint
	@echo notesfile system booted

spool:
	-mkdir $(UTILITY) $(SEQUENCER) $(LOCKS) $(ALIASES)
	-chown $(NOTES) $(UTILITY) $(SEQUENCER) $(LOCKS) $(ALIASES)
	-chgrp $(NOTESGRP) $(UTILITY) $(SEQUENCER) $(LOCKS) $(ALIASES)
	chmod 775 $(MSTDIR) $(UTILITY) $(SEQUENCER) $(LOCKS) $(ALIASES)
	-mkdir $(UTILITY)/coredump
	chmod 777 $(UTILITY)/coredump
	-chown $(NOTES) $(UTILITY)/coredump
	-chgrp $(NOTESGRP) $(UTILITY)/coredump
#	generate the available notesfile file here - only once!
	echo "available notesfiles" > $(UTILITY)/avail.notes
	cp moderators $(UTILITY)/moderators
	touch $(UTILITY)/Dflt-Seq
	-chown $(NOTES) $(UTILITY)/moderators $(UTILITY)/avail.notes $(UTILITY)/Dflt-Seq
	-chgrp $(NOTESGRP) $(UTILITY)/moderators $(UTILITY)/avail.notes $(UTILITY)/Dflt-Seq
	chmod 664 $(UTILITY)/avail.notes $(UTILITY)/Dflt-Seq
	@echo Notesfile Spool directories ready

$(UNIQID):
	echo 1 > $(UNIQID)
	chmod 660 $(UNIQID)
	-chown $(NOTES) $(UNIQID)
	-chgrp $(NOTESGRP) $(UNIQID)

#	make sure that all the appropriate directories exist
dirs:	$(MSTDIR) $(ARCHDIR) $(UTILITY) $(SEQUENCER) $(LOCKS) \
	$(ALIASES) $(UTILITY)/coredump

$(MSTDIR) $(ARCHDIR) $(UTILITY) $(SEQUENCER) $(LOCKS) $(ALIASES):
	-mkdir $@
	-chown $(NOTES) $@
	-chgrp $(NOTESGRP) $@
	chmod 775 $@

$(UTILITY)/coredump:
	-mkdir $@
	-chown $(NOTES) $@
	-chgrp $(NOTESGRP) $@
	chmod 777 $@

install: dirs $(UNIQID) placedoc placelib
	make -f Makefile.user NOTES="${NOTES}" NOTESGRP="${NOTESGRP}" \
		BIN="${BIN}" install
	make -f Makefile.util NOTES="${NOTES}" NOTESGRP="${NOTESGRP}" \
		UTILITY="${UTILITY}" install 
	make -f Makefile.net NOTES="${NOTES}" NOTESGRP="${NOTESGRP}" \
		NET="${NET}" UTILITY="${UTILITY}" install
	@echo notesfiles installed

placedoc:	$(HLPFILES)
	cp $? $(UTILITY)
	-(cd $(UTILITY); chmod 644 $?)
	-(cd $(UTILITY); chown $(NOTES) $?; chgrp $(NOTESGRP) $?)
	@echo notesfile help files installed

placelib: libnfcom.a
	cp libnfcom.a $(LIBDIR)/libnfcom.a
	-ranlib $(LIBDIR)/libnfcom.a

#
#	Save old binaries. Put them all in the utility directory
#	so we don't clutter the bin directories
#

save:	saveuser savenet saveutil

saveuser:
	cp $(BIN)/notes $(UTILITY)/notes.old
	cp $(BIN)/nfpipe $(UTILITY)/nfpipe.old
	cp $(BIN)/nfprint $(UTILITY)/nfprint.old
	cp $(BIN)/nfstats $(UTILITY)/nfstats.old
	cp $(BIN)/checknotes $(UTILITY)/checknotes.old

savenet:
	cp $(NET)/nfxmit $(UTILITY)/nfxmit.old
	cp $(NET)/nfrcv $(UTILITY)/nfrcv.old
	cp $(UTILITY)/newsinput $(UTILITY)/newsinput.old
	cp $(UTILITY)/newsoutput $(UTILITY)/newsoutput.old

saveutil:
	cp $(UTILITY)/mknf $(UTILITY)/mknf.old
	cp $(UTILITY)/rmnf $(UTILITY)/rmnf.old
	cp $(UTILITY)/nfarchive $(UTILITY)/nfarchive.old
	cp $(UTILITY)/nfmail $(UTILITY)/nfmail.old

#
#	User subroutines
#

libnfcom.a:	$(LIBOBJS)
	ar r libnfcom.a $(LIBOBJS)
	-ranlib libnfcom.a

#
#	Load/Dump tools
#

nfdump:	nfdump.o $(NFDUMP) libnfcom.a
	@echo loading nfdump
	@$(CC) -o nfdump nfdump.o $(NFDUMP) libnfcom.a $(TERMCAP)

nfload: nfload.o $(NFLOAD) libnfcom.a
	@echo loading nfload
	@$(CC) -o nfload nfload.o $(NFLOAD) libnfcom.a $(TERMCAP)

#
#	this is to make updating the "release" copy easy.
#

dist:	$(CFILES) $(HFILES) $(HLPFILES) $(MISCFILES) Revisions
	${DCP} $? $(DISTDIR)/src/.

Revisions: $(CFILES) $(HFILES) $(MISCFILES)
	grep "Header:" $(CFILES) $(HFILES) $(MISCFILES) > Revisions

#
#	Update the RCS library with this command.
#	Puts modified files into the RCS libraries with
#	the version number specified by "RCSID".
#	Also uses the RCS[IO]FLAGS variables.
#
#	since RCS has a bug in that the -l flag does no update the
#	version number in the copy left available, we check in
#	the files and then check them out again.
#

freeze:	$(CFILES) $(HFILES) $(MISCFILES) $(HLPFILES)
	ci $(RCSIFLAGS) $?
	co $(RCSOFLAGS) $?

#	Make printouts of the modified routines
#

print:	$(MISCFILES) $(HFILES) $(CFILES) $(HLPFILES)
#	pr -f $? | lpr -b "Notesfile Code"
	imprint -2 -I -P2 $?

#	For those using vi, produce a ctags file of the source code.
#

tags:	$(CFILES)
	ctags $(CFILES)

#
#	Lint the mess to catch inconsistencies.
#

lint:
	lint -haxc $(CFLAGS) *.c

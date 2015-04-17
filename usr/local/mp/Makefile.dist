#
#  Makefile for mp, the PostScript pretty printer.
#
#  @(#)Makefile.dist 1.11 92/02/17
#
#  Copyright (c) Steve Holden and Rich Burridge.
#                All rights reserved.
#
#  Permission is given to distribute these sources, as long as the
#  copyright messages are not removed, and no monies are exchanged.
#
#  No responsibility is taken for any errors inherent either
#  to the comments or the code of this program, but if reported
#  to me then an attempt will be made to fix them.
#
#=====================================================================
#
#  It is possible to change the location of the mp prologue file
#  with the -p command line option. It can also be defined at compile
#  time, by uncommenting the macro definition below and setting
#  appropriately. If it's not present, then a sensible default value
#  is used.
#
PROLOGUE = -DPROLOGUE=\"$(MPLIBDIR)\"
#---------------------------------------------------------------------
#  Mp will extract the user's gecos field, and try to make sense of what
#  it finds. It applies various rules to do this. If this field is too long,
#  then it needs to truncate sensibly. If it's extracted N "words" or has
#  found a comma, then it stops extracting. If an ampersand is found, this
#  is expanded to the users name in capitals.
#  This option allows you to change the number of "words" that mp extracts
#  from the gecos field. By default this is 3. If needed, then it should
#  be uncommented, and set accordingly.
#
#GECOSFIELDS     = -DGECOSFIELDS=3
#---------------------------------------------------------------------
#  Mp will extract upto a certain number of characters from the user's
#  passwd gecos field. By default, this is 18. This option allows you to
#  set this to something different. If needed, then it should be uncommented,
#  and set accordingly.
#
#GECOSLENGTH     = -DGECOSLENGTH=18
#---------------------------------------------------------------------
#  Not all machines have the strchr() string library function. If you
#  don't have this function then you should uncomment the NOSTRCHR
#  definition below.
#
#NOSTRCHR       = -DNOSTRCHR
#---------------------------------------------------------------------
#  If you are running on a Unix System V machine, then you should
#  uncomment the next definition.
#
#SYSV           = -DSYSV
#=====================================================================
#
#  Default locations where mp files will be installed.
#  You might wish to alter these values.
#
MPPATH     = /usr/local
MPBINDIR   = $(MPPATH)/bin
MPLIBDIR   = $(MPPATH)/lib
MPMANDIR   = $(MPPATH)/man/man$(MPMANSECT)
MPMANSECT  = l
#---------------------------------------------------------------------
#  Alternate names for the mailp frontend
#
MAILPNAMES = newsp digestp filep filofaxp franklinp timemanp timesysp
#---------------------------------------------------------------------
#  Compilation flags and standard macro definitions.
#
CDEFS	 = $(GECOSFIELDS) $(GECOSLENGTH) $(NOSTRCHR) $(PROLOGUE) $(SYSV)
CCOMP	 = -O
CFLAGS   = $(CCOMP) $(CDEFS)
LDFLAGS  =

CC	 = cc
#=================================================================

.KEEPSTATE:

BINARIES = mp

SRCS     = header.c io.c main.c misc.c print.c
OBJS     = header.o io.o main.o misc.o print.o

HDRS	= extern.h mp.h patchlevel.h
MPAGES  = mp.man mp.man.text mailp.man mailp.man.text
PROLOGS = mp.common.ps mp.pro.ps mp.pro.l.ps mp.pro.alt.ps mp.pro.altl.ps \
	  mp.pro.ff.ps mp.pro.fp.ps mp.pro.tm.ps mp.pro.ts.ps
OTHERS	= README Makefile.dist mailp MANIFEST FILES CHANGES TODO \
	  mp.el mp.pro.old mp.pro.l.old Data

SFILES1  = README Makefile.dist MANIFEST FILES Data
SFILES2  = $(SRCS) $(HDRS)
SFILES3  = $(PROLOGS)
SFILES4  = TODO mailp mp.el mp.pro.old mp.pro.l.old $(MPAGES)
SFILES5  = CHANGES

all:	$(BINARIES)

mp:	$(OBJS) mp.man mailp.man
	$(CC) $(LDFLAGS) -o mp $(OBJS)
	-sed -e s,LIBDIR,$(MPLIBDIR),g \
	     -e s,BINDIR,$(MPBINDIR),g mp.man > mp.1
	-sed -e s,LIBDIR,$(MPLIBDIR),g \
	     -e s,BINDIR,$(MPBINDIR),g mailp.man > mailp.1

SHELL = /bin/sh
install-a4: common-install
	   @echo "      Adjusting for A4 paper"
	   -@cd $(MPLIBDIR); \
		cp mp.common.ps /tmp/tmp-pro; \
		sed < /tmp/tmp-pro > mp.common.ps \
		   -e 's|^/fullwidth 8.5 inch def|/fullwidth 595 def|' \
		   -e 's|^/fullheight 11 inch def|/fullheight 842 def|'; \
		rm /tmp/tmp-pro

install:   common-install
	   @echo
	   @echo "**IMPORTANT NOTE**"
	   @echo
	   @echo "Mp is setup by default to work with US Letter sized paper."
	   @echo "If you are using A4 sized paper, you should reinstall, by
	   @echo "doing a:"
	   @echo
	   @echo "make install-a4"
	   @echo

common-install: $(BINARIES)
#                                                     mp
	   -cp mp $(MPBINDIR)
	   -chmod 751 $(MPBINDIR)/mp
	   -strip $(MPBINDIR)/mp
#                                                     mp.1
	   -cp mp.1 $(MPMANDIR)/mp.$(MPMANSECT)
	   -chmod 644 $(MPMANDIR)/mp.$(MPMANSECT)
#                                                     mailp.1
	   -cp mailp.1 $(MPMANDIR)/mailp.$(MPMANSECT)
	   -chmod 644 $(MPMANDIR)/mailp.$(MPMANSECT)
#                                                     mp.common.ps
	   -cp mp.common.ps $(MPLIBDIR)
	   -chmod 644 $(MPLIBDIR)/mp.common.ps
#                                                     mp.pro.ps
	   -cp mp.pro.ps $(MPLIBDIR)
	   -chmod 644 $(MPLIBDIR)/mp.pro.ps
#                                                     mp.pro.l.ps
	   -cp mp.pro.l.ps $(MPLIBDIR)
	   -chmod 644 $(MPLIBDIR)/mp.pro.l.ps
#                                                     mp.pro.alt.ps
	   -cp mp.pro.alt.ps $(MPLIBDIR)
	   -chmod 644 $(MPLIBDIR)/mp.pro.alt.ps
#                                                     mp.pro.altl.ps
	   -cp mp.pro.altl.ps $(MPLIBDIR)
	   -chmod 644 $(MPLIBDIR)/mp.pro.altl.ps
#                                                     mp.pro.ff.ps
	   -cp mp.pro.ff.ps $(MPLIBDIR)
	   -chmod 644 $(MPLIBDIR)/mp.pro.ff.ps
#                                                     mp.pro.fp.ps
	   -cp mp.pro.fp.ps $(MPLIBDIR)
	   -chmod 644 $(MPLIBDIR)/mp.pro.fp.ps
#                                                     mp.pro.tm.ps
	   -cp mp.pro.tm.ps $(MPLIBDIR)
	   -chmod 644 $(MPLIBDIR)/mp.pro.tm.ps
#                                                     mp.pro.ts.ps
	   -cp mp.pro.ts.ps $(MPLIBDIR)
	   -chmod 644 $(MPLIBDIR)/mp.pro.ts.ps
#                                                     mailp
#
	   -sed -e s,BINDIR,$(MPBINDIR),g mailp >$(MPBINDIR)/mailp
	   -chmod 755 $(MPBINDIR)/mailp
#
#  NOTE: If you don't want the newsp, filep ... shell scripts installed,
#        then you should comment out the following seven lines.
#
	   @echo Making mailp links
	   -@for name in $(MAILPNAMES); do \
		echo -n $$name " "; \
		rm -f $(MPBINDIR)/$$name; \
		ln -s $(MPBINDIR)/mailp $(MPBINDIR)/$$name; \
	   done
	   @echo

clean:
	rm -rf mp.part? Part? *.Z *.uu mp.1 mp *.o core

create:    SCCS
	-sccs create $(SRCS) $(OTHERS)

lint:
	lint $(CDEFS) $(SRCS)

saber_src:
	#load $(CDEFS) $(SRCS)

shar:
	shar.script $(SFILES1) > Part1
	shar.script $(SFILES2) > Part2
	shar.script $(SFILES3) > Part3
	shar.script $(SFILES4) > Part4
	shar.script $(SFILES5) > Part5

SCCS:
	mkdir SCCS
	chmod 755 SCCS

header.o:	mp.h extern.h
io.o:		mp.h extern.h
main.o:		mp.h
misc.o:		mp.h extern.h patchlevel.h
print.o:	mp.h extern.h

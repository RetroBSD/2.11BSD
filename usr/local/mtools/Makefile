#
#       Makefile for Mtools
#
# check the Configure file for some examples of device-specific setups
# Berkeley flavors of Unix should include -DBSD in the CFLAGS

CFLAGS	= -O -DBSD
LDFLAGS	= -i
LD	= cc
LINT	= lint
SHLIB	= 
SHAR	= shar -a
BINDIR	= /usr/local
MANDIR	= /usr/local/man/cat1
#
# for AT&T Unix PC 7300/3b1 style shared libraries.
#SHOBJ	= /lib/crt0s.o /lib/shlib.ifile
#LD	= ld

PROGS	= mattrib mcd mcopy mdel mdir mformat mlabel mmd mrd mread mren \
	mtype mwrite mkmanifest

MANPAGES= mattrib.0 mcd.0 mcopy.0 mdel.0 mdir.0 mformat.0 mlabel.0 \
	mmd.0 mrd.0 mread.0 mren.0 mtools.0 mtype.0 mwrite.0 mkmanifest.0

MATTRIB	= mattrib.o buf_read.o buf_write.o devices.o dir_read.o \
	dir_write.o expand.o fat_read.o init.o match.o parse.o subdir.o
MCD	= mcd.o buf_read.o devices.o dir_read.o expand.o fat_read.o \
	init.o is_dir.o match.o parse.o subdir.o
MCOPY	= mcopy.o
MDEL	= mdel.o buf_read.o buf_write.o devices.o dir_read.o dir_write.o \
	expand.o fat_free.o fat_read.o fat_write.o init.o match.o parse.o \
	subdir.o 
MDIR	= mdir.o buf_read.o devices.o dir_read.o expand.o fat_read.o \
	init.o is_dir.o match.o parse.o subdir.o
MFORMAT	= mformat.o devices.o dir_write.o expand.o
MLABEL	= mlabel.o buf_read.o buf_write.o devices.o dir_read.o \
	dir_write.o expand.o fat_read.o init.o 
MMD	= mmd.o buf_read.o buf_write.o devices.o dir_make.o dir_read.o \
	dir_write.o expand.o fat_read.o fat_write.o init.o match.o parse.o \
	subdir.o
MRD	= mrd.o buf_read.o buf_write.o devices.o dir_read.o dir_write.o \
	expand.o fat_free.o fat_read.o fat_write.o init.o match.o parse.o \
	subdir.o
MREAD	= mread.o buf_read.o devices.o dir_read.o expand.o fat_read.o \
	file_read.o init.o match.o parse.o subdir.o
MREN	= mren.o buf_read.o buf_write.o devices.o dir_read.o dir_write.o \
	expand.o fat_read.o init.o match.o parse.o subdir.o
MTYPE	= mtype.o buf_read.o devices.o dir_read.o expand.o fat_read.o \
	file_read.o init.o match.o parse.o subdir.o
MWRITE	= mwrite.o buf_read.o buf_write.o devices.o dir_make.o \
	dir_read.o dir_write.o expand.o fat_free.o fat_read.o fat_write.o \
	file_write.o init.o is_dir.o match.o parse.o subdir.o
MKMANIFEST = mkmanifest.o

all:	$(PROGS) $(MANPAGES)

mattrib: $(MATTRIB)
	$(LD) $(LDFLAGS) $(MATTRIB) $(SHOBJ) -o mattrib $(SHLIB)

mcd:	$(MCD)
	$(LD) $(LDFLAGS) $(MCD) $(SHOBJ) -o mcd $(SHLIB)

mcopy:	$(MCOPY)
	$(LD) $(LDFLAGS) $(MCOPY) $(SHOBJ) -o mcopy $(SHLIB)

mdel:	$(MDEL)
	$(LD) $(LDFLAGS) $(MDEL) $(SHOBJ) -o mdel $(SHLIB)

mdir:	$(MDIR)
	$(LD) $(LDFLAGS) $(MDIR) $(SHOBJ) -o mdir $(SHLIB)

mformat: $(MFORMAT)
	$(LD) $(LDFLAGS) $(MFORMAT) $(SHOBJ) -o mformat $(SHLIB)

mlabel:	$(MLABEL)
	$(LD) $(LDFLAGS) $(MLABEL) $(SHOBJ) -o mlabel $(SHLIB)

mmd:	$(MMD)
	$(LD) $(LDFLAGS) $(MMD) $(SHOBJ) -o mmd $(SHLIB)

mrd:	$(MRD)
	$(LD) $(LDFLAGS) $(MRD) $(SHOBJ) -o mrd $(SHLIB)

mread:	$(MREAD)
	$(LD) $(LDFLAGS) $(MREAD) $(SHOBJ) -o mread $(SHLIB)

mren:	$(MREN)
	$(LD) $(LDFLAGS) $(MREN) $(SHOBJ) -o mren $(SHLIB)

mtype:	$(MTYPE)
	$(LD) $(LDFLAGS) $(MTYPE) $(SHOBJ) -o mtype $(SHLIB)

mwrite:	$(MWRITE)
	$(LD) $(LDFLAGS) $(MWRITE) $(SHOBJ) -o mwrite $(SHLIB)

mkmanifest:	$(MKMANIFEST)
	$(LD) $(LDFLAGS) $(MKMANIFEST) $(SHOBJ) -o mkmanifest $(SHLIB)

mattrib.0: Mattrib.1
	 nroff -man Mattrib.1  >  mattrib.0
mcd.0: Mcd.1
	 nroff -man Mcd.1  >  mcd.0
mcopy.0: Mcopy.1
	 nroff -man Mcopy.1  >  mcopy.0
mdel.0: Mdel.1
	 nroff -man Mdel.1  >  mdel.0
mdir.0: Mdir.1
	 nroff -man Mdir.1  >  mdir.0
mformat.0: Mformat.1
	 nroff -man Mformat.1  >  mformat.0
mkmanifest.0: Mkmanifest.1
	 nroff -man Mkmanifest.1  >  mkmanifest.0
mlabel.0: Mlabel.1
	 nroff -man Mlabel.1  >  mlabel.0
mmd.0: Mmd.1
	 nroff -man Mmd.1  >  mmd.0
mrd.0: Mrd.1
	 nroff -man Mrd.1  >  mrd.0
mread.0: Mread.1
	 nroff -man Mread.1  >  mread.0
mren.0: Mren.1
	 nroff -man Mren.1  >  mren.0
mtools.0: Mtools.1
	 nroff -man Mtools.1  >  mtools.0
mtype.0: Mtype.1
	 nroff -man Mtype.1  >  mtype.0
mwrite.0: Mwrite.1
	 nroff -man Mwrite.1  >  mwrite.0

install: $(PROGS) $(MANPAGES)
	install -s -m 755 -o bin $(PROGS) ${DESTDIR}$(BINDIR)
	install -c -m 444 -o bin -g bin *.0 ${DESTDIR}$(MANDIR)

clean:
	rm -f $(PROGS) *.0 *.o

lint:
	$(LINT) mattrib.c buf_read.c buf_write.c devices.c dir_read.c \
	dir_write.c expand.c fat_read.c init.c match.c parse.c subdir.c
	$(LINT) mcd.c buf_read.c devices.c dir_read.c expand.c fat_read.c \
	init.c is_dir.c match.c parse.c subdir.c
	$(LINT) mcopy.c
	$(LINT) mdel.c buf_read.c buf_write.c devices.c dir_read.c dir_write.c \
	expand.c fat_free.c fat_read.c fat_write.c init.c match.c parse.c \
	subdir.c 
	$(LINT) mdir.c buf_read.c devices.c dir_read.c expand.c fat_read.c \
	init.c is_dir.c match.c parse.c subdir.c
	$(LINT) mformat.c devices.c dir_write.c expand.c
	$(LINT) mlabel.c buf_read.c buf_write.c devices.c dir_read.c \
	dir_write.c expand.c fat_read.c init.c 
	$(LINT) mmd.c buf_read.c buf_write.c devices.c dir_make.c dir_read.c \
	dir_write.c expand.c fat_read.c fat_write.c init.c match.c parse.c \
	subdir.c
	$(LINT) mrd.c buf_read.c buf_write.c devices.c dir_read.c dir_write.c \
	expand.c fat_free.c fat_read.c fat_write.c init.c match.c parse.c \
	subdir.c
	$(LINT) mread.c buf_read.c devices.c dir_read.c expand.c fat_read.c \
	file_read.c init.c match.c parse.c subdir.c
	$(LINT) mren.c buf_read.c buf_write.c devices.c dir_read.c dir_write.c \
	expand.c fat_read.c init.c match.c parse.c subdir.c
	$(LINT) mtype.c buf_read.c devices.c dir_read.c expand.c fat_read.c \
	file_read.c init.c match.c parse.c subdir.c
	$(LINT) mwrite.c buf_read.c buf_write.c devices.c dir_make.c \
	dir_read.c dir_write.c expand.c fat_free.c fat_read.c fat_write.c \
	file_write.c init.c is_dir.c match.c parse.c subdir.c
	$(LINT) mkmanifest.c

shar:
	$(SHAR) Configure Makefile *.1 Readme Release.notes buf_read.c \
	buf_write.c devices.c dir_make.c dir_read.c dir_write.c expand.c \
	fat_free.c > mtools_sh.1
	$(SHAR) fat_read.c fat_write.c file_read.c file_write.c init.c \
	is_dir.c match.c mattrib.c mcd.c mcopy.c mdel.c mdir.c mformat.c \
	mkmanifest.c mlabel.c mmd.c > mtools_sh.2
	$(SHAR) mrd.c mread.c mren.c msdos.h mtype.c mwrite.c parse.c \
	patchlevel.h subdir.c > mtools_sh.3
	$(SHAR) Mattrib.out Mcd.out Mcopy.out Mdel.out Mdir.out Mformat.out \
	Mkmanifest.out Mlabel.out Mmd.out Mrd.out Mread.out Mren.out \
	Mtools.out Mtype.out Mwrite.out > mtools_sh.4

buf_read.o:	msdos.h
buf_write.o:	msdos.h
devices.o:	msdos.h
dir_make.o:	msdos.h
dir_read.o:	msdos.h
dir_write.o:	msdos.h
fat_free.o:	msdos.h
fat_read.o:	msdos.h
fat_write.o:	msdos.h
file_read.o:	msdos.h
file_write.o:	msdos.h
init.o:		msdos.h
is_dir.o:	msdos.h
mattrib.o:	msdos.h patchlevel.h
mcd.o:		msdos.h patchlevel.h
mcopy.o:	patchlevel.h
mdel.o:		msdos.h patchlevel.h
mdir.o:		msdos.h patchlevel.h
mformat.o:	msdos.h patchlevel.h
mlabel.o:	msdos.h patchlevel.h
mmd.o:		msdos.h patchlevel.h
mrd.o:		msdos.h patchlevel.h
mread.o:	msdos.h patchlevel.h
mren.o:		msdos.h patchlevel.h
mtype.o:	msdos.h patchlevel.h
mwrite.o:	msdos.h patchlevel.h
parse.o:	msdos.h
subdir.o:	msdos.h

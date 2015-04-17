#	Makefile	(2.11BSD)	2.3	1997/8/11
#
#	The limitations on program size have been removed.  The addition
#	of disklabel support pushed 'restor' over the limit.  Even with
#	additional space saving measures it was impossible to fit restor
#	into 48kb.  'icheck' and 'restor' are now built with split I/D.
#
#	This is not as bad as it sounds - the kernel has not been able to
#	run on a non split machine for many years.  It made little sense 
#	to restrict the standalone utilities to non-split mode when the
#	operating system itself required split I/D.
#
#	It is still possible to hand craft a version of the utilities by leaving
#	out all but the necessary drivers. 
#
# 1997/8/11  - add -O to maketape and makesimtape compile commands
# 1997/1/24  - remove 'skel' from lists of directories.
# 1997/1/18  - add 'makesimtape' target.
# 1996/10/28 - added usr/sbin, usr/libexec and corresponding src directories.
# 1995/12/05 - add RX02 driver.
# 1995/06/05 - add disklabel program to Makefile.
# 1995/06/01 - use split I/D for icheck and restor.
# 1995/05/30 - Begin adding disklabel support.

# DISK	which disk to take a root dump of for the distribution tape
# TAPE	which tape to write the distribution on
# DUMP	where to store a root system dump if you do a ``make dump''
#
DISK=	xp0a
TAPE=	mt8
DUMP=	/usr/root.dump

ROOT=	
SBINSRC=${ROOT}/usr/src/sbin
MKFS=	${SBINSRC}/mkfs/mkfs.c
RESTOR=	${SBINSRC}/restor/restor.c
ICHECK=	${SBINSRC}/icheck/icheck.c

DEFS=	-DSTANDALONE -I${ROOT}/usr/include -I${ROOT}.
CFLAGS=	-O ${DEFS}

BOOT=	M.o boot.o ubmapset.o
DRIVERS=prf.o sys.o label.o \
	ht.o tm.o ts.o tmscp.o \
	xp.o rk.o rl.o rx.o br.o hk.o si.o ra.o

ALL=	mtboot boot disklabel mkfs restor icheck maketape makesimtape toyset

all: ${ALL}

.c.o:
	cc ${CFLAGS} -c $*.c

.s.o:
	/lib/cpp -P ${DEFS} $< | as -u -V -o $@

srt0-i.o: srt0.s
	/lib/cpp -P -DSPLIT_ID ${DEFS} srt0.s | as -u -V -o $@

distribution: tape1 switch_tapes tape2

tmscptape: ${ALL} ${DUMP}
	./maketape /dev/nr${TAPE} maketape.data
	dd if=${DUMP} of=/dev/nr${TAPE} bs=20b
	cd ${ROOT}/usr; tar cfb /dev/nr${TAPE} 20 \
		adm bin crash dict doc games guest hosts include ingres \
		lib libdata libexec local man msgs new old preserve pub \
		sbin share spool tmp ucb
	cd ${ROOT}/usr/src; tar cfb /dev/nr${TAPE} 20 \
		sys include
	cd ${ROOT}/usr/src; tar cfb /dev/r${TAPE} 20 \
		Makefile asm.sed asm.sed.pdp asm.sed.vax bin etc games \
		lib libexec local man new old sbin share ucb usr.bin \
		usr.lib usr.sbin

tape1: ${ALL} ${DUMP}
	./maketape /dev/nr${TAPE} maketape.data
	dd if=${DUMP} of=/dev/nr${TAPE} bs=20b
	cd ${ROOT}/usr; tar cfb /dev/nr${TAPE} 20 \
		adm bin crash dict doc games guest hosts include ingres \
		lib libdata libexec local man msgs new old preserve pub \
		sbin share spool tmp ucb
	cd ${ROOT}/usr/src; tar cfb /dev/nr${TAPE} 20 \
		sys include

switch_tapes: FRC
	@echo "Switch tapes.  Hit CR when second tape is mounted."
	@head -1

FRC:

tape2:
	cd ${ROOT}/usr/src; tar cfb /dev/r${TAPE} 20 \
		Makefile asm.sed asm.sed.pdp asm.sed.vax bin etc games \
		lib libexec local man new old sbin share ucb usr.bin \
		usr.lib usr.sbin

${DUMP}:
	@echo "You must make a dump of the root file system first."
	exit 1

dump:
	dump 0f ${DUMP} /dev/r${DISK}

install: boot
	install -s boot ${ROOT}/


libsa.a: ${DRIVERS}
	ar rv $@ $?
	ranlib $@

mkfs.o: ${MKFS}
	cc ${CFLAGS} -c ${MKFS}

restor.o: ${RESTOR}
	cc ${CFLAGS} -c ${RESTOR}

icheck.o: ${ICHECK}
	cc ${CFLAGS} -c ${ICHECK}

mtboot: mtboot.o
	strip $@.o
	dd if=$@.o of=mtboot bs=16 skip=1
	rm -f $@.o

boot: M.o conf.o boot.o ubmapset.o libsa.a
	ld -X -o $@ M.o conf.o boot.o ubmapset.o libsa.a -lc

mkfs: srt0.o conf.o libsa.a mkfs.o
	ld -X -o $@ srt0.o conf.o $@.o libsa.a -lc

restor: srt0-i.o conf.o libsa.a restor.o
	ld -X -i -o $@ srt0-i.o conf.o $@.o libsa.a -lc

icheck: srt0-i.o conf.o libsa.a icheck.o
	ld -X -i -o $@ srt0-i.o conf.o $@.o libsa.a -lc

disklabel: srt0.o conf.o libsa.a disklabel.o displaylab.o
	ld -X -o $@ srt0.o conf.o $@.o displaylab.o libsa.a -lc

maketape: maketape.c
	cc -O -o $@ maketape.c

makesimtape: makesimtape.c
	cc -O -o $@ makesimtape.c

toyset: toyset.o srt0.o conf.o libsa.a
	ld -X -o $@ srt0.o conf.o $@.o libsa.a -lc

tags: FRC
	rm -f tags
	ctags *.[ch]
	for i in M.s; do \
		${EGREP} '^[_a-zA-Z][_a-zA-Z0-9]*:' $$i \
		| ${SED} "s;\([^:]*\):\(.*\)$$;\1	$$i	/^\1:\2$$/;" \
		>> tags; \
	done
	sort -o tags tags

FRC:

clean:
	rm -f ${ALL} ${AUX} libsa.a *.o tags

boot.o:		boot.c
conf.o:		conf.c
cat.o:		cat.c
displaylab.o:	displaylab.c
disklabel.o:	disklabel.c
hk.o:		hk.c
ht.o:		ht.c
label.o:	label.c
mtboot.o:	mtboot.s
prf.o:		prf.c
rk.o:		rk.c
rl.o:		rl.c
rx.c:		rx.c
br.o:		br.c
srt0.o:		srt0.s
srt0-i.o:	srt0.s
sys.o:		sys.c
tm.o:		tm.c
tmscp.o:	tmscp.c
ts.o:		ts.c
ubmapset.o:	ubmapset.c
xp.o:		xp.c

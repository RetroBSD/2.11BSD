#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.1.1 (2.11BSD) 1996/10/24
#
#
# makefile for line printer filters and associated programs
#

DESTDIR=
CFLAGS=	-O
SEPFLAG=-i
LIBEXECDIR=	/usr/libexec/lpr
CXREF=	/usr/ucb/ctags -x
#
# vpsf, fcvt and vdmp were excluded on the PDP because they require more than
# 64K of bss and I don't have time to figure out how it works.
#
SRCS=	lpf.c necf.c \
	chrtab.c rvsort.c rvcat.c vsort.c vcat.c vpf.c vpsf.c vdmp.c vpltdmp.c \
	vplotf.c vfontinfo.c vwidth.c vfw.c fcvt.c rotate.c rotprt.c
PRINTER=lpf necf
VARIAN=	rvsort rvcat vsort vcat vpf vpltdmp vplotf vfontinfo vwidth \
	vfw rotate rotprt

all:	printer varian

printer: ${PRINTER}

varian:	${VARIAN}

lpf:	lpf.c
	${CC} -o lpf ${CFLAGS} ${SEPFLAG} lpf.c

necf:	necf.c
	${CC} -o necf ${CFLAGS} ${SEPFLAG} necf.c

rvsort:	rvsort.c
	${CC} -o rvsort ${CFLAGS} ${SEPFLAG} rvsort.c

rvcat:	rvcat.c
	${CC} -o rvcat ${CFLAGS} ${SEPFLAG} rvcat.c

vsort:	vsort.c
	${CC} -o vsort ${CFLAGS} ${SEPFLAG} vsort.c

vcat:	vcat.c
	${CC} -o vcat ${CFLAGS} ${SEPFLAG} vcat.c

vpf:	vpf.c
	${CC} -o vpf ${CFLAGS} ${SEPFLAG} vpf.c

vpsf:	vpsf.c
	${CC} -o vpsf ${CFLAGS} ${SEPFLAG} vpsf.c

vdmp: vdmp.c
	${CC} -o vdmp ${CFLAGS} ${SEPFLAG} vdmp.c

vpltdmp: vpltdmp.c
	${CC} -o vpltdmp ${CFLAGS} ${SEPFLAG} vpltdmp.c

vplotf: vplotf.c chrtab.o
	${CC} -o vplotf ${CFLAGS} ${SEPFLAG} vplotf.c chrtab.o

vfontinfo: vfontinfo.c
	${CC} -o vfontinfo ${CFLAGS} ${SEPFLAG} vfontinfo.c

vwidth: vwidth.c
	${CC} -o vwidth ${CFLAGS} ${SEPFLAG} vwidth.c

vfw: vfw.c
	${CC} -o vfw ${CFLAGS} ${SEPFLAG} vfw.c

fcvt: fcvt.c
	${CC} -o fcvt ${CFLAGS} ${SEPFLAG} fcvt.c

rotate: rotate.c
	${CC} -o rotate ${CFLAGS} ${SEPFLAG} rotate.c

rotprt: rotprt.c
	${CC} -o rotprt ${CFLAGS} ${SEPFLAG} rotprt.c

install: printer
	-mkdir -p ${DESTDIR}/${LIBEXECDIR}
	install -s lpf ${DESTDIR}/${LIBEXECDIR}/lpf
	install -s necf ${DESTDIR}/${LIBEXECDIR}/necf

installv: varian
	-mkdir -p ${DESTDIR}/${LIBEXECDIR}
	install -s vfontinfo ${DESTDIR}/${LIBEXECDIR}/vfontinfo
	install -s vwidth ${DESTDIR}/${LIBEXECDIR}/vwidth
	install -s vfw ${DESTDIR}/${LIBEXECDIR}/vfw
	install -s rotate ${DESTDIR}/${LIBEXECDIR}/rotate
	install -s rotprt ${DESTDIR}/${LIBEXECDIR}/rotprt
	install -s rvsort ${DESTDIR}/${LIBEXECDIR}/rvsort
	install -s rvcat ${DESTDIR}/${LIBEXECDIR}/rvcat
	install -s vsort ${DESTDIR}/${LIBEXECDIR}/vsort
	install -s vcat ${DESTDIR}/${LIBEXECDIR}/vcat
	install -s vpf ${DESTDIR}/${LIBEXECDIR}/vpf
	ln ${DESTDIR}/${LIBEXECDIR}/vpf ${DESTDIR}/${LIBEXECDIR}/vpfW
	install -s vdmp ${DESTDIR}/${LIBEXECDIR}/vdmp
	install -s vpltdmp ${DESTDIR}/${LIBEXECDIR}/vpltdmp
	install -s vplotf ${DESTDIR}/${LIBEXECDIR}/vplotf

#	install -s fcvt ${DESTDIR}/${LIBEXECDIR}/fcvt
#	install -s vpsf ${DESTDIR}/${LIBEXECDIR}/vpsf

clean:
	rm -f ${PRINTER} ${VARIAN} *.o

print:
	@pr makefile
	@${CXREF} *.c | pr -h XREF
	@pr *.h *.c

# m11 Makefile.  Revised 1996/1/28
#
# Note that the pathname for macxrf is wired into exec.m11 ...

DESTDIR=

M11=	./m11.x
L11=	./l11.x

SEPFLAG=-i
MFLAGS=

.SUFFIXES: .m11 .obj

.m11.obj:
	${M11} ${MFLAGS} $<

.SUFFIXES: .1 .0

.1.0:
	/usr/man/manroff $*.1 > $*.0

SRCS=	exec.m11 macro.m11 code.m11 expr.m11 fltg.m11 getl.m11 \
	lout.m11  mac.m11 srch.m11 xlat.m11 misc.m11 pst.m11 \
	errs.m11 debug.m11 often.m11 xpcor.m11 syscalls.m11 at.sml
OBJS=	exec.obj macro.obj code.obj expr.obj fltg.obj getl.obj \
	lout.obj mac.obj srch.obj xlat.obj misc.obj pst.obj \
	errs.obj debug.obj often.obj xpcor.obj syscalls.obj

all: m11 macxrf m11.0

m11: ${OBJS}
	${L11} -ls ${SEPFLAG} ${OBJS}
	mv exec.out m11

macxrf: macxrf.c
	cc ${SEPFLAG} -O -w -o macxrf macxrf.c

install: all m11.0
	install -s m11 ${DESTDIR}/usr/new/m11
	install -s macxrf ${DESTDIR}/usr/new/macxrf
	install -c -m 444 -o bin -g bin m11.0 ${DESTDIR}/usr/new/man/cat1/m11.0

cmp: all
	cmp m11	${DESTDIR}/usr/new/m11
	cmp macxrf ${DESTDIR}/usr/new/macxrf

clean:
	rm -f *.obj *.lst *.map m11 macxrf *.0

#
# px makefile
#

DESTDIR=

AS=	as -V -

DEFS=		-DHZ=60.0
CFLAGS=		-O -w
LDFLAGS=	-X
SEPFLAG=	-i

SRCS=	00int.s 02rel.s 02relset.s 03bool.s 04as.s 05lv.s  \
	06add.s 07sub.s 10mul.s 12div.s 13mod.s 14neg.s  \
	16dvd.s 17ind.s 17rv.s 20con.s 21rang.s 24case.s 24pxp.s 25set.s  \
	26for.s 27conv.s 30atof.s 30getname.s 30io.s 30iosubs.s 30read.s  \
	30write.s 34fun.s opcode.s

OTHERS= Edata Emake.ed exit.c int.c palloc.c pcttot.c pdattim.c perror.c \
	pwrite.c gather.c

OBJS=	exit.o int.o palloc.o pcttot.o pdattim.o perror.o pwrite.o \
	getc.o putc.o

ALL=	px gather

all: ${ALL}

.c.o:
	${CC} ${CFLAGS} ${DEFS} -c $*.c

.s.o:
	${AS} -o $*.o $*.s

px: as.o ${OBJS}
	ld ${SEPFLAG} ${LDFLAGS} -o px /lib/crt0.o as.o ${OBJS} -lm -lc

gather: gather.o
	cc ${SEPFLAG} -o gather gather.o

as.o: E.s ${SRCS}
	${AS} -o as.o E.s ${SRCS}

int.c palloc.c pcttot.c perror.c: E.h

E.s E.h: Edata Emake.ed
	ed - Edata < Emake.ed

install: ${ALL}
	install -s px ${DESTDIR}/usr/ucb/px
	install -s gather ${DESTDIR}/usr/libexec/gather

tags: FRC

FRC:

clean:
	rm -f ${ALL} core E.h E.s *.o tags

print:
	@ls -ls | pr
	@pr README Makefile
	@pr 0x.h int.c exit.c
	@pr [0-9][0-9]*.s
	@pr p*.c

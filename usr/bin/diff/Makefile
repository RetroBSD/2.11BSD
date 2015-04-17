#	@(#)Makefile	4.1.1	1996/10/21
#
DIFF=	/bin/diff
DIFFH=	/usr/libexec/diffh
PR=	/bin/pr
CFLAGS=	-O -DDIFF='"${DIFF}"' -DDIFFH='"${DIFFH}"' -DPR='"${PR}"'
SEPFLAG=-i
DESTDIR=

SRCS=	diff.c diffdir.c diffreg.c
HDRS=	diff.h
OBJS=	diff.o diffdir.o diffreg.o

all: diff diffh

diff: ${OBJS}
	cc ${CFLAGS} -o diff ${OBJS} ${SEPFLAG} ${LDFLAGS}

diffh: diffh.o
	cc -o diffh diffh.o ${SEPFLAG} ${LDFLAGS}

clean:
	rm -f diff diffh ${OBJS} diffh.o

install: all
	install -s diff ${DESTDIR}/bin/diff
	install -s diffh ${DESTDIR}/usr/libexec/diffh

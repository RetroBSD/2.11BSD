#
# bm Makefile - revised 1996/1/28
#
DESTDIR=

CFLAGS = -O
SEPFLAG = -i

HDRS=	Extern.h bm.h
SRCS=	Execute.c GetPatFile.c Global.c MakeDesc.c MakeSkip.c MatchFound.c \
	MkDescVec.c MoveResidue.c PrintLine.c PutUsage.c Search.c bm.c
OBJS=	Execute.o GetPatFile.o Global.o MakeDesc.o MakeSkip.o MatchFound.o \
	MkDescVec.o MoveResidue.o PrintLine.o PutUsage.o Search.o bm.o

.SUFFIXES: .1 .0

.1.0:
	/usr/man/manroff $*.1 > $*.0

bm: ${OBJS} bm.0
	cc -o $@ ${SEPFLAG} ${OBJS}

install: bm bm.0
	install -s bm ${DESTDIR}/usr/new/bm
	install -c -m 444 -o bin -g bin  bm.0 ${DESTDIR}/usr/new/man/cat1/bm.0

tags: ${HDRS} ${SRCS}
	ctags $<

clean:
	rm -f *.o bm tags core *.0

bm.o:		bm.c bm.h Extern.h
PutUsage.o:	PutUsage.c bm.h 
MakeSkip.o:	MakeSkip.c bm.h 
Search.o:	Search.c bm.h Extern.h
Execute.o:	Execute.c bm.h 
MoveResidue.o:	MoveResidue.c bm.h Extern.h
MatchFound.o:	MatchFound.c bm.h Extern.h
PrintLine.o:	PrintLine.c Extern.h
MkDescVec.o:	MkDescVec.c bm.h
GetPatFile.o:	GetPatFile.c bm.h
MakeDesc.o:	MakeDesc.c bm.h
Global.o:	Global.c


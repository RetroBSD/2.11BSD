#
# pxp makefile
#

DESTDIR=

ED=	-ed
AS=	as -V -
EYACC=	../eyacc/eyacc

DEFS=		-DPXP
CFLAGS=		-O -w
LDFLAGS=	-X
SEPFLAG=	-i

HDRS=	0.h opcode.h tree.h yy.h

SRCS=	TRdata.c Version.c call.c case.c const.c cset.c error.c fdec.c \
	func.c hash.c lab.c lval.c main.c nl.c pmon.c pp.c proc.c rec.c \
	rval.c stat.c string.c subr.c tree.c type.c var.c yycomm.c yycosts.c \
	yyerror.c yyget.c yyid.c yylex.c yymain.c yypanic.c yyparse.c \
	yyprint.c yyput.c yyrecover.c yyseman.c yytree.c

OTHERS=	gram.ex how_pxp pas.y printf.s treen.s yycopy.s \
	../px/getc.s ../px/putc.s

OBJS=	TRdata.o Version.o call.o case.o const.o cset.o error.o fdec.o \
	func.o hash.o lab.o lval.o main.o nl.o pmon.o pp.o proc.o rec.o \
	rval.o stat.o string.o subr.o tree.o type.o var.o yycomm.o yycosts.o \
	yyerror.o yyget.o yyid.o yylex.o yymain.o yypanic.o yyparse.o \
	yyprint.o yyput.o yyrecover.o yyseman.o yytree.o

OBJS2=	printf.o treen.o yycopy.o y.tab.o getc.o putc.o

VPATH=.:../px

all: pxp

.c.o:
	${CC} ${CFLAGS} ${DEFS} -c $*.c

.s.o:
	${AS} -o $*.o $*.s

pxp: y.tab.h ${OBJS} ${OBJS2}
	${CC} ${CFLAGS} ${DEFS} -c Version.c
	${CC} ${SEPFLAG} ${LDFLAGS} -o pxp ${OBJS} ${OBJS2} Version.o

y.tab.c y.tab.h: pas.y
	${EYACC} -v pas.y
	${ED} - y.tab.c < gram.ex

install: pxp how_pxp
	install -s pxp ${DESTDIR}/usr/ucb/pxp
	install -c how_pxp ${DESTDIR}/usr/share/pascal/how_pxp

tags: FRC
	ctags ${HDRS} ${SRCS}

FRC:

clean:
	rm -f pi1.2strings a.out core y.tab.c y.output *.o x* tags

print:
	@ls -ls | pr
	@pr READ_ME Makefile
	@pr 0.h main.c pas.y
	@pr yy.h yy*.c
	@pr [a-ln-x]*.c
	@pr treen.s yycopy.s

CFLAGS=	-O
SEPFLAG= -i
LIB=	/lib
ALL=	tst_adec tst_rhh-g tst_pntint tst_asm tst_lgm-g \
	tst_sgncmp tst_assign old-assn

all: ${ALL}

tst_adec: tst_adec.o
	${CC} -o $@ tst_adec.o

# This will fail... see README
#tst_rhh: tst_rhh.o
#	${CC} -o $@ tst_rhh.o

tst_rhh-g: tst_rhh-g.o
	${CC} -o $@ tst_rhh-g.o

tst_pntint: tst_pntint.o
	${CC} -o $@ tst_pntint.o

tst_asm: tst_asm.o
	${CC} -o $@ tst_asm.o

# This requires changes to the compiler to run, see README
#tst_longsym: tst_longsym.o
#	${CC} -o $@ tst_longsym.o

tst_longsym: tst_longsym.o
	${CC} -o $@ tst_longsym.o

tst_lgm: tst_lgm.o
	${CC} -o $@ tst_lgm.o

# This will fail... see README
#tst_lgmgood: tst_lgmgood.o
#	${CC} -o $@ tst_lgmgood.o

tst_assign: tst_assign.o
	${CC} -o $@ tst_assign.o

tst_sgncmp: tst_sgncmp.o
	${CC} -o $@ tst_sgncmp.o

old-assn: old-assn.l
	lex old-assn.l
	${CC} ${SEPFLAG} ${CFLAGS} -o old-assn lex.yy.c -lln
	rm lex.yy.c

clean:
	rm -f lex.yy.c *.o core ${ALL}

install:

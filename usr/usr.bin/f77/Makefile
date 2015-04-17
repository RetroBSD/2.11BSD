#	Makefile for the Fortran 77 compiler
#	generating code for the PDP11,
#	using the Ritchie pass 2
#
#	1996/10/25 - 2.11BSD
#

AS=		as -V
C2=		/lib/c2
LD=		ld
STRINGS=	f77_strings
CFL=		-DTARGET=PDP11 -DFAMILY=DMR -DHERE=PDP11 -DOUTPUT=BINARY \
		-DPOLISH=POSTFIX -DOVERLAID -DC_OVERLAY
CFLAGS=		-w ${CFL}		# Don't use -O here.
SEPFLAG= -i

SRCS= 	data.c driver.c equiv.c error.c exec.c expr.c gram.c init.c \
	intr.c io.c lex.c main.c malloc.c misc.c pdp11.c pdp11x.c \
	proc.c put.c putdmr.c

PASS1OBJECTS=	data.o equiv.o error.o exec.o expr.o gram.o init.o \
		intr.o io.o lex.o main.o misc.o pdp11.o pdp11x.o proc.o \
		put.o putdmr.o

all:		f77 f77pass1

f77:		driver.c pdp11x.c
		${CC} ${SEPFLAG} ${CFLAGS} driver.c pdp11x.c -o f77
		@size f77

f77pass1:	${PASS1OBJECTS}
		-if [ X${SEPFLAG} = X-i ]; then \
			${LD} /lib/crt0.o ${SEPFLAG} -o f77pass1 \
			main.o misc.o gram.o lex.o \
			-Z proc.o init.o \
			-Z equiv.o data.o error.o \
			-Y io.o expr.o intr.o exec.o \
			put.o putdmr.o pdp11.o pdp11x.o -lc; \
		else \
			${LD} /lib/crt0.o ${SEPFLAG} -o f77pass1 \
			-Z main.o init.o equiv.o data.o io.o \
			-Z gram.o \
			-Z lex.o proc.o \
			-Z expr.o \
			-Z exec.o intr.o \
			-Z putdmr.o pdp11.o pdp11x.o \
			-Y misc.o error.o put.o -lc; \
		fi
		@size f77pass1

# If non-separate I&D load, take certain data tables out of data space and
# put them in text space (warning-- this doesn't work except with
# special cases)

gram.o:		gram.c
		mkstr - f77_strings xx gram.c
		${CC} -S ${CFLAGS} xxgram.c 
		mv xxgram.s gram.s
#	Eliminate yacc data structures
		-if [ X${SEPFLAG} = X-n ]; then ed - gram.s < :gram.fix; \
			else :; fi
		rm xxgram.c
		${AS} -u -o gram.o gram.s
		rm gram.s

gram.c:		gram.head gram.dcl gram.expr gram.exec gram.io tokdefs
		( sed <tokdefs "s/#define/%token/" ;\
		cat gram.head gram.dcl gram.expr gram.exec gram.io ) >gram.in
		${YACC} ${YFLAGS} gram.in
		mv y.tab.c gram.c
		rm gram.in

.c.o:
#	Remove error strings
		mkstr - f77_strings xx $*.c
		${CC} -S ${CFLAGS} xx$*.c
#	Move switch code to text space
		-if [ X${SEPFLAG} = X-n ]; \
			then ed - xx$*.s < :rofix; else :; fi
		${C2} xx$*.s | ${AS} -u -o $*.o
		rm xx$*.c xx$*.s

driver.o:
pdp11x.o:
		${CC} -c ${CFLAGS} $*.c

defs:		ftypes defines locdefs
		touch defs

tokdefs:	tokens
		grep -n . <tokens | sed "s/\([^:]*\):\(.*\)/#define \2 \1/" >tokdefs

locdefs:	pdp11defs
		cp pdp11defs locdefs

install: all
		install -s f77 ${DESTDIR}/usr/bin
		install -s f77pass1 ${DESTDIR}/usr/libexec
		install -c -m 644 ${STRINGS} ${DESTDIR}/usr/share/misc

clean:
		-rm -f f77 f77pass1 *.o gram.c x.c xs.c f77_strings strings

tags:
	cwd=`pwd`; \
	for i in ${SRCS}; do \
		ctags -a -f tags $$cwd/$$i; \
	done


data.o:		defs string_defs data.c
driver.o:	defs drivedefs driver.c
equiv.o:	defs string_defs equiv.c
error.o:	defs string_defs error.c
exec.o:		defs string_defs exec.c
expr.o:		defs string_defs expr.c
gram.o:		defs string_defs gram.c
init.o:		defs string_defs init.c
intr.o:		defs string_defs intr.c
io.o:		defs string_defs io.c
lex.o:		defs string_defs tokdefs lex.c
main.o:		defs string_defs main.c
misc.o:		defs string_defs misc.c
pdp11.o:	defs string_defs pdp11.c
proc.o:		defs string_defs proc.c
put.o:		defs string_defs put.c
putdmr.o:	defs string_defs dmrdefs putdmr.c

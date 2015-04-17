#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.6.1 (2.11BSD GTE) 4/9/94
#
#	crt0	Normal C run time startoff
#	mcrt0	C run time start off for profiling, ``prof'' conventions
#	gcrt0	C run time start off for profiling, ``gprof'' conventions
#
DESTDIR=
SRCS=	crt0.s mon.c mcount.s gmon.c
#OBJS=	crt0.o mcrt0.o gcrt0.o mon.o gmon.o
OBJS=	crt0.o mcrt0.o mon.o
CFLAGS=	-O ${DEFS}
TAGSFILE=tags

all: ${OBJS}

install: ${OBJS}
	install -c -m 0644 crt0.o ${DESTDIR}/lib/crt0.o
	install -c -m 0644 mcrt0.o ${DESTDIR}/lib/mcrt0.o
#	install -c -m 0644 gcrt0.o ${DESTDIR}/usr/lib/gcrt0.o

crt0.o:	crt0.s
	/lib/cpp ${DEFS} ${DFLAGS} crt0.s | sed 's;^#;/;' | as -o x.o
	ld -x -r -o crt0.o x.o
	rm -f x.o

moncrt0.o: crt0.s
	/lib/cpp ${DEFS} ${DFLAGS} -DMCRT0 crt0.s | sed 's;^#;/;' | as -o x.o
	ld -x -r -o moncrt0.o x.o
	rm -f x.o

gcrt0.o: moncrt0.o gmon.o
	ld -x -r -o gcrt0.o moncrt0.o gmon.o

mcrt0.o: moncrt0.o mon.o
	ld -x -r -o mcrt0.o moncrt0.o mon.o

mon.o: mon.c mcount.s mon.ex
	cc ${CFLAGS} -S ${DFLAGS} mon.c
	ex - mon.s < mon.ex
	cat mon.s mcount.s > x.s
	/lib/cpp ${DEFS} ${DFLAGS} x.s | sed 's;^#;/;' > xx.s
	as - -o x.o xx.s
	ld -x -r -o mon.o x.o
	rm -f x.o x.s xx.s mon.s

gmon.o: gmon.c gmon.h gmon.ex
	cc ${CFLAGS} -S ${DFLAGS} gmon.c
	ex - gmon.s < gmon.ex
	as -o x.o gmon.s
	ld -x -r -o gmon.o x.o
	rm -f x.o gmon.s

tags:	
	cwd=`pwd`; \
	for i in *.c; do \
		ctags -a -f ${TAGSFILE} $$cwd/$$i; \
	done

clean:
	rm -f ${OBJS} *.o core errs tags Makefile.bak

lint:

depend:
	for i in ${SRCS}; do \
	    cc -M ${DEFS} ${DFLAGS} $$i | awk ' { if ($$1 != prev) \
		{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		else rec = rec " " $$2 } } \
		END { print rec } ' >> makedep; done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it

crt0.o: crt0.s
mon.o: mon.c
gmon.o: gmon.c ./gmon.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above

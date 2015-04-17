#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.6.1 (2.11BSD GTE) 4/9/94
#
# SRCS=	doprnt.s ffltpr.s fgetc.s fgets.s fltpr.s fputc.s puts.s gets.s
# OBJS=	doprnt.o ffltpr.o fgetc.o fgets.o fltpr.o fputc.o puts.o gets.o
SRCS=	doprnt.s ffltpr.s fgetc.s fgets.s fltpr.s fputc.s gets.s
OBJS=	doprnt.o ffltpr.o fgetc.o fgets.o fltpr.o fputc.o gets.o
CFLAGS= -O ${DEFS}
TAGSFILE=tags

.s.o:
	/lib/cpp -E -DPROF ${DEFS} $< | sed -e 's;^#;/;' | as -V -o $@
	-ld -X -r $*.o
	mv a.out profiled/$*.o
	/lib/cpp -E ${DEFS} $< | sed -e 's;^#;/;' | as -V -o $@
	-ld -x -r $*.o
	mv a.out $*.o

.c.o:
	cc -p -c ${CFLAGS} $*.c
	-ld -X -r $*.o
	mv a.out profiled/$*.o
	cc -c ${CFLAGS} $*.c
	-ld -x -r $*.o
	mv a.out $*.o

stdiolib stdiolib_p: ${OBJS}
	@echo "building profiled stdio"
	@cd profiled; ar cru ../stdiolib_p ${OBJS}
	@echo "building normal stdio"
	@ar cru stdiolib ${OBJS}

tags:

clean:
	rm -f *.o profiled/*.o errs a.out core stdiolib stdiolib_p \
		tags Makefile.bak

depend:
	for i in ${SRCS}; do \
	    cc -M ${CFLAGS} $$i | awk ' { if ($$1 != prev) \
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

doprnt.o: doprnt.s ./DEFS.h
ffltpr.o: ffltpr.s
fgetc.o: fgetc.s ./DEFS.h ./STDIO.h
fgets.o: fgets.s ./DEFS.h ./STDIO.h
fltpr.o: fltpr.s ./DEFS.h
fputc.o: fputc.s ./DEFS.h ./STDIO.h
gets.o: gets.s ./DEFS.h ./STDIO.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above

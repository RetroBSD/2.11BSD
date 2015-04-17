INGRES=	../..
CC=	cc
CFLAGS=	-O
AR=	ar
ARFLAGS=rvuc
LIB=	$(INGRES)/lib

OBJS=	printup.o resyncpipes.o rdpipe.o proc_error.o copypipes.o \
	wrpipe.o prpipe.o getuser.o getnuser.o initucode.o \
	initproc.o out_arg.o dblocks.o errfilen.o error.o \
	end_job.o ingresname.o printdesc.o paramd.o typeconv.o \
	typeunconv.o rub.o cslocks.o rllocks.o pglocks.o \
	clr_tuple.o trim_relname.o trbuild.o globals.o \
	version.o reset.o

iutil:	$(OBJS)
	-$(AR) $(ARFLAGS) iutil $(OBJS)
	ranlib iutil

install: iutil
	install -c -m 644 iutil $(LIB)/iutil
	ranlib $(LIB)/iutil

clean:
	rm -f iutil *.o *.core a.out

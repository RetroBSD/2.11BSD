#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	7.13.1.3 (2.11BSD GTE) 1996/10/23
#
VERSION=3.7
#
# NB: This makefile doesn't indicate any dependencies on header files.
#
# Ex is very large - this version will not fit on PDP-11's without overlay
# software.  Things that can be turned off to save
# space include LISPCODE (-l flag, showmatch and lisp options), CHDIR (the 
# undocumented chdir command.)  VMUNIX makes ex considerably larger, raising 
# many limits and improving speed and simplicity of maintenance.  It is 
# suitable only for a VAX or other large machine, and then probably only in 
# a paged system.
#
# Don't define VFORK unless your system has the VFORK system call,
# which is like fork but the two processes have only one data space until the
# child execs. This speeds up ex by saving the memory copy.
#
# If your system expands tabs to 4 spaces you should -DTABS=4 below
#
BINDIR=	/usr/ucb
SBINDIR= /usr/sbin
MISCDIR= /usr/share/misc
FOLD=	${BINDIR}/fold
CTAGS=	${BINDIR}/ctags
XSTR=	${BINDIR}/xstr
DEBUGFLAGS=	-DTRACE -g
NONDEBUGFLAGS=	-O
DEB=	${NONDEBUGFLAGS}	# or ${DEBUGFLAGS} to to debug
#OPTIONS= -DLISPCODE -DCHDIR -DVFORK -DVMUNIX -DFASTTAG
OPTIONS= -DLISPCODE -DCHDIR -DVFORK -DFASTTAG
CFLAGS=	-DTABS=8 ${OPTIONS} ${DEB}
LDFLAGS= -X
SEPFLAG= -i
TERMLIB=	-ltermcap
MKSTR=	${BINDIR}/mkstr
CXREF=	${BINDIR}/cxref
INCLUDE=/usr/include
PR=	pr
OBJS=	ex.o ex_addr.o ex_cmds.o ex_cmds2.o ex_cmdsub.o \
	ex_data.o ex_extern.o ex_get.o ex_io.o ex_put.o ex_re.o \
	ex_set.o ex_subr.o ex_tagio.o ex_temp.o ex_tty.o ex_unix.o \
	ex_v.o ex_vadj.o ex_vget.o ex_vmain.o ex_voper.o \
	ex_vops.o ex_vops2.o ex_vops3.o ex_vput.o ex_vwind.o \
	printf.o strings.o
HDRS=	ex.h ex_argv.h ex_re.h ex_temp.h ex_tty.h ex_tune.h ex_vars.h ex_vis.h
SRC1=	ex.c ex_addr.c ex_cmds.c ex_cmds2.c ex_cmdsub.c
SRC2=	ex_data.c ex_get.c ex_io.c ex_put.c ex_re.c
SRC3=	ex_set.c ex_subr.c ex_tagio.c ex_temp.c ex_tty.c ex_unix.c
SRC4=	ex_v.c ex_vadj.c ex_vget.c ex_vmain.c ex_voper.c
SRC5=	ex_vops.c ex_vops2.c ex_vops3.c ex_vput.c ex_vwind.c
SRC6=	printf.c expreserve.c exrecover.c
MISC=	makefile READ_ME
VGRIND=	csh /usr/ucb/vgrind
VHDR=	"Ex Version ${VERSION}"

.c.o:
# ifdef VMUNIX
#	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
# else
	${MKSTR} - exstrings x $*.c
	${CC} -E ${CFLAGS} x$*.c | ${XSTR} -c -
	rm -f x$*.c
# endif
	${CC} ${CFLAGS} -c x.c 
	mv x.o $*.o

all:	a.out exrecover expreserve

# 11/23, 34, 40 and other non split I/D machines
# each of the 6 overlays must stay less than 16K.

a.out: ${OBJS}
	-if [ X${SEPFLAG} = X-i ]; then \
		ld ${LDFLAGS} /lib/crt0.o ${SEPFLAG} \
		    -Z ex_voper.o ex_vget.o ex_vops.o ex_vops2.o ex_vops3.o \
		    -Z ex_set.o ex_re.o ex_io.o ex_tty.o ex_unix.o ex.o ex_v.o \
		    -Y ex_addr.o ex_cmds.o ex_cmds2.o ex_cmdsub.o ex_tagio.o \
		       ex_get.o ex_temp.o ex_vadj.o ex_vmain.o ex_vwind.o\
		       ex_vput.o ex_put.o ex_subr.o printf.o strings.o \
		       ex_data.o ex_extern.o ${TERMLIB} -lc; \
	else \
		ld ${LDFLAGS} /lib/crt0.o ${SEPFLAG} \
		    -Z ex_cmds.o ex_cmds2.o ex_cmdsub.o ex_tagio.o ex_re.o \
		    -Z ex_vadj.o ex_vmain.o ex_voper.o ex_vwind.o ex_vops3.o\
		    -Z ex_v.o ex_vget.o ex_vops.o ex_vops2.o ex_vput.o\
		    -Z ex_get.o ex_io.o ex_temp.o ex_tty.o ex_unix.o \
			ex_addr.o ex.o ex_set.o \
		    -Z ex_put.o ex_subr.o ${TERMLIB} \
		    -Y ex_data.o ex_extern.o printf.o strings.o -lc; \
	fi

tags:	/tmp
	${CTAGS} -w ex.[hc] ex_*.[hc]

${OBJS}: ${HDRS}

ex_vars.h:
	csh makeoptions ${CFLAGS}

# xstr: hands off!
strings.o: strings
	${XSTR}
	${CC} -c -o strings.o xs.c
	
exrecover: exrecover.o ex_extern.o
	${CC} ${SEPFLAG} ${CFLAGS} exrecover.o ex_extern.o -o exrecover

exrecover.o: exrecover.c
	${CC} ${CFLAGS} -c -O exrecover.c

expreserve: expreserve.o
	${CC} ${SEPFLAG} expreserve.o -o expreserve

expreserve.o:
	${CC} ${CFLAGS} -c -O expreserve.c

clean:
#	If we dont have ex we cant make it so don't rm ex_vars.h
	-rm -f a.out exrecover expreserve exstrings strings
	-rm -f *.o x*.[cs] core errs trace

# install in standard place (/usr/ucb)
install: a.out exrecover expreserve
	-rm -f ${DESTDIR}/${BINDIR}/ex
	-rm -f ${DESTDIR}/${BINDIR}/vi
	-rm -f ${DESTDIR}/${BINDIR}/view
	-rm -f ${DESTDIR}/usr/bin/ex
	install -s -m 1755 a.out ${DESTDIR}/${BINDIR}/ex
	install -c -m 444  exstrings ${DESTDIR}/${MISCDIR}/exstrings
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/vi
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/${BINDIR}/view
	ln ${DESTDIR}/${BINDIR}/ex ${DESTDIR}/usr/bin/ex
	chmod 1755 ${DESTDIR}/${BINDIR}/ex
	install -s -o root -m 4755 exrecover ${DESTDIR}/${SBINDIR}/exrecover
	install -s -o root -m 4755 expreserve ${DESTDIR}/${SBINDIR}/expreserve
# The following line normally fails.  This is OK.
	-mkdir ${DESTDIR}/usr/preserve

lint:
	lint ${CFLAGS} ex.c ex_?*.c
	lint ${CFLAGS} -u exrecover.c
	lint ${CFLAGS} expreserve.c

print:
	@${PR} READ* BUGS
	@${PR} makefile*
	@${PR} /etc/termcap
	@(size -l a.out ; size *.o) | ${PR} -h sizes
	@${PR} -h errno.h ${INCLUDE}/errno.h
	@${PR} -h setjmp.h ${INCLUDE}/setjmp.h
	@${PR} -h sgtty.h ${INCLUDE}/sgtty.h
	@${PR} -h signal.h ${INCLUDE}/signal.h
	@${PR} -h sys/stat.h ${INCLUDE}/sys/stat.h
	@${PR} -h sys/types.h ${INCLUDE}/sys/types.h
	@ls -ls | ${PR}
	@${CXREF} *.c | ${PR} -h XREF
	@${PR} *.h *.c

vgrind:
	tee index < /dev/null
	${VGRIND} -h ${VHDR} ${HDRS}
	${VGRIND} -h ${VHDR} ${SRC1}
	${VGRIND} -h ${VHDR} ${SRC2}
	${VGRIND} -h ${VHDR} ${SRC3}
	${VGRIND} -h ${VHDR} ${SRC4}
	${VGRIND} -h ${VHDR} ${SRC5}
	${VGRIND} -h ${VHDR} ${SRC6}
	${VGRIND} -n -h ${VHDR} ${MISC}
	${VGRIND} -i -h ${VHDR} index

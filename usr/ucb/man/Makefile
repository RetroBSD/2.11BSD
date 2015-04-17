#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	5.5.2 (2.11BSD) 1996/12/1
#
CFLAGS=	-O
SEPFLAG= -i
ASRCS=	apropos.c
AOBJS=	apropos.o
MSRCS=	man.c
MOBJS=	man.o

all: man apropos

apropos: ${AOBJS}
	${CC} ${SEPFLAG} -o $@ ${AOBJS}

man: ${MOBJS}
	${CC} ${SEPFLAG} -o $@ ${MOBJS}

clean: FRC
	rm -f core apropos man *.o

depend: FRC
	mkdep ${CFLAGS} ${ASRCS} ${MSRCS}

install: man apropos
	rm -f ${DESTDIR}/usr/ucb/whatis ${DESTDIR}/usr/ucb/apropos
	install -s -o bin -g bin -m 755 apropos ${DESTDIR}/usr/ucb/apropos
	install -s -o bin -g bin -m 755 man ${DESTDIR}/usr/ucb/man
	ln ${DESTDIR}/usr/ucb/apropos ${DESTDIR}/usr/ucb/whatis

lint: FRC
	lint ${CFLAGS} ${ASRCS}
	lint ${CFLAGS} ${MSRCS}

tags: FRC
	ctags ${ASRCS}
	ctags ${MSRCS}
	sort -o tags tags

FRC:
# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

apropos.o: apropos.c /usr/include/sys/param.h /usr/include/sys/localopts.h
apropos.o: /usr/include/sys/stddef.h /usr/include/machine/machparam.h
apropos.o: /usr/include/sys/types.h /usr/include/signal.h
apropos.o: /usr/include/sys/types.h /usr/include/stdio.h /usr/include/ctype.h
apropos.o: /usr/include/strings.h /usr/include/string.h
apropos.o: /usr/include/sys/types.h
man.o: man.c /usr/include/sys/param.h /usr/include/sys/localopts.h
man.o: /usr/include/sys/stddef.h /usr/include/machine/machparam.h
man.o: /usr/include/sys/types.h /usr/include/signal.h /usr/include/sys/types.h
man.o: /usr/include/sys/file.h /usr/include/fcntl.h /usr/include/sys/types.h
man.o: /usr/include/sys/dir.h /usr/include/stdio.h /usr/include/ctype.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY

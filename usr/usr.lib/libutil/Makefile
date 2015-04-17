#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such redistribution and
# use acknowledge that the software was developed by the University
# of California, Berkeley.  The name of the University may not be
# used to endorse or promote products derived from this software
# without specific prior written permission.  THIS SOFTWARE IS PROVIDED
# ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.
#
# @(#)Makefile	5.4.1 (2.11BSD GTE) 1995/04/13
#

CFLAGS=	-O ${DEFS}
SRCS=	login.c logout.c logwtmp.c dkcksum.c
OBJS=	login.o logout.o logwtmp.o dkcksum.o

.c.o:
	@${CC} -p ${CFLAGS} -c $*.c
	@-ld -X -o profiled/$*.o -r $*.o
	${CC} ${CFLAGS} -c $*.c
	@-ld -x -r $*.o
	@mv a.out $*.o

all: libutil.a libutil_p.a

libutil.a libutil_p.a: ${OBJS}
	@echo building normal libutil
	@ar ru libutil.a ${OBJS}
	ranlib libutil.a
	@echo building profiled libutil
	@cd profiled; ar ru ../libutil_p.a ${OBJS}
	ranlib libutil_p.a

clean:
	rm -f ${OBJS} profiled/*.o libutil.a libutil_p.a

cleandir: clean
	rm -f .depend

depend:
	mkdep ${CFLAGS} ${SRCS}

install: all
	install -o bin -g bin -m 644 libutil.a ${DESTDIR}/usr/lib
	ranlib -t ${DESTDIR}/usr/lib/libutil.a
	install -o bin -g bin -m 644 libutil_p.a ${DESTDIR}/usr/lib
	ranlib -t ${DESTDIR}/usr/lib/libutil_p.a

tags:
	ctags ${SRCS}

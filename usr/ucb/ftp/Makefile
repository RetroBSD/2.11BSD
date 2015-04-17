#
# Copyright (c) 1988 Regents of the University of California.
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
#	@(#)Makefile	5.9.2 (2.11BSD) 1996/12/1
#
CFLAGS=	-O -I.
SEPFLAG= -i
SRCS=	cmds.c cmdtab.c ftp.c glob.c main.c ruserpass.c domacro.c
OBJS=	cmds.o cmdtab.o ftp.o glob.o main.o ruserpass.o domacro.o
MAN=	ftp.0
MANSRC=	ftp.1

all: ftp ${MAN}

ftp: ${OBJS}
	${CC} ${SEPFLAG} -o $@ ${CFLAGS} ${OBJS}

ftp.0: ${MANSRC}
	/usr/man/manroff ${MANSRC} > ${MAN}

clean:
	rm -f ${OBJS} ${MAN} core ftp

cleandir: clean
	rm -f tags .depend

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

install: ftp ${MAN}
	install -s -o bin -g bin -m 751 ftp ${DESTDIR}/usr/ucb
	install -c -o bin -g bin -m 444 ${MAN} ${DESTDIR}/usr/man/cat1

lint: ${SRCS}
	lint ${CFLAGS} ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

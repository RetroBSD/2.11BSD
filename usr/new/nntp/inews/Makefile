#
# Makefile for pseudo-inews
#

SRCS	=	inews.c uname.c ../common/clientlib.c ../common/version.c
OBJS	=	inews.o uname.o ../common/clientlib.o ../common/version.o
HFILES	=	../common/nntp.h ../common/conf.h

CFLAGS	= -O

DESTDIR	= /usr/local/lib/rn

inews:  $(OBJS) $(HFILES)
	$(CC) $(CFLAGS) $(OBJS) -o inews

uname.o: ../common/conf.h

../common/clientlib.o:
	${CC} ${CFLAGS} -DNNTPSRC -c ../common/clientlib.c
	mv clientlib.o ../common/clientlib.o

../common/version.o:
	${CC} ${CFLAGS} -c ../common/version.c
	mv version.o ../common/version.o

install: inews
	cp inews ${DESTDIR}/inews
	chmod 755 ${DESTDIR}/inews

clean:
	-rm -f ${OBJS} inews make*.out

distrib: clean
	rm -f make.out
	rm -rf SCCS

check:
	sccs check
	sccs prt -y *.[ch] > /dev/null

lint:
	lint ${SRCS} ${HFILES}

tags:
	ctags $(CFILES) $(HFILES)

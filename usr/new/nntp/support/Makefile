#
# Makefile for NNTP server support programs
#

OBJS	=	mkgrdates.o

SRCS	=	mkgrdates.c

HFILES	=	../common/conf.h

CFLAGS= -O

# Where these support programs will live

DESTDIR	= 
BIN = /usr/lib/news

all: mkgrdates

mkgrdates: mkgrdates.o
	$(CC) ${CFLAGS} -o mkgrdates mkgrdates.o

install: mkgrdates
	install -s -m  755 ${DESTDIR}${BIN}/mkgrdates

lint:
	lint mkgrdates.c

clean:
	-rm -f *.o mkgrdates

distrib: clean
	rm -rf SCCS old

check:
	sccs check
	sccs prt -y *.[ch] > /dev/null

tags:	${SRCS} ${HFILES}
	ctags ${SRCS} ${HFILES}

${OBJS}:	${HFILES}

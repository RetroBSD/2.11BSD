#	@(#)Makefile	5.16.2 (2.11BSD) 1996/10/31

SRCS=	version.c aux.c cmd1.c cmd2.c cmd3.c cmdtab.c collect.c edit.c fio.c \
	getname.c head.c v7.local.c lex.c list.c main.c names.c popen.c \
	quit.c send.c strings.c temp.c tty.c vars.c
OBJS=	version.o aux.o cmd1.o cmd2.o cmd3.o cmdtab.o collect.o edit.o fio.o \
	getname.o head.o v7.local.o lex.o list.o main.o names.o popen.o \
	quit.o send.o strings.o temp.o tty.o vars.o str.o
#
CFLAGS=	-O
XSTR=	/usr/ucb/xstr
DESTDIR=
CTAGS=	/usr/ucb/ctags -w
LIBES=
AS=	-as
RM=	-rm
HDRS=	def.h glob.h pathnames.h rcv.h
ALL=	Mail fmt
S = $(SRCS) $(HDRS)

#
# Special massaging of C files for sharing of strings
#
.c.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
	${CC} -c ${CFLAGS} x.c 
	mv x.o $*.o
	@$(RM) -f x.c

all:	${ALL}

Mail:	$S $(OBJS)
	$(RM) -f Mail
	@echo Loading ...
	@$(CC) -i -o Mail $(OBJS) $(LIBES)
	@size Mail

install: all
	install -s Mail ${DESTDIR}/usr/ucb/Mail
	install -s fmt ${DESTDIR}/usr/ucb/fmt
	cd ${DESTDIR}/usr/ucb; rm -f mail; ln Mail mail
	install -c -m 444 Mail.rc ${DESTDIR}/etc/Mail.rc

tags:	$(SRCS)
	${CTAGS} $(SRCS);

clean:
	$(RM) -f *.o
	rm -f Mail a.out fmt x.c xs.c tags core xhead.c strings

lint:
	lint -hxa $(CFLAGS) $(SRCS)

fmt:	fmt.o xhead.o
	$(CC) $(CFLAGS) -i fmt.o xhead.o -o fmt
	size fmt

fmt.o:	fmt.c
	$(CC) $(CFLAGS) -c fmt.c

xhead.c:
	-ln -s head.c xhead.c

xhead.o: xhead.c
	$(CC) $(CFLAGS) -c xhead.c

str.o: strings
	$(XSTR)
	$(CC) -c xs.c
	mv xs.o str.o
	$(RM) xs.c

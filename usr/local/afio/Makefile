##
##  I wrote this Makefile, based on comments in the source.  -rich $alz.
##  Define INDEX to use index() in place of strchr() (v7, BSD).
1	= -DINDEX
#1	= -UINDEX
##  Define MEMCPY when an efficient memcpy() exists (SysV).
2	= -DMEMCPY
#2	= -UMEMCPY
##  Define MKDIR when a mkdir() system call is present (4.2BSD, SysVr3).
3	= -DMKDIR
#3	= -UMKDIR
##  Define NOVOID if your compiler doesn't like void casts.
4	= -DNOVOID
#4	= -UNOVOID
##  Define SYSTIME to use <sys/time.h> rather than <time.h> (4.2BSD).
5	= -DSYSTEM
#5	= -USYSTEM
##  Define VOIDFIX to allow pointers to functions returning void (non-PCC).
#6	= -DVOIDFIX
6	= -UVOIDFIX
##  Define CTC3B2 to support AT&T 3B2 streaming cartridge tape.
#7	= -DCTC3B2
7	= -UCTC3B2
CFLAGS	= $1 $2 $3 $4 $5 $6 $7 -O


all: afio afio.0

afio: afio.o
	$(CC) -i $(CFLAGS) -o afio afio.o

install: all
	install -s -c -m 0751 afio ${DESTDIR}/usr/local/afio
	install -c -m 444 -o bin -g bin afio.0 ${DESTDIR}/usr/local/man/cat1

afio.0:	afio.1
	rm -f afio.0
	/usr/man/manroff afio.1 > afio.0

clean:
	rm -f *.o afio afio.0

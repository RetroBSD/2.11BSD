#
#REMOTE=-DREMOTE			# -DREMOTE  use remote tape
SWAP=				# -DSWAP swap bytes
CFLAGS= $(SWAP) $(REMOTE) -O
LFLAGS=-i
OWNER=bin			# user for remote tape access
MODE=755
BINDIR=/usr/new
MANDIR=/usr/new/man/cat1
MAN=	vmsbackup.0
MANSRC=	vmsbackup.1

#
vmsbackup: vmsbackup.o match.o
	cc $(LFLAGS) -o vmsbackup vmsbackup.o match.o $(LIBS)

install: vmsbackup $(MAN)
	install -m $(MODE) -o $(OWNER) -s vmsbackup ${DESTDIR}$(BINDIR)	
	install -c -m 0444 -o bin -g bin vmsbackup.0 ${DESTDIR}$(MANDIR)

vmsbackup.0: $(MANSRC)
	/usr/man/manroff $(MANSRC) > $(MAN)

clean:
	rm -f vmsbackup *.o core $(MAN)

shar:
	shar -a README vmsbackup.1 Makefile vmsbackup.c match.c \
	    > vmsbackup.shar

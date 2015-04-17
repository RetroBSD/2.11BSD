BIN=	../../bin
LIB=	../../lib
LIBS=	$(LIB)/dbulib $(LIB)/access $(LIB)/iutil $(LIB)/gutil
CFLAGS= -O
LDFLAGS=-i
OBJS=	error.o
ALL=	creatdb destroydb purge helpr ingres printr restore sysmod \
	usersetup
ALLQ=	copydb

all: $(ALL)

allq: $(ALLQ)

creatdb: creatdb.o $(OBJS)
	$(CC) $(LDFLAGS) -o creatdb creatdb.o $(OBJS) $(LIBS)

destroydb: destroydb.o
	$(CC) $(LDFLAGS) -o destroydb destroydb.o $(LIBS)

purge: purge.o subs.o $(OBJS)
	$(CC) $(LDFLAGS) -o purge purge.o subs.o $(OBJS) $(LIBS)
		
helpr: helpr.o $(OBJS)
	$(CC) $(LDFLAGS) -o helpr helpr.o $(OBJS) $(LIBS)

ingres: ingres.o
	$(CC) $(LDFLAGS) -o ingres ingres.o $(LIBS)

printr: printr.o
	$(CC) $(LDFLAGS) -o printr printr.o $(LIBS)

restore: restore.o $(OBJS) subs.o
	$(CC) $(LDFLAGS) -o restore restore.o $(OBJS) subs.o $(LIBS)

sysmod: sysmod.o $(OBJS)
	$(CC) $(LDFLAGS) -o sysmod sysmod.o $(OBJS) $(LIBS)

usersetup: usersetup.o
	$(CC) $(LDFLAGS) -o usersetup usersetup.o $(LIBS)

copydb: copydb.q
	equel copydb.q
	$(CC) $(LDFLAGS) -o copydb copydb.c $(LIB)/gutil $(LIB)/libq.a

install: creatdb destroydb helpr ingres printr restore sysmod purge usersetup
	install -c -m 0700 usersetup $(BIN)/usersetup
	install -c -m 4751 purge $(BIN)/purge
	install -c -m 4751 creatdb $(BIN)/creatdb
	install -c -m 4751 destroydb $(BIN)/destroydb
	install -c -m 4751 helpr $(BIN)/helpr
	install -c -m 4751 ingres $(BIN)/ingres
	install -c -m 4751 printr $(BIN)/printr
	install -c -m 4751 restore $(BIN)/restore
	install -c -m 4751 sysmod $(BIN)/sysmod

installq: copydb
	install -c -m 0751 copydb $(BIN)/copydb

clean:
	rm -f $(ALL) $(ALLQ) copydb.c *.o *.core a.out

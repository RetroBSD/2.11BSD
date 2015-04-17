# Makefile for ddd

DEFS = -DBSD
CFLAGS = -O $(DEFS)

CC = cc
LINT = lint
CP = install -s -m 755 
RM = rm -f

SRC = ddd.c
OBJ = ddd.o
HEAD =
BIN = ddd
SHAR = ddd.shar

BINDIR = /usr/local

MANSRC = ddd.1
MAN = ddd.0
MANDIR = /usr/local/man/cat1

all: $(BIN) $(MAN)

$(BIN): $(OBJ) $(HEAD) Makefile
	$(CC) -i $(OBJ) -o $(BIN)

lint: $(SRC) $(HEAD)
	$(LINT) $(DEFS) $(SRC)
	touch lint

install: all
	$(CP) $(BIN) ${DESTDIR}$(BINDIR)
	install -m 444 -o bin -g bin $(MAN) ${DESTDIR}$(MANDIR)

$(MAN): $(MANSRC)
	/usr/man/manroff $(MANSRC) > $(MAN)

clean:
	-$(RM) $(BIN) $(OBJ) all lint a.out core *~ #* $(SHAR) $(MAN)

shar: lint README $(MAN) Makefile $(HEAD $(SRC)
	shar README $(MAN) Makefile $(HEAD) $(SRC) > $(SHAR)

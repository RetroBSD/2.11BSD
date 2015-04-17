DESTDIR= 
CFLAGS=	-O -DNUM_SYMS_CACHE=50
LDFLAGS=
SEPFLAG= -i
CFILES=	access.c command.c expr.c findfn.c format.c input.c main.c message.c \
	opset.c output.c pcs.c print.c runpcs.c setup.c sym.c

OFILES=	access.o command.o expr.o findfn.o format.o input.o main.o message.o \
	opset.o output.o pcs.o print.o runpcs.o setup.o sym.o

adb: ${OFILES}
	${CC} ${SEPFLAG} ${LDFLAGS} -o adb ${OFILES}

install: adb
	install -m 755 -s adb ${DESTDIR}/bin

clean:
	-rm -f adb ${OFILES}

print:
	@ls -l | pr
	@pr -f Makefile *.h ${CFILES}

lint:
	lint -haxc -I. ${CFILES}

access.o:	defs.h access.c
command.o:	defs.h command.c
expr.o:		defs.h expr.c
findfn.o:	defs.h findfn.c
format.o:	defs.h format.c
input.o:	defs.h input.c
main.o:		defs.h main.c
message.o:	defs.h message.c
opset.o:	defs.h opset.c
output.o:	defs.h output.c
pcs.o:		defs.h pcs.c
print.o:	defs.h print.c
runpcs.o:	defs.h runpcs.c
setup.o:	defs.h setup.c
sym.o:		defs.h sym.c

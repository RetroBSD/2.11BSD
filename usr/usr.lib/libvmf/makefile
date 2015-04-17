OBJECTS = vmf.o

all:  genlib genlib_p vmf.0

CFLAGS = -O

all:   genlib genlib_p

vmf.0:	vmf.3
	nroff -man vmf.3 > vmf.0

install:
	install -c -m 644 genlib /usr/lib/libvmf.a
	ranlib /usr/lib/libvmf.a
	install -c -m 644 genlib_p /usr/lib/libvmf_p.a
	ranlib /usr/lib/libvmf_p.a
	install -c -m 444 vmf.0 /usr/man/cat3/vmf.0

clean:
	rm -f ${OBJECTS} profiled/${OBJECTS} genlib genlib_p vmf.0

.c.o:
	${CC} -p ${CFLAGS} -c $*.c
	-ld -X -r $*.o
	mv a.out profiled/$*.o
	${CC} ${CFLAGS} -c $*.c
	-ld -X -r $*.o
	mv a.out $*.o

genlib genlib_p: ${OBJECTS}
	@echo "building profiled genlib"
	@cd profiled; ar cru ../genlib_p ${OBJECTS}
	@echo "buiding normal genlib"
	@ar cru genlib ${OBJECTS}

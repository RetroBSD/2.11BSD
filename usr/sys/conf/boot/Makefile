# 2.11BSD 1.0 April 8, 1994

H=	/sys/h

DEFS=	-DKERNEL -I. -I${H}

CPP=	/lib/cpp -P ${DEFS}
AS=	/bin/as -V

all: NONEboot.o brboot.o dvhpboot.o hk6boot.o hk7boot.o raboot.o rlboot.o rmboot.o sc11boot.o sc21boot.o siboot.o

.s.o:
	${CPP} $< | ${AS} -o $@


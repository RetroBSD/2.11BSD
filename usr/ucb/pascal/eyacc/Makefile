#
# eyacc makefile
#

CFLAGS=
LDFLAGS=
SEPFLAG= -i

HDRS= ey.h
SRCS= ey0.c ey1.c ey2.c ey3.c ey4.c ey5.c
OBJS= ey0.o ey1.o ey2.o ey3.o ey4.o ey5.o

eyacc: ${OBJS}
	-if [ X${SEPFLAG} = X-i ]; then \
		${CC} ${SEPFLAG} ${LDFLAGS} -o eyacc ${OBJS}; \
	else \
		ld ${SEPFLAG} ${LDFLAGS} -o eyacc /lib/crt0.o ey0.o \
		-Z ey1.o ey3.o -Z ey2.o ey4.o \
		-Y ey5.o -lc; \
	fi

install:
	@echo "Eyacc is only used by pascal, we don't install it for general"
	@echo "use."

tags: FRC
	ctags ${HDRS} ${SRCS}

FRC:

clean:
	rm -f *.o eyacc tags

print:
	@ls -l | pr
	@pr READ_ME Makefile
	@pr ${HDRS} ${SRCS}

ey0.o:	ey0.c
ey1.o:	ey1.c ey.h
ey2.o:	ey2.c ey.h
ey3.o:	ey3.c ey.h
ey4.o:	ey4.c ey.h
ey5.o:	ey5.c

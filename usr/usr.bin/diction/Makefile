#
#	@(#)Makefile	4.5.1	(2.11BSD)	1996/10/23
#
#	make file for STYLE and DICTION
#
#	DICT is the full pathname of the file dict.d, the
#		dictionary file used by DICTION
#
DESTDIR=
SEPFLAG= -i
SRCS = \
	abbrev.c conp.h dict.c dict.d \
	diction.c diction.sh edict.c ehash.c \
	explain.d \
	end.l getopt.c names.h nhash.c \
	nwords.l outp.c part.l pscan.c \
	style.h style.sh explain.sh ydict.c \
	README

DICT= -DDICT=\"/usr/share/misc/dict.d\"

all: diction style

style: style1 style2 style3

install: style diction style.sh diction.sh explain.sh dict.d explain.d
	-mkdir -p ${DESTDIR}/usr/libexec/diction
	install -s -m 755 style1 ${DESTDIR}/usr/libexec/diction
	install -s -m 755 style2 ${DESTDIR}/usr/libexec/diction
	install -s -m 755 style3 ${DESTDIR}/usr/libexec/diction
	install -s -m 755 dprog ${DESTDIR}/usr/libexec/diction
	install -c -m 755 style.sh ${DESTDIR}/usr/bin/style
	install -c -m 755 diction.sh ${DESTDIR}/usr/bin/diction
	install -c -m 755 explain.sh ${DESTDIR}/usr/bin/explain
	install -c -m 444 dict.d ${DESTDIR}/usr/share/misc
	install -c -m 444 explain.d ${DESTDIR}/usr/share/misc

clean:
	rm -f prtlex.c wdslex.c endlex.c
	rm -f *.o style1 style2 style3 dprog

style3: prtlex.o pscan.o outp.o
	cc ${SEPFLAG} prtlex.o pscan.o outp.o -ll  -o style3
prtlex.c: part.l
	lex part.l
	mv lex.yy.c prtlex.c
prtlex.o: names.h prtlex.c conp.h style.h names.h
	cc -c -O prtlex.c
pscan.o: names.h conp.h pscan.c
	cc -c -O pscan.c
outp.o: names.h conp.h style.h outp.c
	cc -c -O outp.c

style1: wdslex.o
	cc ${SEPFLAG} wdslex.o -ll  -o style1
wdslex.o: wdslex.c nhash.c dict.c ydict.c names.h abbrev.c
	cc -c wdslex.c
wdslex.c: nwords.l
	lex nwords.l
	mv lex.yy.c wdslex.c

style2: endlex.o
	cc ${SEPFLAG} endlex.o -ll  -o style2
endlex.o: names.h endlex.c ehash.c edict.c
	cc -c endlex.c
endlex.c: end.l
	lex end.l
	mv lex.yy.c endlex.c

diction: dprog
dprog: diction.c
	cc ${SEPFLAG} -O $(DICT) diction.c -o dprog

srcs:	sources

sources: $(SRCS)

$(SRCS):
	sccs get $@


#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.6.1 (2.11BSD GTE) 4/9/94
#
# Machine dependent routines for the PDP are located here
#
COMPAT=	compat-2.9
ALL=	crt gen net stdio sys ${COMPAT}
TAGSFILE=tags

pdplib pdplib_p: ${ALL}
	-mkdir tmp
	for i in ${ALL}; do (cd tmp; ar x ../$$i/$${i}lib); done
	ar cr pdplib `ls tmp/*.o | sort -t/ +1`
	rm -f tmp/*
	for i in ${ALL}; do (cd tmp; ar x ../$$i/$${i}lib_p); done
	ar cr pdplib_p `ls tmp/*.o | sort -t/ +1`
	rm -rf tmp

${ALL}:	FRC
	cd $@; make ${MFLAGS} DEFS=${DEFS}

FRC:

tags:
	for i in ${ALL}; do \
		(cd $$i; make ${MFLAGS} TAGSFILE=../${TAGSFILE} tags); \
	done

clean:
	for i in ${ALL}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f pdplib pdplib_p tmp/*.o;
	-rmdir tmp

depend:
	for i in ${ALL}; do \
		(cd $$i; make ${MFLAGS} DEFS=${DEFS} depend); done

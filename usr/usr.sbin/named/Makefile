#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	@(#)Makefile	4.14 (Berkeley) 2/28/88
#
SUBDIR=	tools named

all: named tools

tools: FRC
	(cd tools; make ${MFLAGS} );

named: FRC
	(cd named; make ${MFLAGS} named);

clean: FRC
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} clean); \
	done

install: all
	(cd named; make ${MFLAGS} install);
	(cd tools; make ${MFLAGS} install);

FRC:

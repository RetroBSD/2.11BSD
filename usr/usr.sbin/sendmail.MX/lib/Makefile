#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	@(#)Makefile	5.1.1 (2.11BSD) 1996/10/24
#
ALL=	sendmail.hf
SRCS=	sendmail.hf

all: FRC
clean: FRC
depend: FRC

install: sendmail.hf
	install -c -o bin -m 444 sendmail.hf ${DESTDIR}/usr/share/misc/sendmail.hf

FRC:

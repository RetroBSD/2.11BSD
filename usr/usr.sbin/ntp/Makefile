# @(#) Makefile 1.0 (2.11BSD) 1996/12/1
#
# $Log:	Makefile,v $
# Revision 3.4.1.7  89/05/18  12:43:32  louie
# Add preliminary support of NeXT machine, new floating point bug work-around
# and reference clock support.
# 
# Revision 3.4.1.6  89/05/03  15:08:06  louie
# In the Makefile, remove references to the readclock.c module.
# 
# Revision 3.4.1.5  89/04/10  15:54:19  louie
# Add dependency for ntpd.o on ntp.h so things sorta work even without doing a
# "make depend".  Trash "ci" rule.
# 
# Revision 3.4.1.4  89/04/07  18:04:14  louie
# Add definition of NOSWAP for Ultrix systems.
# 
# Revision 3.4.1.3  89/03/29  12:21:27  louie
# Don't bother to create the Version file for the distribution any longer.
# Define SUN_FLT_BUG rather than just 'sun' to get the a fix for a floating
# point problem in ntpsubs; apparantly the DECSTATION 3100 also has the
# same problem.
# 
# Revision 3.4.1.2  89/03/22  18:25:14  louie
# patch3: Use new and improved RCS headers.
# 
# Revision 3.4.1.1  89/03/20  00:01:24  louie
# patch1: Add LIBS macro to Makefile directives which link ntp, ntpdc, ntpd and
# patch1: test programs.
# patch1:  
# patch1: Add symbolic version identifier to Makefile install targets for
# patch1: proper directory name.
# 
# Revision 3.4  89/03/17  18:36:37  louie
# Latest test release.
# 
# Revision 3.3.1.1  89/03/17  18:21:17  louie
# Install new binaries only if they differ.
# 
# Revision 3.3  89/03/15  14:19:08  louie
# New baseline for next release.
# 
# Revision 3.2.1.2  89/03/15  13:42:36  louie
# Updated distribution directory.
# 
# Revision 3.2.1.1  89/03/10  11:23:21  louie
# Repair a bunch of typos in the Makefile
# 
# Revision 3.2  89/03/07  18:19:37  louie
# New version of UNIX NTP daemon and software based on the 6 March 1989 
# draft of the new NTP specification.  
# 
# Revision 3.1.1.1  89/02/15  09:09:47  louie
# Applied fixes from bug reports.  Makefile now runs the ntest program to
# verify that certain arithmetic operations are done correctly on the
# host that the code is being compiled on.
# 
DESTDIR=
LDFLAGS= -i
BINDIR=/usr/sbin
LIBS=

VERS=3.4

INSTALL= install -c -s -m 755

# FEATURES include:
#	DEBUG	  - include DEBUG code
#	BROADCAST_NTP - experimental support for broadcast NTP
#	XADJTIME2 - experimental support for second-order clock adjustment
#		    system call.
#	SETTICKADJ - attempt to modify kernel's `tickadj' variable at run time.
#	REFCLOCK  - define if you have a reference clock attached to your
#		    machine.  (untested by UMD)
#	PSTI - define along with REFCLOCK if you have a PSTI clock attached
#		that you'd like to use a a reference clock.
#	XTAL=0 - for line freq clock, or
#	XTAL=1 	 for crystal controlled clock (default)
#	LOG_NTP=foo - to change the syslog facility.  You could specify
#		    something like -DLOG_NTP=LOG_LOCAL3 to log into the
#		    LOG_LOCAL3 syslog facility
#	NOSWAP - allow use of plock() to prevent swapping
#

# FEATURES=
FEATURES= -DDEBUG -DLOG_NTP=LOG_LOCAL3
#FEATURES= -DBROADCAST_NTP -DSETTICKADJ -DDEBUG -DLOG_NTP=LOG_LOCAL3

# for 2.11BSD
DEFINES= -DGENERIC_UNS_BUG -DSUN_FLT_BUG

# for 4.3 BSD
#DEFINES=

# for Sun
#DEFINES= -DSUN_FLT_BUG

# for Ultrix 2.0/2.2
# don't forget to fix the broken definition of inet_addr in netdb.h
# it should be declared as a u_long not a in_addr  (the doc is wrong also)
# VAX_COMPILER_FLT_BUG is defined for pcc which doesn't know how to 
# convert an unsigned long into a float/double
#DEFINES= -DVAX_COMPILER_FLT_BUG -DNOSWAP

#
# for a NeXT system, define these pre-processor symbols.
#DEFINES=-DSUN_FLT_BUG -DGENERIC_UNS_BUG 

CFLAGS= -O ${DEFINES} ${FEATURES}
#
# Header files
#
HDRS=	ntp.h patchlevel.h

# Source files
#
NTPDSRC= ntpd.c ntpsubs.c ntp_proto.c ntp_sock.c ntp_adjust.c read_local.c \
	read_psti.c
SRCS=	ntp.c ntpdc.c ${NTPDSRC}

# Object files
#
NTPDOBJ= ntpd.o ntpsubs.o ntp_proto.o ntp_sock.o ntp_adjust.o read_local.o \
	read_psti.o
OBJS=	ntp.o ntpdc.o ${NTPDOBJ}

PROGS=	ntp ntpd ntpdc ntest

all:	${PROGS} man
	@./ntest

man:	FRC
	cd man; make ${MFLAGS} DESTDIR=${DESTDIR} all

ntp:	ntp.o ntpsubs.o
	${CC} ${LDFLAGS} -o ntp ntp.o ntpsubs.o ${LIBS}

ntpd:	${NTPDOBJ}
	${CC} ${LDFLAGS} -o ntpd ${NTPDOBJ} ${LIBS}

FRC:

ntpdc: ntpdc.o
	${CC} ${LDFLAGS} -o ntpdc ntpdc.o ${LIBS}

ntest: test.o ntpsubs.o
	${CC} ${LDFLAGS} -o ntest test.o ntpsubs.o ${LIBS}

sock_test: ntp_sock.c
	${CC} ${LDFLAGS} -DTEST -o sock_test ntp_sock.c ${LIBS}

${OBJS}:	ntp.h
ntpd.o:	patchlevel.h

install: ${PROGS}
	@./ntest
	${INSTALL}  ntpd ${DESTDIR}${BINDIR}/ntpd
	${INSTALL}  ntp ${DESTDIR}${BINDIR}/ntp
	${INSTALL}  ntpdc ${DESTDIR}${BINDIR}/ntpdc
	cd man; make ${MFLAGS} DESTDIR=${DESTDIR} install

clean:
	rm -f *.o *~ ${PROGS} ntest sock_test
	cd man; make ${MFLAGS} DESTDIR=${DESTDIR} clean

lint:
	lint -bac ${DEFINES} ${NTPDSRC}

depend:
	mkdep $(CFLAGS) $(SRCS)

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

ntp.o: ntp.c /usr/include/stdio.h /usr/include/sys/types.h
ntp.o: /usr/include/sys/param.h /usr/include/sys/localopts.h
ntp.o: /usr/include/machine/machparam.h /usr/include/sys/types.h
ntp.o: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/time.h
ntp.o: /usr/include/time.h /usr/include/sys/uio.h /usr/include/sys/socket.h
ntp.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
ntp.o: /usr/include/sys/ttydev.h /usr/include/netinet/in.h
ntp.o: /usr/include/netinet/in_systm.h /usr/include/netinet/ip.h
ntp.o: /usr/include/netinet/udp.h /usr/include/arpa/inet.h /usr/include/netdb.h
ntp.o: /usr/include/strings.h /usr/include/errno.h ntp.h
ntpdc.o: ntpdc.c /usr/include/sys/types.h /usr/include/sys/param.h
ntpdc.o: /usr/include/sys/localopts.h /usr/include/machine/machparam.h
ntpdc.o: /usr/include/sys/types.h /usr/include/signal.h
ntpdc.o: /usr/include/sys/types.h /usr/include/signal.h /usr/include/sys/uio.h
ntpdc.o: /usr/include/sys/socket.h /usr/include/sys/time.h /usr/include/time.h
ntpdc.o: /usr/include/netinet/in.h /usr/include/netinet/udp.h
ntpdc.o: /usr/include/errno.h /usr/include/stdio.h /usr/include/netdb.h
ntpdc.o: /usr/include/strings.h /usr/include/arpa/inet.h ntp.h
ntpd.o: ntpd.c /usr/include/stdio.h /usr/include/sys/types.h
ntpd.o: /usr/include/sys/param.h /usr/include/sys/localopts.h
ntpd.o: /usr/include/machine/machparam.h /usr/include/sys/types.h
ntpd.o: /usr/include/signal.h /usr/include/sys/types.h /usr/include/sys/uio.h
ntpd.o: /usr/include/sys/socket.h /usr/include/sys/time.h /usr/include/time.h
ntpd.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
ntpd.o: /usr/include/sys/ttydev.h /usr/include/sys/resource.h
ntpd.o: /usr/include/sys/file.h /usr/include/net/if.h /usr/include/net/if_arp.h
ntpd.o: /usr/include/netinet/in.h /usr/include/netinet/in_systm.h
ntpd.o: /usr/include/netinet/ip.h /usr/include/netinet/udp.h
ntpd.o: /usr/include/arpa/inet.h /usr/include/netdb.h /usr/include/strings.h
ntpd.o: /usr/include/errno.h /usr/include/syslog.h /usr/include/nlist.h
ntpd.o: /usr/include/sys/types.h ntp.h patchlevel.h
ntpsubs.o: ntpsubs.c /usr/include/stdio.h /usr/include/sys/types.h
ntpsubs.o: /usr/include/sys/param.h /usr/include/sys/localopts.h
ntpsubs.o: /usr/include/machine/machparam.h /usr/include/sys/types.h
ntpsubs.o: /usr/include/signal.h /usr/include/sys/types.h
ntpsubs.o: /usr/include/sys/time.h /usr/include/time.h /usr/include/sys/uio.h
ntpsubs.o: /usr/include/sys/socket.h /usr/include/netinet/in.h
ntpsubs.o: /usr/include/arpa/inet.h /usr/include/errno.h /usr/include/syslog.h
ntpsubs.o: ntp.h
ntp_proto.o: ntp_proto.c /usr/include/stdio.h /usr/include/sys/types.h
ntp_proto.o: /usr/include/sys/param.h /usr/include/sys/localopts.h
ntp_proto.o: /usr/include/machine/machparam.h /usr/include/sys/types.h
ntp_proto.o: /usr/include/signal.h /usr/include/sys/types.h
ntp_proto.o: /usr/include/sys/uio.h /usr/include/sys/socket.h
ntp_proto.o: /usr/include/sys/time.h /usr/include/time.h
ntp_proto.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
ntp_proto.o: /usr/include/sys/ttydev.h /usr/include/sys/resource.h
ntp_proto.o: /usr/include/net/if.h /usr/include/net/if_arp.h
ntp_proto.o: /usr/include/netinet/in.h /usr/include/netinet/in_systm.h
ntp_proto.o: /usr/include/netinet/ip.h /usr/include/netinet/udp.h
ntp_proto.o: /usr/include/arpa/inet.h /usr/include/netdb.h
ntp_proto.o: /usr/include/strings.h /usr/include/errno.h /usr/include/syslog.h
ntp_proto.o: ntp.h
ntp_sock.o: ntp_sock.c /usr/include/sys/types.h /usr/include/sys/param.h
ntp_sock.o: /usr/include/sys/localopts.h /usr/include/machine/machparam.h
ntp_sock.o: /usr/include/sys/types.h /usr/include/signal.h
ntp_sock.o: /usr/include/sys/types.h /usr/include/sys/ioctl.h
ntp_sock.o: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
ntp_sock.o: /usr/include/sys/file.h /usr/include/sys/socket.h
ntp_sock.o: /usr/include/netinet/in.h /usr/include/net/if.h
ntp_sock.o: /usr/include/net/if_arp.h /usr/include/errno.h
ntp_sock.o: /usr/include/syslog.h /usr/include/stdio.h ntp.h
ntp_adjust.o: ntp_adjust.c /usr/include/stdio.h /usr/include/sys/types.h
ntp_adjust.o: /usr/include/sys/param.h /usr/include/sys/localopts.h
ntp_adjust.o: /usr/include/machine/machparam.h /usr/include/sys/types.h
ntp_adjust.o: /usr/include/signal.h /usr/include/sys/types.h
ntp_adjust.o: /usr/include/sys/socket.h /usr/include/sys/time.h
ntp_adjust.o: /usr/include/time.h /usr/include/sys/ioctl.h
ntp_adjust.o: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
ntp_adjust.o: /usr/include/sys/resource.h /usr/include/netinet/in.h
ntp_adjust.o: /usr/include/netinet/in_systm.h /usr/include/netinet/ip.h
ntp_adjust.o: /usr/include/netinet/udp.h /usr/include/arpa/inet.h
ntp_adjust.o: /usr/include/netdb.h /usr/include/strings.h /usr/include/errno.h
ntp_adjust.o: /usr/include/syslog.h ntp.h
read_local.o: read_local.c
read_psti.o: read_psti.c

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY

.EH 'Page %''Changes in 2.11BSD'
.OH 'Changes in 2.11BSD''Page %'
.ds Ps 2.10.1BSD
.ds 2B 2.11BSD
.TL
Changes in the \*(2B kernel
.sp
.de D
.ie \\n(.$>1 \\$1 \\$2 \\$3
.el DRAFT of \n(mo/\n(dy/\n(yr
..
.D November 1, 1990
.AU
Steven M. Schultz
.AI
Contel Federal Systems
Westlake Village, California 91359-5027
wlbr!wlv!sms; sms@wlv.imsd.contel.com
.PP
This document summarizes changes in PDP-11\(ua UNIX\(dd between
.FS
\u\(ua\d\s-2DEC\s0, \s-2PDP-11\s0, \s-2QBUS\s0, and
\s-2UNIBUS\s0 are trademarks of Digital Equipment Corporation.
.br
\u\(dd\d\s-2UNIX\s0 is a trademark of Bell Laboratories.
.FE
this release and the January 1989 \*(Ps distribution.
.PP
It is intended to provide sufficient information that those who maintain
the kernel, have local modifications to install, or who have versions of
\*(Ps modified to run on other hardware should be able to determine how
to integrate this version of the system into their environment.  As always,
the source code is the final source of information, and this document is
intended primarily to point out those areas that have changed.
.LP
Most of the changes between \*(Ps and \*(2B fall into one of several
categories.  These are:
.RS
.IP \(bu 3
bug fixes,
.IP \(bu 3
performance improvements,
.IP \(bu 3
addition of 4.3BSD system calls and application programs,
.IP \(bu 3
removal of features no longer supported in the 4.3BSD release,
.IP \(bu 3
new protocol and hardware support.
.RE
.LP
The major changes to the kernel are:
.RS
.IP \(bu 3
the implementation of the 4.3BSD on disk directory structure,
.IP \(bu 3
a rewrite of the kernel I/O interface using the 4.3BSD UIO/IOVEC mechanism,
.IP \(bu 3
the addition of system calls,
.IP \(bu 3
replacement of even more of the high kernel with portions of the 4.3BSD kernel,
.IP \(bu 3
the addition of the 4.3BSD namei cache,
.IP \(bu 3
additional restructuring of the kernel into the 4.3BSD structure.
.RE
.PP
The major purpose of
this document is to summarize the changes between this and the January
1989 release of 2.10BSD.  The latter was designated as \*(Ps.
Although the changes are fairly simple to describe, they cover large
portions of the distribution.  Most will not be visible to either
users or administrators; in some cases no recompilation is necessary.
However, since the on disk directory structure and password file format
have changed, programs which deal with directories or the password
file will need to be recompiled.
Administrators should be aware that the 4.3BSD disk quota system
is available and works quite well.  
.PP
The major change, and the reason for the release, is an
.B extensive
reworking of the kernel to implement the \fIreadv\fP and \fIwritev\fP
system calls and the 4.3BSD on disk directory structure.
Filenames are no longer limited to 14 characters in length.  At present,
MAXNAMELEN is set at 63 (one fourth of the maximum path name length
specified by MAXPATHLEN).
.PP
In application land, many of the 4BSD changes/enchancements released
over the Internet
have been ported to \*(2B, most notably the shadow password file
(with password aging), FTP with the \fIrestart\fP capability, rshd and
rlogind with security fixes.
.B Many
other programs have had fixes applied, in particular, the line printer
spooling sub-system.
.PP
This document is not intended to be an introduction to the kernel, but
assumes familiarity with prior versions of the kernel, particularly the
\*(Ps release.  Other documents may be consulted for more complete
discussions of the kernel and its subsystems.  It cannot be too strongly
emphasized that \*(2B resembles 4.3BSD even more than \*(Ps did.
The tradition of remaining bug-for-bug compatible with
4.3BSD has been continued.  It is \fBSTRONGLY\fP recommended that 
4.3BSD manuals be consulted when using this system.  
2.9BSD manuals are not even close to being correct.  Online
documentation is as complete and correct as time permitted.
.PP
This release is not supported, and it is definitely
.B NOT
an official
Berkeley release.  It was called \*(2B because of the number of
changes (including the file system) and because a coldstart from tape is
required.
The ``bugs'' address
supplied with this release (as well as with the 4BSD releases) will
work for some unknown period of time; \fBmake sure\fP that the ``Index:''
line of the bug report indicates that the release is ``2.11BSD''.  See
the \fIsendbug\fP(8) program for more details.  All fixes that
I make, or that are sent to me, will be posted on \fIUSENET\fP, in
the news group ``comp.bugs.2bsd''.
.PP
The author would like to thank the following people for their
contributions to the development of this release.  His employer
for providing the environment and equipment used in the creation
of this release.  To Paul Taylor (taylor@oswego.oswego.edu) for
doing the 11/73 testing in spite of Q22 being undefined.  To
Terry Kennedy (terry@spcvxa.spc.edu) for being eclectic enough
to beta-test a \*(2B system alongside the VMS and RSTS
systems.
.PP
Finally, much credit should go to the authors of 4.2BSD and 4.3BSD from
which even more was stolen than had previously been the case for \*(Ps.

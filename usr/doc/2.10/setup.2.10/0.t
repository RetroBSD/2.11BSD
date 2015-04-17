.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)0.t	6.2 (Berkeley) 10/1/88
.\"
.EH 'setup.2.10 - %''Installing and Operating 2.10.1BSD on the PDP'
.OH 'Installing and Operating 2.10.1BSD on the PDP''Setup.2.10 - %'
.ds Ps 2.10BSD
.ds 2B 2.10.1BSD
.bd S B 3
.TL
Installing and Operating \*(2B on the PDP
.br
October 1, 1988
.AU
Keith Bostic
.AI
Computer Systems Research Group
Department of Electrical Engineering and Computer Science
University of California, Berkeley
Berkeley, California 94720
seismo!keith; bostic@ucbvax.berkeley.edu
.AU
Casey Leedom
.AI
Department of Computer Science
California State University, Stanislaus
Turlock, California  95380
ucbvax!vangogh!casey; casey@vangogh.berkeley.edu
.de IR
\\fI\\$1\|\\fP\\$2
..
.de UX
UNIX\\$1
..
.AB
.PP
.FS
* DEC, PDP, VAX, IDC, SBI, UNIBUS and MASSBUS are trademarks of
Digital Equipment Corporation.
.FE
.FS
** UNIX is a Trademark of Bell Laboratories.
.FE
This document contains instructions for the
installation and operation of the \*(2B PDP*
.UX **
system.  It is adapted from the papers \fIInstalling and Operating 4.3BSD
on the VAX\fP by Michael J. Karels, James M. Bloom, Marshall Kirk McKusick,
Samuel J. Leffler, and William N. Joy, and \fIInstalling and Operating
2.9BSD\fP by Michael J. Karels and Carl F. Smith.
.PP
It discusses procedures for installing \*(2B UNIX on a PDP-11, including
explanations of how to lay out file systems on available disks, how to set
up terminal lines and user accounts, how to do system-specific tailoring,
and how to install and configure the networking facilities.  Finally, the
document details system operation procedures: shutdown and startup,
hardware error reporting and diagnosis, file system backup procedures,
resource control, performance monitoring, and procedures for recompiling
and reinstalling system software.
.PP
This release is not supported, nor should it be considered an official
Berkeley release.  It was called \*(2B because 2.9BSD has clearly become
overworked, System V was already taken, and referring to it simply as
\*(Ps would engender too much confusion as to whether the first or
second release was being talked about.
.PP
The ``bugs'' address supplied with this release (as well as with the 4BSD
releases) will work for some unknown period of time; \fBmake sure\fP that
the ``Index:'' line of the bug report indicates that the release is
``\*(Ps''.  See the \fIsendbug\fP(8) program for more details.  All
fixes that we make, or that are sent to us, will be posted on
\fIUSENET\fP, in the news group ``comp.bugs.2bsd''.
.AE

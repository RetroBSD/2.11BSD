.\" troff -ms
.if n .nr LL 7i
.if t .nr LL 6.5i
.DS C
.LG
Second Distribution of Berkeley PDP-11\(dg Software for UNIX\(dd
.NL
.FS \(dg
DEC, PDP, and VAX are tradmarks of Digital Equipment Corporation
.FE
.FS \(dd
UNIX is a tradmark of Bell Loboratories
.FE
.LG
Release 2.10
(Revised April 1987)
.NL
.DE
.PP
The USENIX association and the Computer Systems Research Group (CSRG) of
the University of California, Berkeley, are pleased to announce the
distribution of a new release of the "Second Berkeley Software Distribution"
(2.10BSD).
.PP
This release will be handled by the USENIX association, and is available
to all V7, System III, System V, and 2.9BSD licensees.  The Association
will continue to maintain the non-profit price of $200, as was charged by
the CSRG.  The release will consist of two 2400 ft., 1600 bpi tapes
(approximately 80M) and approximately 100 pages of documentation.  If you
require 800 bpi tapes, please contact USENIX for more information.
.PP
If you have questions about the distribution of the release, please
contact USENIX.  USENIX's address and phone number is as follows:
.RS
.LP
USENIX Association
.br
P.O. Box 2299
.br
Berkeley, CA  94710
.br
+1 415 528-8649
.RE
.LP
USENIX may also be contacted by electronic mail at:
.RS
{ucbvax,decvax}!usenix!office
.RE
.LP
If you have technical questions about the release, please contact
Keith Bostic at:
.RS
seismo!keith, ucbvax!bostic
.br
bostic@okeeffe.berkeley.edu
.br
+1 415 642-4948
.RE
.LP
.B "Q: What machines will 2.10BSD run on?"
.RS
.LP
2.10BSD will run on:
.RS
11/24/34/44/53/60/70/73/83/84
.br
11/23/35/40/45/50/55 with 18 or 22 bit addressing
.RE
.LP
2.10 WILL NOT run on:
.RS
T11, 11/03/04/05/10/15/20/21
.br
11/23/35/40/45/50/55 with 16 bit addressing
.RE
.RE
.B "Q: What's new in this release?"
.RS
.PP
Lots of stuff.  This release is 4.3BSD.  We don't expect to distribute
manuals this time, we expect people to simply use the 4.3BSD ones.
A list of some of the larger things that have been added:
.RS
.sp 1
22-bit Qbus support
.br
4.3BSD networking, (TCP/IP, SLIP)
.br
4.3BSD serial line drivers
.br
4.3BSD C library
.br
most of the 4.3BSD application programs
.br
complete set of 4.3BSD system calls
.br
MSCP device driver for (RQDX? UDA50, KLESI)
.br
RAM disk
.br
inode, core, and swap caching
.br
conversion of the entire system to a 4.3BSD structure
.RE
.RE
.LP
.B "Q: Why get this release?"
.RS
.PP
You want to get this release for one of two reasons.  Either you have a
number of 4.3BSD programs or machines in your environment and you'd like
consistency across the environment, or you want a faster, cleaner version
of 2.9BSD, with or without networking.
.PP
This release is, without question, considerably faster than any other
PDP-11 system out there.  There have been several major changes to the
2.10BSD kernel to speed it up.
.RS
.IP \(bu
The kernel namei routine has been modified to read the entire path name in
at once rather than as a single character at a time, as well as
maintaining a cache of its position in the current directory.
.IP \(bu
The exec routine now copies its arguments a string at a time, rather than
a character at a time.
.IP \(bu
All inodes are placed in an LRU cache, eliminating going to disk for
often used inodes; kernel inodes also contain more of the disk inode
information to eliminate require disk access for stat(2) calls.
.IP \(bu
Both core and swap are LRU cached; the former is particularly interesting
on PDP-11's with large amounts (for PDP's, anyway) of memory.  Our
experience with an 11/44 with 4M of memory, in a student environment, is
that it never swaps, and only rarely do programs leave core.
.IP
This change is largely responsible for My Favorite Timing:
Ultrix 11, V3.0, on my 11/73, with a single RD52, takes 1.1 system seconds
to run vi.  2.9BSD takes approximately .9 system seconds, a difference
probably attributable to the fact that 2.9BSD has vfork.  Once 2.10BSD has
the vi image in its core cache, it executes vi in .2 system seconds.
.IP \(bu
Finally, many other speedups, such as rewriting several of C library
routines in assembler, replacing the kernel clist routines with the
faster 4.3BSD ones, caching and hashing process id's, and splitting the
process list into multiple lists have been added.
.RE
.RE
.LP
.B "Q: How good is the networking?"
.RS
.PP
The networking is 4.3BSD's.  It runs, it runs correctly.  It eats space
like there's some kind of reward.  We are considering fixing the latter
by moving the networking into supervisor space.
.RE
.LP
.B "Q: Will this release be supported?"
.RS
.PP
This release is not supported, nor should it be considered an official
Berkeley release.  It was called 2.10BSD because 2.9BSD has clearly
become overworked and System V was already taken.
.PP
The ``bugs'' address supplied with this release (as well as with the 4BSD
releases) will work for some unknown period of time; make sure that the
``Index:'' line of the bug report indicates that the release is
``2BSD''.  See the sendbug(8) program for more details.  All fixes that
we make, or that are sent to us, will be posted on USENET, in the news
group ``comp.bugs.2bsd''.  USENIX is aware of this problem and is willing
to make hard-copy bug reports available to those of you not connected to
the net.
.PP
To summarize, all that I can say is that any
.I major
problems will be fixed, i.e. if you've got a program that's crashing the
kernel, we'll be inclined to fix it.  If ``ls'' is misformatting its
output, you're probably on your own.
.RE
.LP
.B "Q: Is this the last release?"
.RS
.PP
Yes, at least by us; quite frankly, we'd rather sacrifice our chance at
heaven than look at a 16-bit machine again.
.RE
.LP
.B "Q: Who did all this wonderful, exciting, neat stuff?"
.RS
.PP
Mostly Casey Leedom, of California State University, Stanislaus, and
Keith Bostic, of the CSRG.  From the ``Changes to the Kernel in 2.10BSD''
paper:
.RS
.PP
The authors gratefully acknowledge the contributions of many other
people to the work described here.  Major contributors include
Gregory Travis of the Institute for Social Research, and Steven Uitti
of Purdue University.  Jeff Johnson, also of the Institute for Social
Research, was largely responsible for the port of 4.3BSD's networking
to 2.10BSD.  Cyrus Rahman of Duke University should hold some kind of
record for being able to get the entire kernel rewritten with a single
10-line bug report.  Much credit should also go to the authors of 4.2BSD
and 4.3BSD from which we stole everything that wasn't nailed down and
several things that were.  (Just ``diff'' this document against \fIChanges
to the Kernel in 4.2BSD\fP if you don't believe that!)  We are also
grateful for the invaluable guidance provided by Michael Karels, of
the Computer Science Research Group, at Berkeley \- although we felt
that his suggestion that we ``just buy a VAX'', while perhaps more
practical, was not entirely within the spirit of the project.
.RE
.RE
.sp 1
Keith Bostic
.br
Casey Leedom
.bp
.PP
The tape that USENIX will be distributing for the first few weeks will
.UL only
support machines with split I/D and floating point hardware.  This
is not because any work remains to be done, but because we just haven't
had the time to build and test a system.
.PP
Sites wishing to run 2.10BSD should also be aware that the networking is
only lightly tested, and that certain hardware has yet to be ported.
Contact Keith Bostic at the above address for current information as to the
status of the networking.  As of August 6, 1987, the complete 4.3BSD
networking is in place and running, albeit with minor problems.  The holdup
is that only the Interlan Ethernet driver has been ported, as well as some
major space constraints.  Note, if we decide to go to a supervisor space
networking, 2.10 networking will only run on:
.RS
11/44/53/70/73/83/84
.br
11/45/50/55 with 18 bit addressing
.RE

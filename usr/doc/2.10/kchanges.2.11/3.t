.NH
Changes in application programs and libraries.
.PP
The application directories have been rearranged slightly in an effort
to follow the 4.3Tahoe based updates released over the Internet.  Many
programs which used to be single .c source files are now subdirectories
with their own make files (\fIlogin\fP(1) for example).
As in the kernel, the goal was to remain as true to the 4.3BSD sources
as possible.
In some of the source directories there are either or both of
the special directories ``PORT'' and ``OLD''.  PORT contains
copies of 4.3BSD source which ought to be ported to \*(2B, but due to
time constraints had to be left undone.  The number of PORT directories
included with \*(2B is smaller than with \*(Ps for two reasons: 1) to keep
the distribution from being more than two reels of tape and 2) the amount 
of new/ported software has increased greatly.  Copies of the missing PORT 
directories are
available from either a cooperating 4.3BSD system or the network
archives around the world.
OLD contains copies of 2.9BSD source that, while we have ported
the 4.3BSD version of the source code, we're unsure enough of it that we
wanted to provide a backup copy.  The only OLD directory of significance
included in \*(2B is /usr/src/new/OLD which contains miscellaneous bits
of trivia such as \fIlisp11\fP and so on.

The portable/ASCII \fIar\fP(1) file format from 4.3BSD has been implemented,
this is described in the paragraphs below.

The following paragraphs are a description
of several things that have changed outside of the kernel.
.IP 1)
Since the C compiler still only recognizes seven significant
characters in external names, several standard library names had to be
changed to prevent name collisions.  However, to prevent portability
problems in your programs you should use the standard names.  All known
collisions in the standard include files or the C library have been handled
either in the include file itself or in the include file
\fIinclude/short_names.h\fP.  This works because we're using the 4.3BSD C
preprocessor, which has flexnames.  Networking programs almost always need
to include \fIshort_names.h\fP.  See \fIsrc/bin/mail.c\fP and
\fIsrc/bin/login/login.c\fP for examples of long name work arounds.  The C
library itself with only a couple exceptions is a port of the 4.3BSD C library.
.IP 2)
Files ported from 4BSD systems that have more than MAXNAMLEN
characters will no longer
port correctly.  Since MAXNAMLEN has been raised to 63 this should not
be a problem.
.IP 3)
The directory reading routines are a fresh port from 4.3BSD and are
part of stdio.
The old V7 directory structure does \fBNOT\fP exist any longer in \fIdir.h\fP.
There are only two programs in \*(2B which know what the old 
directory structure looked like: src/old/\fIdump\fP and \fIrestor\fP(8).
The define ``DIRSIZ'' has been ported (and fixed, there was a rounding
error) from 4.3BSD.  MAXNAMLEN is now 63.  It would be possible (and
certainly easier) to raise this now than it was to implement the new
filesystem initially, but once raised the limit can not be lowered.
.IP 4)
The 4.3BSD manual pages for \fIsigblock\fP(2), \fIsigpause\fP(2), and
\fIsigsetmask\fP(2) are deceptive.  They indicate that signal masks are
integers, but, they must be able to hold 32 bits.  Typically you'll see
code like:
.nf
.ft B

	int omask;
	omask = sigsetmask(0);
	...
	sigsetmask(omask);
.ft R

which should be translated to:

.ft B
	long omask;
	omask = sigsetmask(0L);
	...
	sigsetmask(omask);

.ft R
.fi
In general, the fact that 4.3BSD thinks an ``int'' is 32 bits is the
worst porting problem that you'll run into; finding ``ints'' that
should be ``longs'' is an arcane art.  Routines that I look for as a matter
of habit are any one of the \fIseek\fP(2) routines, \fIftell\fP(3),
\fItime\fP(2), the various \fIsignal\fP(2) routines, \fIselect\fP(2) and the
\fItruncate\fP(2) routines.
.IP
The functions \fIsigblock\fP and \fIsigsetmask\fP are defined as returning
a long result in \fIh/signal.h\fP, which should ease some of the porting
problems.  The lint libraries have also been updated.  In general, though,
you'll have to scan any source you plan on porting for calls to
\fIsigblock\fP, \fIsigpause\fP, or \fIsigsetmask\fP that take an int
as a parameter or store their return value in an int.
.IP
To give an indication of the subtlety the long/int problem can take
on, consider the following code fragment taken from /sys/sys/tty.c:
.nf
.ft B

	newflags = (tp->t_flags&0xffff0000) | (sg->sg_flags&0xffff));

.ft R
.fi
where \fInewflags\fP and the fields \fIt_flags\fP and \fIsg_flags\fP are
all longs.  The problem occurs with "sg->sg_flags&0xffff".  The intent is
fairly obvious, but in \fBlong op int\fP the \fBint\fP ("0xffff") is
sign-extended to \fBlong\fP ("0xffffffff") before the operation as per
K&R.  The resulting operation in this case is a no-op!  The fix is fairly
simple in this instance and yields the following:
.nf
.ft B

	newflags = (tp->t_flags&0xffff0000) | (sg->sg_flags&0xffffL));

.ft R
.IP 5)
The PDP-11 \fIsetjmp\fP(2) implementation contains a subtle bug that
occurs when a routine containing a \fIsetjmp\fP has \fIregister\fP
variables.  The bug sometimes causes those variables to be given
invalid values when a longjmp is made back to the routine.  This is
probably impossible to fix in a reasonable manner, and it's much simpler
to simply avoid register variables in such routines.
.IP 6)
The optional '#' character is still not supported by \fIprintf\fP(3).
.IP 7)
The DEC MXV11 bootstrap ROM, for the RL's, TU's, RX's, TK's and RD's among
others, \fIrequires\fP that deadstart blocks begin with an 0240 and a
branch. This has already been implemented
in \fIrauboot.s\fP and \fIrluboot.s\fP in \fIsys/mdec\fP as well as
\fIsys/pdpstand/tmscpboot.s\fP.
.IP 8)
To port the 4.3BSD \fImake\fP(1) program, several of its table sizes had
to be reduced.  Make is very unforgiving of Makefiles that are too large.
If make drops core for no reason that you can think of, try reducing the
size of the Makefile.  Also, don't run make depend in the system
application directories, make can't handle it.
.IP 9)
Don't set \fIcsh\fP history too high; it eventually runs out of space
and logs you out.  \fIcsh\fP now is overlaid.  The additional code added
by the shadow password file routines pushed \fIcsh\fP over the edge - no
surprise there, \fIcsh\fP was within a couple hundred bytes to begin with.
Since \fIcsh\fP had to be overlaid, \fIlimits\fP were enabled.
.IP 10)
The games directory under \*(Ps was largely untested and nothing has
been done to change this.
.IP 11)
The C compiler actually handles bit fields (but generates atrocious code),
identically named global structure elements, and lots of other stuff.
The generated code is not terrible overall but not exactly great either,
and the optimizer does very little to correct the situation.
It \fIdoesn't\fP handle any of the old assignment operators, and, not only
doesn't handle them, but \fBproduces bogus code\fP.  It is \fBSTRONGLY\fP
recommended that you read the file \fIsrc/lib/ccom/TEST/README\fP.  It goes
into the problems with this compiler in more depth and contains some other
Extremely Important Information.
.IP 12)
\fIreadv\fP(2) and \fIwritev\fP(2) under \*(2B are implemented as actual
system calls rather than compatibility routines.  At present very little
in the system aside from \fIperror\fP(3) and \fIsyslog\fP(3) use
scatter read or write.
.IP 13)
There is a define, ``SEPFLAG'', in many Makefiles, that governs compilation
for separate and non-separate I/D machines.  If you have a non-separate I/D
machine, set it to ``-n''.  If you have a separate I/D machine, set it to
``-i''.   This should really go away since the chances of \*(2B ever
running on a non-split machine are not distinguishable from zero at this
time.  Too many capabilities have been added, programs such as \fIcsh\fP and
sendmail have to be overlaid even on split I/D machines!
.IP 14)
The directory \fI/usr/src/new\fP is a compendium of programs moved
in from the 4.3BSD directory \fIusr/src/new\fP, assorted
programs ported to \*(2B from various places around the Internet, and
remnants
from the 2.9BSD \fIusr/src/local\fP and \fIusr/src/contrib\fP directories.
Most of the programs in \fI/usr/src/new\fP have been in production use
for many months, while others are less well tested.
.IP 15)
New versions of \fIftp\fP and \fIftpd\fP are present.  The stream
restart capability is present allowing an aborted transfer to be
restarted if the remote server also provides the \fIrestart\fP command.
Also a number of bugs were fixed, some were simple 
long vs. int problems, in other cases both \fIftp\fP and \fIftpd\fP
insisted on freeing memory which had never been allocated (in a couple
cases part of the stack would be free'd!).
.IP 16)
\fImkpasswd\fP, \fIchpass\fP, \fIpasswd\fP, \fIvipw\fP, \fIlock\fP were
ported from the 4.3BSD shadow password implementation.  The password
file format has changed, and unlike the Vax version, the new password
files are not binary compatible with old programs.  Any program which
uses the password routines will have to be relinked with the new
libraries.
.IP
The maximum length of a login name has been increased from 8 to 15
under \*(2B.  This was dictated by having to share development
facilities with production systems where the length had been increased.
If this change is not acceptable, then the \fIutmp.h\fP file
will have to be edited and the system libraries and applications
recompiled.
.IP 17)
\fIrlogind\fP, \fIrshd\fP, \fIrcp\fP, \fIfingerd\fP, \fIsyslogd\fP,
\fIfinger\fP are all ported from Internet distributed
4BSD bugfix releases.  These programs include many security enhancements,
and in the process of porting them to \*(2B the long vs. int bugs
were fixed as well.
.IP 18)
\fIsendmail\fP had a fatal memory leak in alias processing.  A string
extraction method is used (thank you Cyrus) to reduce sendmail's
D space requirements by about 5kb - there is now the file 
\fI/usr/share/misc/sendmail.sr\fP used to hold much of sendmail's string
data.
.IP
\fIctimed\fP is a program which moves the time zone/daylight savings time
tables to a separate process, this saves approximately 2kb of D space
in sendmail.  The \fIctime\fP(3) entry points are replaced in 
\fIsendmail/src/ctime.c\fP with calls to \fIctimed\fP.  At present
only \fIsendmail\fP uses \fIctimed\fP, but the method may be used
by any process which needs to save about 2kb of D space.
.IP 19)
\fIvmstat\fP was modified to print out the namei cache statistics.  A
typo was corrected which caused \fIvmstat\fP to omit print the
DZ pseudo dma count.
.IP 20)
The assembler \fIas\fP underwent \fBMAJOR\fP changes to permit it to
run split I/D (the text being sharable speeds loading).  The buffer
sizes \fIas\fP uses to read and write files was doubled from 512 to
1024 bytes.
.IP 21)
\fIcsh\fP was overlaid because the password file library routines grew
in size.  The overlay scheme is optimal, the only time an overlay 
switch is incurred is when doing a ~ expansion.  \fIlimits\fP are
now enabled.
.IP 22)
\fIfsck\fP, \fIicheck\fP, \fIdcheck\fP, \fIncheck\fP, \fImkfs\fP,
\fImkproto\fP were all ported from 4.3BSD to handle the new
on disk directory structure.  The 30000 inode/file limit in \fIdcheck\fP
has been removed.  \fImkfs\fP no longer has the ability to populate
a filesystem with files, this capability has been moved to
\fImkproto\fP.  Note: \fImkproto\fP can only handle files up to single
indirect (about 256kb) in length.
.IP
\fIfsck\fP can now recover from a disconnected root inode!  also,
\fIfsck\fP can create and dynamically expand \fIlost+found\fP up
to the number of direct blocks allowed to an inode.  This limit is
comparable to the number of empty slots created by the obsolete
\fImklost\fP(8).  \fImklost\fP(8) has been renamed \fImklost+found\fP(8)
and uses 63 character file names to create the empty slots.
.IP 23)
\fImake\fP has been fixed to correctly handle library archive members
The man page was correct, \fImake\fP just had a bug.
.IP 24)
\fIld\fP had a bug in computing the size of the symbol table.  This
resulted in debuggers not being able to find symbols.  Other bugs
in the loadmap and trace options were fixed.  Modifications were also made to
accommodate the new \fIar\fP archive file format.
.IP 25)
\fIranlib\fP has the -t option from 4.3BSD now.  This allows the 
internal time on an object archive to be updated (typically used
after a copy which changed the timestamp on the file but not that
within the archive).  \fIranlib\fP was modified to handle the new \fIar\fP
archive file format.
.IP 26)
\fIlogin\fP is a new version ported from the 4.3BSD
shadow password file release.  This version of \fIlogin\fP performs
the password and account aging checks.
.IP 27)
\fIchroot\fP is a utility ported from 4.3Tahoe, used to execute a command
after issuing a \fIchroot\fP(2) system call.
.IP 28)
Both \fIwrite\fP and \fIwall\fP had long vs. int bugs.
.IP 29)
\fIdump\fP and \fIrestor\fP were modified to handle the new directory
structure.  \fIrestor\fP can read old \*(Ps \fIdump\fP tapes.
.IP 30)
\fImkhosts\fP now correctly handles a host having multiple addresses.
Although host table usage is discouraged in this era of domain name
service, if you want to use host tables they should at least work.
.IP 31)
The \fBhayes\fP dialer in the \fItip\fP program was totally broken.
The correct solution would have been to rewrite it to behave like the
\fIuucp\fP \fBhayes\fP dialer, but time did not permit this.  The fixed
version will work, but it's not pretty.
.IP 32)
The spooling system (\fIlpd\fP) was rife with long vs. int bugs, 
in particular the free space check could cause the connection to
be dropped.  Another problem, that of removing print jobs has been
fixed by implementing long file names - the concatenation of the hostname
to a queue id previously would result in mismatches of file names
when trying to remove remote requests.  A small change was made to permit
printing of the new ASCII \fIar\fP archive files.
.IP 33)
The pascal subsystem had a flaw in the \fIclock\fP() function, the
floating point conversions of the timeofday were wrong.
.IP 34)
\fIf77\fP had a bug in the \fBequivalence\fP handling caused by a
long vs. int problem.
.IP 35)
\fIrn\fP, \fIzmodem\fP, \fInotes\fP, \fIkermit\fP have been ported.
The overlay schemes for \fIrn\fP and \fInotes\fP are definitely not
optimal, but the programs will run.  There were numerous long vs. int
problems corrected, hopefully all were found, but there might be some
lurking about.
.IP 
\fIkermit\fP is a fairly modern version (4F(89)), the
DEBUG option had to be left disabled to fit the D space available.  The
comment about one of the modules crashing optimizers is correct - it has
to be compiled manually so that the remaining modules can use the -O
flag.
.IP 36)
A version of the Network Time Protocol has been ported.  Typical usage
is to use \fIntp\fP to synchronize the PDP-11 with a master system.
The daemon \fIntpd\fP does run and will use \fIadjtime\fP(2) to adjust
the system clock.
.IP 37)
The \fItraceroute\fP program has been ported, the kernel modifications
are included in \*(2B as well.
.IP 38)
\fIgetpwent\fP(3) could fail to recognize when a rewind of the password
file was necessary.
.IP 39)
\fIqsort\fP(3) could fail when sorting large arrays, a missing 'unsigned'
was the cause.
.IP 40)
\fIsignal(3)\fP misdeclared the saved signal masks as \fBint\fP rather than
\fBlong\fP.
.IP 41)
\fIstrdup\fP(3), \fIstrsep\fP(3), \fIreadv\fP(2), \fIwritev\fP(2),
\fIstrtok\fP(3) are new to \*(2B.
.IP 42)
\fIputs\fP(3) is implemented in C rather than assembly.  The assembly
version is still present on the system, but has a fatal flaw in
programs which run out of D space.  The port of \fBputs.s\fP from
the Vax assembly is correct, but the PDP-11 can fail to \fImalloc\fP(3)
a buffer where the Vax would succeed, the failure to allocate the
buffer causes file corruption when \fIputs\fP is called.
.IP 43)
\fIperror\fP, \fIgetusershell\fP, \fIrcmd\fP are new versions ported 
from 4.3BSD Internet releases.
.IP 44)
\fIgethostent\fP, \fIgethnamadr\fP have been rewritten to support
multiple addresses per host.
.IP 45)
\fIsyslog\fP had a long vs. int bug corrected in a \fIsignal\fP mask.
.IP 46)
\fIreaddir\fP was ported from 4.3BSD to handle the new directory
structure.
.IP 47)
\fIreadv\fP and \fIwritev\fP are system calls rather than
compatibility routines.
.IP 48)
\fIMail\fP incorrectly handled the editing of multiple messages, only
the first one was processed.
.IP 49)
\fIgcore\fP,\fIpstat\fP and \fIfstat\fP were
modified to deal with the new \fIu\fP structure.
.IP 50)
\fIrdist\fP ported from a 4.3BSD Internet release and the \fIlong\fP
vs. \fIint\fP bugs corrected.
.IP 51)
\fIlastcomm\fP could not handle large (>2048) uids.  also, a large 
amount of memory was wasted with static tables for the uid to name
mapping.  The uid to name mapping logic from \fIls\fP was adopted.
.IP 52)
\fImkstr\fP had a error in an ascii-octal to binary conversion.  This
could result in erroneous \fIstring\fP file being constructed.
.IP 53)
\fItcopy\fP incorrectly checked the number of arguments passed.
.IP 54)
\fIgetty\fP failed to handle adjacent colons in the \fIgettytab\fP
file which would cause skipping of a field.
.IP 55)
\fIsa\fP incorrectly handled uids which are signed.  The fix was to 
use \fIuid_t\fP instead of int.
.IP 56)
\fIrwhod\fP created files unreadable by \fIrwho\fP.  Changed \fIrwhod\fP
to create publicly readable files.
.IP 57)
\fItestnet\fP is a new program used by \fI/etc/rc\fP to test for
a networking kernel.
.IP 58)
\fItelnet\fP has had several bugs in the open and close handling fixed.
.IP 59)
\fIar\fP was ported from 4.3BSD.  Archive files now use a portable ASCII
format rather than embedding binary information in the archive headers.
If printable files ar \fIar\fP'd together, the resulting archive is
printable.
.IP 60)
\fIfile\fP was ported from 4.3BSD.
.IP 61)
\fInm\fP was heavily modified to deal with the new \fIar\fP file format.
At the same time changes were made which permit \fInm\fP to list the
symbol table of \fI/unix\fP without running out of memory.
.IP 62)
The \fIman\fP pages for \fIranlib\fP, \fIar\fP, \fIfile\fP have been updated
or copied from 4.3BSD.
.IP 63)
\fIarcv\fP was ported from 4.3BSD, it is used to convert (in place) old
format \fIar\fP archives to the new portable format.  ALL system libraries
have been converted, \fIarcv\fP will only be needed to convert local 
archives.  See \fI/usr/src/old/arcv\fP for the source and man page.  The
executable is already installed in \fI/usr/old\fP.

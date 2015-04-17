.NH
Changes in application programs.
.PP
The application directories have also been rearranged in an effort
to mimic the 4.3BSD release.  The following table indicates the old and new
names as well of the functions of these directories.  For more information,
see the
.UX
manual page for \fIhier\fP(7).
.TS
center box;
l | l | l
l | l | l.
new name	old name	purpose
_
bin	cmd	commands found in /bin
etc	cmd	commands found in /etc
games	games	commands found in /usr/games
include	include	files found in /usr/include
lib	lib	commands found in /lib
local	local	commands found in /local
new	contrib	commands found in /usr/new
old	N/A	commands found in /usr/old
ucb	ucb	commands found in /usr/ucb
usr.bin	cmd	commands found in /usr/bin
usr.lib	lib	commands found in /usr/lib
.TE
Many new commands have been added, and several have been removed; whenever
possible we've dropped the 4.3BSD source directories in place.  As in the
kernel, 4.3BSD support was usually the deciding factor as to a program's
status.  In many of the source directories there are either or both of
the special directories ``PORT'' and ``OLD''.  PORT contains
copies of 4.3BSD source that we feel ought to be ported to 2.10BSD.
OLD contains copies of 2.9BSD source that, while we have ported
the 4.3BSD version of the source code, we're unsure enough of it that we
wanted to provide a backup copy.  The following paragraphs are a description
of several things that have changed outside of the kernel.
.IP 1)
Since the 2.10BSD C compiler still only recognizes seven significant
characters in external names, several standard library names had to be
changed to prevent name collisions.  However, to prevent portability
problems in your programs you should use the standard names.  All known
collisions in the standard include files or the C library have been handled
either in the include file itself or in the include file
\fIinclude/short_names.h\fP.  This works because we're using the 4.3BSD C
preprocessor, which has flexnames.  Networking programs almost always need
to include \fIshort_names.h\fP.  See \fIsrc/bin/mail.c\fP and
\fIsrc/bin/passwd.c\fP for examples of long name work arounds.  The C
library itself is a port of the 4.3BSD C library.
.IP 2)
Files ported from 4BSD systems that have more than MAXNAMLEN
characters will port correctly as long as they are unique in
those characters.  For example, the kernel will translate an open
call of the form:
.nf
.ft B

	open("/usr/man/man1/file_with_too_long_a_name",O_RDONLY,0);
.ft R

as if it were:

.ft B
	open("/usr/man/man1/file_with_too_",O_RDONLY,0);
.ft R
.fi
.IP 2)
The new directory reading routines are now part of stdio.  We found that
it was usually easier to put the directory reading routines into the
code than to handle both the old and the new structures.  We haven't
changed the actual file system.  If you \fIhave\fP to have the old directory
stuff, i.e. you are reading the actual device, convert your code to
use the following structure, also found in \fIh/dir.h\fP.
.nf
.ft B

	#define    MAXNAMLEN  14

	struct     v7direct   {
	           ino_t      d_ino;
	           char       d_name[MAXNAMLEN];
	};
.ft R
.fi
.bp
This allows access to both the old and new directory structures.
Make sure that you change all of the references, e.g. ``struct direct''
should be ``struct v7direct''.  \fIFsck\fP(8) and \fIdump\fP(8) are
good examples of programs that have to do this.  Since the define
``DIRSIZ'' has changed in 4BSD (in V7 it was 14, the length of a
file/directory name) we've
replaced all occurrences of ``DIRSIZ'' in 2.10BSD with the corresponding
4BSD define, ``MAXNAMLEN''.
.IP 3)
The 2.10BSD stdio allocates space for file I/O buffering differently
than 2.9BSD did.  If you have programs that use buffered I/O and are
complaining about bad input, or just dying horribly, and the input looks
okay, start looking at the way it handles \fIsbrk\fP(2).  If it calls
\fIsbrk\fP with an argument of 0 and then uses that offset
to figure out where the rest of its allocated objects are, you've probably
found the problem.  A simple work-around is to allocate the necessary
stdio buffers as part of the program's data space and then use
\fIsetbuf\fP(3) to so inform stdio.  See the C compiler for examples.
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
should be ``longs'' is an art.  Routines that we look for as a matter
of habit are any one of the \fIseek\fP(2) routines, \fIftell\fP(3),
\fItime\fP(2), the various \fIsignal\fP(2) routines and the
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
.fi
This particular type of low word masking bit us no less than 33 times in
the kernel.  Other possibilities that are even more annoying:
.bp
.nf
.ft B
.ta .5i +\w'#define   'u +\w'FLAG32    'u

	#define	FLAG1	0x00000001
	...
	#define	FLAG16	0x00008000
	...
	#define	FLAG32	0x80000000

.ft R
.fi
Using any of the above definitions to test or set flag values in a long
quantity will work, \fBexcept FLAG16\fP.
.IP 5)
The PDP-11 \fIsetjmp\fP(2) implementation contains a subtle bug that
occurs when a routine containing a \fIsetjmp\fP has \fIregister\fP
variables.  The bug sometimes causes those variables to be given
invalid values when a longjmp is made back to the routine.  This is
probably impossible to fix in a reasonable manner, and it's much simpler
to simply avoid register variables in such routines.
.IP 6)
The optional '#' character is not supported by \fIprintf\fP(3).
.IP 7)
The DEC MXV11 bootstrap ROM, for the RL's, TU's, RX's, and RD's among
others, \fIrequires\fP that deadstart blocks begin with an 0240 and a
branch. Noone seems to know why.  This has already been implemented
in \fIrauboot.s\fP and \fIrluboot.s\fP in \fIsys/mdec\fP.
.IP 8)
To port the 4.3BSD \fImake\fP(1) program, several of its table sizes had
to be reduced.  Make is very unforgiving of Makefiles that are too large.
If make drops core for no reason that you can think of, try reducing the
size of the Makefile.  Also, don't run make depend in the system
application directories, make can't handle it.
.IP 9)
Don't set csh history too high; it eventually runs out of space
and logs you out.
.IP 10)
The games directory is largely untested.
.IP 11)
There's a new (and wonderful) compiler, that actually handles bit fields,
identically named global structure elements, and lots of other stuff.
It \fIdoesn't\fP handle any of the old assignment operators, and, not only
doesn't handle them, but \fBproduces bogus code\fP.  It is \fBSTRONGLY\fP
recommended that you read the file \fIsrc/lib/ccom/TEST/README\fP.  It goes
into the problems with this compiler in more depth and contains some other
Extremely Important Information.
.IP 12)
The 4.3BSD loader uses the \fI-L\fP flag to specify a list of library
directories that should be searched for libraries specified with the
\fI-l\fP option.  The old meaning of the \fI-L\fP flag, the termination
of overlaid text, is now the function of the \fI-Y\fP flag.
.IP 13)
\fIreadv\fP(2) and \fIwritev\fP(2) are implemented as compatibility
routines in the standard library and are semantically identical to the
4.3BSD \fIreadv\fP and \fIwritev\fP on all current descriptors, with the
exception of sockets (SOCK_STREAM does work identically however).  In other
words, the fact that the compatibility routines implement
\fIreadv\fP and \fIwritev\fP non-atomically as multiple \fIread\fP(2)
and \fIwrite\fP(2) calls doesn't normally matter; disk files don't
care, tape reads and writes via read/writev under 4.3BSD don't work
atomically anyway, and the compatibility code for \fIreadv\fP's on a
tty seems to work as advertised.
.IP 14)
Any limit other than RLIMIT_CPU, RLIMIT_CORE, or RLIMIT_FSIZE will be
ignored when passed to the kernel by \fIsetrlimit\fP.  A subsequent
call by \fIgetrlimit\fP will return the correct value, however.  The
three limits listed above are fully implemented in the kernel.
.IP 15)
There is a define, ``SEPFLAG'', in many Makefiles, that governs compilation
for separate and non-separate I/D machines.  If you have a non-separate I/D
machine, set it to ``-n''.  If you have a separate I/D machine, set it to
``-i''.  This define needs to be set in \fIMakefile\fPs in several
directories, each of which is found in \fIusr/src\fP.  These directories
are \fIbin\fP, \fIetc\fP, \fIgames\fP, \fIlib\fP, \fIlocal\fP, \fIold\fP,
\fIucb\fP, \fIusr.bin\fP, and \fIusr.lib\fP.  Each of these \fIMakefile\fPs
has been written to pass this flag down to its subdirectories.  The default
``SEPFLAG'' value is ``-i''.  You may also enter the definition
``SEPFLAG=-i'' in your shell environment, rather than explicitly
within the various makefiles.  See \fImake\fP(1) for more information.
.IP 17)
The directory \fIusr/src/new\fP is a compendium of things that we moved
in from the 4.3BSD directory \fIusr/src/new\fP and things we moved in from
from the 2.9BSD \fIusr/src/local\fP and \fIusr/src/contrib\fP directories.
They come with even less of a guarantee than the rest of the system.  If
that's possible.

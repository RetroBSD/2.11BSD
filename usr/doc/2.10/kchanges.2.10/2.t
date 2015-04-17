.NH
Changes inside the kernel
.IP 1)
Code which previously used the \fIiomove\fP, \fIpassc\fP, or \fIcpass\fP
routines will have to be modified to use the new \fIuiomove\fP,
\fIureadc\fP, and \fIuwritec\fP routines.  These routines are
faster than the 2.9BSD versions as well as having the added facilities
of mapping in user space for large transfers and transferring odd
lengths of data or data to odd addresses without the serious performance
degradation seen in 2.9BSD.
.IP 2)
The calling convention
for the driver \fIioctl\fP routine has changed.  Any data copied in or out
of the system is now done at the highest level, inside \fIioctl\fP\|().  The
third parameter to the driver \fIioctl\fP routine is a data buffer passed by
reference.  Values to be returned by a driver must be copied into the
associated buffer from which the system then copies into the user
address space.  It should also be noted that \fIioctl\fP's are 32-bit
quantities, not 16-bit as in 2.9BSD, although the kernel still stores
them in 16 bits internally.  These changes are reflected in \fIioctl.h\fP.
Also, the \fIread\fP, \fIwrite\fP, and \fIioctl\fP entry points in device
drivers must return 0 or an error code from <\fIerrno.h\fP>. This is a
reflection of the effort to have kernel routines return errors rather than
setting global variables to error values.  When porting your local device
drivers make sure that they correctly return these errors.
.IP 3)
Assembly routines which make system calls will not port from 2.9BSD or
earlier systems to 2.10BSD, as the system call protocol to the kernel has
changed a great deal.  Arguments are \fBalways\fP passed into the kernel on
the stack, as opposed to registers and/or data space.  This change was
made for a number of reasons, and triggered by the need for reliable
signal delivery.  The file \fIusr/include/syscall.h\fP details the new
entrance values.  The 2.9BSD protocol of using the high bit of the syscall
value to indicate that the arguments are on the stack has also been
removed.  The \fIsyslocal\fP (58) call has disappeared entirely as the
\fIsyslocal\fP table has been folded into the \fIsysent\fP table.  The
\fIindir\fP (0) call has also been deleted.  If you have any assembly
source which makes system calls directly, it is strongly recommended that
you convert them to simply push their parameters onto the stack and call
the standard library routines.  \fBObviously, there is no binary
compatibility with previous versions of PDP
.UX.
2.10BSD is, however, binarily compatible with 2.10.1BSD.
.IP
\fRThere have been many changes in the system calls available to user
programs, as approximately thirty new system calls have been added and
five or so deleted.  Use of a deleted system call is fairly easy to
detect since the kernel delivers a SIGSYS signal to any program
requesting a non-existent system call.  The one exception to this rule
is the use of a \fIsocket\fP(2) or \fIsocketpair\fP(2) call on a system
without networking.  Either of these calls will simply return an error
so that networking programs can fail correctly.
.IP 4)
On the PDP-11 only some of the \fIrusage\fP and \fIitimerval\fP structures
are meaningful.  In order to minimize the data space required by the user
and proc structures in the kernel, the kernel has private copies of these
structures.  Only programs such as \fIps\fP(1) and \fIvmstat\fP(1), that
read through the
proc structure, will need to know about the kernel's versions of these
structures.  Any unused members of these structures are zero'd out
before being returned to a user process.
.IP 5)
Both the kernel and application systems may now have up to NOVL (15)
overlays.  This feature requires modification of \fIld(1)\fP, \fIadb(1)\fP,
the C library, the kernel, \fIstrip\fP(1), and \fInm\fP(1) among other
things.  If you're
not hearing what I'm saying, don't mess with it.  Separate libraries
are no longer required for overlaid and non-overlaid programs.  It
turned out that it
cost approximately 3.89 micro-seconds per subroutine call on a PDP 11/44
to combine
the two.  Therefore, all programs should use the standard library (libc.a)
for loading.  The -V flag is ignored by \fIcc\fP(1) and \fIf77\fP(1);
they automagically pass it to the assembler and the various passes of the
C compiler.  (-V wasn't hardwired into the assembler so that it would
still be possible to run \fIas\fP(1) without the -V flag and get an
executable without having to use the loader.)  If you call
the assembler directly, remember to pass it the -V
flag.  Also, the C library should not be loaded as an overlay, as many of
its routines don't use the csv/cret protocol.
.IP 6)
The process table has grown considerably since its original incarnation.
Several parts of the kernel used a linear search of the entire table to
locate a process, a group of processes, or group of processes in a certain
state.  In order to reduce the time spent examining the process table,
several changes have been made.  The first is to place all process table
entries onto one of three linked lists, one each for entries in use by
existing processes (\fIallproc\fP), entries for zombie processes
(\fIzombproc\fP), and free entries (\fIfreeproc\fP).  This allows the
scheduler and other facilities that must examine all existing processes
to limit their search to those entries actually in use.  Searches for
unused process id's are avoided by noting a range of usable process ID's
when trying to generate a new, unique ID.  Searches for individual processes
are avoided by using linked lists of hashed pid's.
.IP 7)
In order to avoid searching through the list of open files when the
actual number in use is usually small, the index of the last used open
file slot is maintained in the field \fIu.u_lastfile\fP.  The routines
that implement open and close or implicit close (\fIexit\fP
and \fIexec\fP) maintain this field, and it is used whenever
the open file array \fIu.u_ofile\fP is scanned.
.IP 8)
The values for \fInice\fP used in 2.9BSD and previous systems
ranged from 0 though 39.  Each use of these scheduling parameters
was offset by the default, NZERO (20).  This has
been changed in 2.10BSD to use a range of -20 to 20, with NZERO
redefined as zero.
.IP 9)
There have been an large number of changes in the types defined
and used by the system.  The system file \fIsys/types.h\fP should
be serve as the final description of these new types.  Of particular
interest are the new typedefs that have been added for user ID's and
group ID's in the kernel and common data structures.  These typedefs,
\fIuid_t\fP and \fIgid_t\fP, are both of type \fIu_short\fP.  This
change from the previous usage (explicit \fIshort\fP ints) allows user
and group ID's greater than 32767 to work reasonably.  Other changes
include the conversion of the \fIlabel_t\fP type to a structure, the
addition of a \fIquad\fP (64-bit) type, and the much more extensive
use of such types as \fIdaddr_t\fP, \fImemaddr\fP, \fIoff_t\fP, and
\fIcaddr_t\fP.
.IP 10)
The berknet driver didn't work in 2.9BSD, and we've done nothing to change
that.  In any case, it's been moved to the \fIsys/OTHERS\fP directory with
all of the other defunct drivers.  We have provided a version of the 4.3BSD
driver since it will understand the other changes to the kernel made for
2.10BSD, although it won't work either.  See the README.  Enjoy.
.IP 11)
The routines \fIschar\fP and \fIuchar\fP have gone away and have
been replaced by \fIcopyinstr\fP and \fIcopyoutstr\fP, routines
that copy a NULL terminated string of characters in or out of
kernel space.  Also available are \fIvcopyin\fP, \fIvcopyout\fP,
and \fIcopystr\fP.  The first two routines copy odd numbers of
bytes and data to and from odd addresses efficiently.  \fICopystr\fP
copies a NULL terminated string of characters within kernel space.
.IP 12)
Directories with more than 10 trailing empty slots will be automagically
truncated whenever a new file is created within them.  This creates
a minor problem: if someone creates an entry in the
\fIlost+found\fP directory, that directory will be cheerfully
truncated to a very small length.  Since we didn't have the time to
really fix the problem, we changed \fImklost+found\fP(8) to leave a
special file in the very last slot of the \fIlost+found\fP directory.  It
would probably be a good idea for you to remake your \fIlost+found\fP
directories, just in case.  Of course, the correct fix is to port the
4.3BSD fsck; I'd really like a copy when you're done.
.IP 13)
The ``real-time'' features of 2.9BSD have been left in place, and, should
work as they always have, with one major exception.  The \fIfmove\fP()
routine has not been fixed to be interruptible.  See the \fIcopy\fP()
routine for examples of what needs to be done to make it behave correctly.
This, however, will be fairly difficult.  We suggest that if you want to
use \fBCGL_RTP\fP that you comment out the use of \fIuiofmove\fP() in
\fIuiomove\fP().
.IP 14)
Most of the conditional compilation defines in the 2.9BSD kernel have
been removed because the features they controlled are now either standard
or no longer supported in 4.3BSD.  Other features have been grouped
together and are now controlled by the same define.
.PP
The following table lists \fI#defines\fP that have been removed
from 2.10BSD.
.TS
center box;
l | l | l
l | l | l.
define name	feature	comment
_
DISKMON	keep statistics on the buffer cache	absorbed by UCB_METER
INTRLVE	interleave file systems across devices	not supported in 4.3BSD
MPX_FILS	multiplexed files	not supported in 4.3BSD
UCB_GRPMAST	``group'' super-users	not supported in 4.3BSD
UCB_LOGIN	``login'' system call	not supported in 4.3BSD
UCB_PGRP	V7 bug fix	supplanted by job-control
UCB_SUBM	``submit'' system call	not supported in 4.3BSD
UNFAST	turn off inline macros
UCB_QUOTAS	dynamic disk quota scheme
TEXAS_AUTOBAUD	getty feature	part of ported 4.3BSD source
.TE
.bp
.PP
The following \fI#defines\fP are new or remain largely unchanged since 2.9BSD.
Note, no software outside of the kernel needs to be aware of the
``VIRUS_VFORK'' flag.  The kernel automatically maps \fIvfork(2)\fP
into \fIfork\fP(2) if the \fIvfork\fP call isn't available.
.TS
center box;
l | l | l
l | l | l.
define name	feature	comment
_
CGL_RTP	support one ``real-time'' process	untested
DIAGNOSTIC	more stringent error checking	now more expensive to run
ENABLE34	Able's MM board	untested in quite some time
HZ	line clock frequency	renamed "LINEHZ"
MAXMEM	limit process memory
NOKA5	only buffers and clists use KA5
Q22	22-bit QBUS
QUOTA	dynamic file system quotas	\fBvery\fP expensive
SMALL	smaller queues and hash tables	untested in quite some time
UCB_CLIST	map out clists
UCB_FRCSWAP	force swap on forks/expands
UCB_METER	various system statistics	expanded; expensive to run
UCB_NET	Berkeley's TCP/IP	a 4.3BSD interface
UCB_RUSAGE	resource accounting
UNIBUS_MAP	18-bit Unibus mapping
VIRUS_VFORK	the \fIvfork\fP(2) system call
.TE
.sp 2
.PP
The following table lists \fI#defines\fP that are now a standard part
of 2.10BSD.
.TS
center box;
l | l | l
l | l | l.
define name	feature	comment
_
ACCT	keep accounting records
DZ_PDMA	pseudo DMA for DZ boards
INSECURE	retain set-id bits	now follows 4.3BSD behavior
MENLO_OVLY	support user text overlays
MENLO_JCL	support job control
MENLO_KOV	support kernel overlays
OLDTTY	V7 tty line discipline	part of the 4.3BSD tty driver
PARITY	handle cache and memory parity traps
TM_IOCTL	ioctl calls in the TM-11 driver
TS_IOCTL	ioctl calls in the TS-11 driver
UCB_AUTOBOOT	allow automatic rebooting
UCB_BHASH	hash buffer headers
UCB_DEVERR	translate device error messages
UCB_ECC	correct soft ecc disk transfer errors
UCB_FSFIX	update files in correct order
UCB_IHASH	hash in-core inodes
UCB_LOAD	compute Tenex style load average
UCB_NKB	set file system block size	always set to ``1''
UCB_NTTY	Berkeley line discipline	part of the 4.3BSD tty driver
UCB_RENICE	support \fIrenice\fP(2) system call
UCB_SCRIPT	scripts may specify interpreters
UCB_SYMLINKS	support symbolic links
UCB_UPRINTF	send error messages to users
UCB_VHANGUP	revoke access to tty after logout
.TE

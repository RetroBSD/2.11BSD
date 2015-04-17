.nr i \w'\fBkern_resource.c\0'u/1n
.de XP
.IP \\fB\\$1\\fP \\ni
..
.NH
Organizational changes
.PP
The directory organization and file names are very different
from 2.9BSD.  The new directory layout breaks machine-specific
and network-specific portions of the system out into separate
directories.  A new file, \fImachine\fP, is a symbolic link to a
directory for the target machine, in this case, \fIpdp\fP.  This allows a
single set of sources to be shared between multiple machine types
(by including header files as \fI../machine/file\fP).
The directory naming conventions, as they relate to the
network support, are intended to allow expansion in supporting
multiple ``protocol families''.   The following directories
comprise the system sources for the PDP:
.TS
lw(1.0i) l.
/sys/OTHERS	defunct PDP UNIBUS device drivers
/sys/autoconfig	PDP dependent autoconfiguration code
/sys/bootrom	PDP dependent boot rom code
/sys/conf	site configuration files and basic templates
/sys/h	machine independent include files
/sys/machine	symbolic link to /sys/pdp
/sys/mdec	PDP dependent deadstart (block zero) code
/sys/net	network independent, but network related code
/sys/netimp	IMP support code
/sys/netinet	DARPA Internet code
/sys/netns	DARPA Internet code
/sys/netpup	PUP-1 support code
/sys/pdp	PDP specific mainline code
/sys/pdpdist	PDP distribution files
/sys/pdpif	PDP network interface code
/sys/pdpstand	PDP dependent stand-alone code
/sys/pdpuba	PDP UNIBUS device drivers and related code
/sys/pdpmba	PDP MASSBUS device drivers and related code
/sys/sys	machine independent system source files
/sys/vaxdist	VAX distribution files
/sys/vaxif	VAX network interface code
/sys/vaxuba	VAX UNIBUS device drivers and related code
.TE
.DE
.PP
Files indicated as \fImachine independent\fP are shared between
all
.UX
systems.  Files
indicated as \fImachine dependent\fP are located in directories
indicative of the machine on which they are used; the 2.10BSD
release from Berkeley only contains support for the PDP.
Files marked \fInetwork independent\fP form the ``core'' of the
networking subsystem, and are shared among all network software;
the 2.10BSD release only contains support
for the DARPA Internet protocols IP, TCP, UDP, and ICMP.
.NH 2
/sys/h
.PP
Files residing here are intended to be machine independent.  Consequently,
the header files for device drivers, which were present in this directory
in 2.9BSD, have been moved to other directories; e.g. \fI/sys/pdpuba\fP,
\fI/sys/netinet\fP, etc.  Many
files which had been duplicated in \fI/usr/include\fP are now present only
in \fI/sys/h\fP.  Further, the 2.9BSD \fI/usr/include/sys\fP directory is
now, normally,
a symbolic link to this directory.  By having only a single copy of these
files the ``multiple update'' problem no longer occurs.  (It is still
possible to have \fI/usr/include/sys\fP be a copy of the \fI/sys/h\fP
for sites where
it is not feasible to allow the general user community access to the
system source code.)  For further information, see the Makefile for
\fIusr/src/include\fP.
.PP
In general, the include files for 2.10BSD have been reworked to
be as close as possible to 4.3BSD.  In many cases, they have even been
renamed.  This may cause problems when attempting to port
local software, although it should facilitate porting software
developed under any version of 4BSD.
.PP
The following files are new to /sys/h in 2.10BSD.
.XP clist.h
contains the \fIcblock\fP structure definition previously found in
tty.h
.XP dk.h
contains the disk and tty monitoring information previously found
in \fIsystm.h\fP.
.XP domain.h
describes the internal structure of a communications domain; part of
the new ipc facilities
.XP errno.h
had previously been only in \fI/usr/include\fP; the file
\fI/usr/include/errno.h\fP is now a symbolic link to this file
.XP exec.h
the kernel's definition of the exec structure, as well as the
overlay structure and the magic number definitions
.XP fs.h
replaces the old filsys.h description of the file system organization
.XP gprof.h
describes various data structures used in profiling the kernel; see
\fIgprof\fP\|(1) for details; not yet implemented under 2.10BSD
.XP kernel.h
is an offshoot of systm.h and param.h; contains constants and
definitions related to the logical
.UX
``kernel''
.XP mbuf.h
describes the memory management support used mostly by the networking;
see ``4.2BSD Networking Implementation Notes'' for more information
.XP msgbuf.h
describes the kernel message buffer
.XP namei.h
defines various structures and manifest constants used in
conjunctions with the \fInamei\fP routine
.XP protosw.h
contains a description of the protocol switch table and related
manifest constants and data structures use in communicating with
routines located in the table
.XP ptrace.h
contains definitions for process tracing
.XP quota.h
contains definitions related to the 4.3BSD disk quota facilities.
.XP resource.h
contains definitions used in the \fIgetrusage\fP,
\fIgetrlimit\fP, and \fIgetpriority\fP system calls (among others)
.XP socket.h
contains user-visible definitions related to the new socket ipc
facilities
.XP socketvar.h
contains implementation definitions for the new socket ipc facilities
.XP syslog.h
contains definitions for the system logging facility
.XP tablet.h
contains structures and definitions concerning the line table
discipline
.XP trace.h
contains definitions and storage for file system buffer tracing points
.XP ttychars.h
contains definitions related to tty character handling; in particular,
manifest constants for the system standard erase, kill, interrupt,
quit, etc. characters are stored here (all the appropriate user
programs use these manifest definitions)
.XP ttydev.h
contains definitions related to hardware specific portions of tty 
handling (such as baud rates); to be expanded in the future
.XP uio.h
contains definitions for users wishing to use the new scatter-gather
i/o facilities; scatter-gather is unimplemented in 2.10BSD, although
compatibility routines are available.  These compatibility routines
will behave exactly as a ``correct'' implementation would except for
a few fairly unlikely situations.
.XP un.h
contains user-visible definitions related to the ``unix'' ipc domain
.XP unpcb.h
contains the definition of the protocol control block used
in the ``unix'' ipc domain
.XP vlimit.h
contains definitions for kernel limits.
.XP vm.h
a quick way to include all of the vm header files
.XP vmmac.h
contains various macros concerning memory
.XP vmmeter.h
contains structures for monitoring various kernel functions which
were in vm.h; several new items are metered: floating point simulation
traps, number of calls to swapout and swapin (v_swpout/v_swpin); v_pgout/v_pgin
count the number of I/O operations to and from the swap device (these use to
be counted in v_swpout/v_swpin); synchronous software traps are no longer
counted in v_intr; network soft interrupts are now counted in v_soft.
.XP vmparam.h
memory definitions
.XP vmsystm.h
various memory variables and structures
.XP vtimes.h
memory metering structures
.XP wait.h
contains definitions used in the \fIwait\fP and \fIwait3\fP\|(2)
system calls; previously in \fI/usr/include/wait.h\fP
.PP
The following files have undergone significant change:
.XP buf.h
several macros to replace rarely used routines and for dealing with buffer
chains have been added.  A new structure, \fIbufhd\fP, has been added.  The
\fIb_link\fP field has been removed from the buffer structure.  Of particular
interest may be a macro for the translation of a buffer address to a physical
address.  Many of the buffer flag values have been changed; note that the
addition of any more flag values will require changing the flag word,
\fIb_flags\fP, to a long.
.XP callout.h
the callout structure has been changed; kernel callouts are now implemented
as a linked list.
.XP conf.h
reflects changes made for the new \fIselect\fP\|(2) system call;
the character device table has a new member, \fId_select\fP, which
is passed a \fIdev_t\fP value and an FREAD (FWRITE) flag and returns
1 when data may be read (written), and 0 otherwise.  The \fIbdevsw\fP
structure has lost the \fId_tab\fP entry and gained a flags field,
\fId_flags\fP.  A new structure has also been added, \fIlinesw\fP,
to support the use of various line protocols.
.XP dir.h
is completely different since definitions for the user level interface
routines described in \fIdirectory\fP\|(3) are included
.XP file.h
has a very different \fIfile\fP structure definition and definitions for
the new \fIopen\fP and \fIflock\fP system calls; the symbolic definitions
for many constants commonly supplied to \fIaccess\fP and \fIlseek\fP,
have been changed since 2.9BSD to bring them into line with 4.3BSD.  A
new member has been added, \fIf_type\fP.  This member indicates the type
of structure pointed to by \fIf_data\fP, and should be either DTYPE_INODE,
DTYPE_PIPE, or DTYPE_SOCKET, constants found in \fIh/file.h\fP.
.XP inode.h
some of the standard macros have been renamed; the in-core inode structure
has been rearranged to take advantage of areas of commonality between it
and the dinode structure.  A complete set of definitions for accessing
various sub-fields have been added, and explicit references in the kernel
have been removed.  The access, modification and status change times have
been added to the incore inode structure, as well as new fields for the
advisory locking facilities and a back pointer to the file system super block
information.  A pointer to a text object has also been added.  The dinode
structure is now defined here.  Also, various definitions for the
\fIopen\fP(2), \fIseek\fP(2), and \fIaccess\fP(2) calls have been added
and/or changed.
.XP ioctl.h
has all request codes constructed from _IO, _IOR, _IOW, and _IOWR macros
which encode whether the request requires data copied in, out, or in and
out of the kernel address space; the size of the data parameter (in bytes)
is also encoded in the request, allowing the \fIioctl\fP\|() routine to
perform all user-kernel address space copies
.XP localopts.h
is now generated by the script \fIconfig\fP, which is found in the
\fIsys/conf\fP directory; it contains far fewer values than it did
before, mostly as a result of the removal of most of the user defined
options from 2.9BSD, as well as containing a few new ones.
.XP param.h
has had numerous items deleted from it; in particular, many definitions
logically part of the ``kernel'' have been moved to \fIkernel.h\fP, and
machine-dependent values and definitions previously found in \fIparam.h\fP
have been moved to \fImachine/machparam.h\fP; it now contains a manifest
constant, NGROUPS, which defines the maximum size of the group access list.
.XP proc.h
has changed extensively as a result of the new signals, the different
resource usage structure, and the new timers; in addition, new members
are present to support the status information required by new system
calls as well as to provide the linked lists and hashing required by
new access methods.
.XP signal.h
reflects the new signal facilities; several new signals have been
added; structures used in the \fIsigvec\fP\|(2) and \fIsigstack\fP\|(2)
system calls, as well as signal handler invocations are defined here
.XP stat.h
has been updated to reflect the changes to the inode structure; in
addition several new fields have been added.  One of those fields,
\fIst_block\fP, claims to be the number of blocks used by the file.  It
isn't really, it's the size of the file in bytes rounded up to a kilobyte
boundary.  This results in some programs, notably \fIdu\fP(1), believing
ridiculous file sizes.
.XP systm.h
has been trimmed back a bit as various items were moved
to \fIkernel.h\fP and other include files
.XP text.h
two pointers have been added to the text structure to support LRU
caching of text objects.  The reference and loaded reference counters
are now unsigned values.
.XP time.h
contains the definitions for the new time and interval
timer facilities
.XP tty.h
reflects changes made to the internal structure of the
terminal handler
.XP types.h
reflects several new types, and has been restructured
.XP user.h
has been extensively modified; members have been grouped and categorized
to reflect the ``4.2BSD System Manual'' presentation; new members have
been added and existing members changed to reflect the new groups facilities,
changes to resource accounting and limiting, new timer facilities, and new
signal facilities
.NH 2
/sys/sys
.PP
This directory contains the ``mainstream'' kernel code.  Files in this
directory are intended to be shared between 2.10BSD implementations on
all machines.  As there is little correspondence between the current
files in this directory and those which were present in 2.9BSD, a
general overview of each files's contents will be presented rather than
a file-by-file comparison.  This is also where we will discuss some of
the major changes that have been made to various areas of the kernel.
.PP
Files in the \fIsys\fP directory are named with prefixes
which indicate their placement in the internal system
layering.  The following table summarizes these naming
conventions.
.TS
lw(1.0i) l.
init_	system initialization
kern_	kernel (authentication, process management, etc.)
quota_	disk quotas
sys_	system calls and similar
tty_	terminal handling
ufs_	file system
uipc_	interprocess communication
vm_	memory
.TE
.DE
.NH 3
Initialization code
.XP init_main.c
contains system startup code
.XP init_sysent.c
contains the definition of the \fIsysent\fP table \- the table of system
calls supported by 2.10BSD.
.NH 3
Kernel-level support
.XP kern_acct.c
contains code used in per-process accounting
.XP kern_clock.c
contains code for clock processing; work was done here to minimize time
spent in the \fIhardclock\fP routine.
.XP kern_descrip.c
contains code for management of descriptors; descriptor
related system calls such as \fIdup\fP and \fIclose\fP (the
upper-most levels) are present here
.XP kern_exec.c
contains code for the \fIexec\fP system call.  Argument and environment
copies in exec are now done on a per-string rather than a per-character
basis.
.XP kern_exit.c
contains code for the \fIexit\fP and \fIwait\fP system calls
.XP kern_fork.c
contains code for the \fIfork\fP and \fIvfork\fP system calls.  The
system now keeps a range of pids it can directly assign to new processes.
.XP kern_mman.c
contains code for memory management related calls
.XP kern_proc.c
contains code related to process management; in particular,
support routines for process groups
.XP kern_prot.c
contains code related to access control and protection;
the notions of user ID, group ID, and the group access list
are implemented here
.XP kern_resrce.c
code related to resource accounting and limits; the
\fIgetrusage\fP and ``get'' and ``set'' resource
limit system calls are found here
.XP kern_rtp.c
code supporting the ``real-time'' processing features of 2.10BSD;
these features are essentially untested since 2.9BSD.
.XP kern_sig.c
the signal facilities; in particular, kernel level routines
for posting and processing signals
.XP kern_subr.c
support routines for manipulating the kernel I/O structure:
\fIuiomove\fP, \fIuiofmove\fP, \fIureadc\fP, and \fIuwritec\fP
.XP kern_synch.c
code related to process synchronization and scheduling:
\fIsleep\fP and \fIwakeup\fP among others.  The scheduler no longer scans
the process table once per second, it only considers runnable processes
when recomputing priorities.  Sleeping processes get their priority
bump when they get put back on the run queue.
.XP kern_time.c
code related to processing time, the handling of interval timers and time of
day clock.  Kernel timeouts are now implemented as a linked list.  There are
several minor differences in the treatment of the timers in 2.10BSD.  All
real-time timer calls are to one second resolution, not to a clock tick as
offered in 4.3BSD.  (Incidentally, while the \fIualarm\fP(3) and
\fIusleep\fP(3) calls are available, this one second resolution makes them
useless on the 2.10BSD.)  We also compute the real-time timer for
all processes at the same time, as part of the cpu scheduler, rather than as
a timeout, as is done in 4.3BSD.  This was done to allow the timeout offset,
\fIc_time\fP, to be stored in the kernel as a int, instead of a long,
minimizing the number of long operations done at clock interrupt.  If the
kernel ever requires a timeout of greater than 9 minutes, it should be
converted to a long, at which point the real-time timer calls should be
done as timeouts.  The other timers, which are necessarily processed at
clock interrupt, are stored internally as ticks, not as seconds, but only
one second resolution is offered by the system call interface to be
consistent with the real time timer.  The RLIMIT_CPU is also stored
internally as clock ticks.  The only noticeable effect of this is that very
large values supplied with the RLIMIT_CPU option of \fIgetrlimit\fP(2) and
\fIsetrlimit\fP(2) will automatically be converted to RLIM_INFINITY since
the conversion of seconds to ticks would cause overflow.
.XP kern_xxx.c
miscellaneous system facilities
.XP syscalls.c
list of available system calls
.NH 3
Disk quotas
.XP quota_sys.c
disk quota system call routines.
.XP quota_kern.c
in-core data structures for the in-core data structures.
.XP quota_subr.c
miscellaneous support routines for quota system.
.XP quota_ufs.c
file system routines for quota system.
.NH 3
General subroutines
.XP subr_prf.c
\fIprintf\fP and friends; also, code related to
handling of the diagnostic message buffer
.XP subr_rmap.c
subroutines which manage resource maps.  The kernel routines \fImalloc\fP(),
\fImfree\fP(), and \fImalloc3\fP() have been redone to various degrees for
speed.
.XP subr_xxx.c
miscellaneous routines, including the \fInonet\fP(2) routine.  This
routine has been supplied as a stub for the routines \fIsocket\fP(2)
and \fIsocketpair\fP(2) on non-networking systems so that networking
programs can behave rationally.
.NH 3
System level support
.XP sys_generic.c
code for the upper-most levels of the ``generic'' system calls: \fIread\fP,
\fIwrite\fP, \fIioctl\fP, and \fIselect\fP; a ``must read'' file for the
system guru trying to shake out 2.9BSD bad habits.
.XP sys_inode.c
code supporting the ``generic'' system calls of sys_generic.c as they
apply to inodes; the guts of the byte stream file i/o interface.  Inode
locking, (the \fIflock\fP(2) call) is also implemented here.
.XP sys_kern.c
kernel stubs to allow the network kernel to access components of
structures that exist in kernel data space.
.XP sys_net.c
copies of kernel routines required by the networking kernel as well
as some network initialization routines.
.XP sys_process.c
code related to process tracing
.XP sys_socket.c
code supporting the ``generic'' system calls of sys_generic.c
as they apply to sockets
.NH 3
Terminal handling
.XP tty.c
the terminal handler proper; both 2.9BSD and version 7 terminal
interfaces have been merged into a single set of routines which
are selected as line disciplines
.XP tty_conf.c
initialized data structures related to terminal handling;
.XP tty_pty.c
support for pseudo-terminals; actually two device drivers in
one
.XP tty_subr.c
c-list support routines; these have been completely rewritten to be
much faster, particularly in moving groups of characters around.  The
definition \fICBSIZE\fP has also been doubled, to 32, while the
number of clists (\fINCLISTS\fP) has been cut by about a third.  The
number of clists is also relative to the constant \fIMAXUSERS\fP, rather
than being the same for all systems.
.XP tty_tb.c
two line disciplines for supporting RS232 interfaces to Genisco and
Hitachi tablets.  Untested in 2.10BSD.
.XP tty_tty.c
trivial support routines for ``/dev/tty''
.NH 3
File system support
.XP ufs_alloc.c
code which handles allocation and deallocation of file system related
resources: disk blocks, on-disk inodes, etc.  2.9BSD had a real problem
here in that it did a linear search of the mount table every time it
freed or allocated a block.  The addition of a back-pointer to the
associated file system to the incore inode eliminated this lookup.
.XP ufs_bio.c
block i/o support, the buffer cache proper.  The 2.9BSD block i/o code
has been completely ripped out and replaced with 4.3BSD code.  The
most visible effects of this were the removal of the \fIb_link\fP field
from the buffer header structure, the removal of the \fIbunhash\fP()
subroutine, and the reorganizing of the free queues into four separate
queus.  This will have some interesting side effects if you have local
drivers that use the \fIb_link\fP field, or if you have drivers that
misuse some of the remaining pointers in the buffer header structure.
Note, the correct fix is \fBnot\fP to put the \fIb_link\fP field back
in, but to rewrite the driver's queueing mechanism.  All current drivers
have also been rewritten to supply the \fIBYTE\fP or \fIWORD\fP argument
directly to \fIphysio\fP(), eliminating the routine \fIphysio1\fP().
.XP ufs_bmap.c
code which handles logical file system to logical disk block 
number mapping; understands structure of indirect blocks and files
with holes; handles automatic extension of files on write
.XP ufs_dsort.c
sort routine implementing a prioritized seek sort algorithm
for disk i/o operations
.XP ufs_fio.c
code handling file system specific issues of access
control and protection
.XP ufs_inode.c
inode management routines; in-core inodes are now hashed and retained
in an LRU cache, so that inodes do not have to be continually re-read
from disk.  The in-core inode also contains the access, modification,
and creation times which make \fIstat\fP(2) calls much faster.  There is
an interesting relationship between texts and inodes, now, in that
the number of inodes available for normal system use will be the number
of inodes minus the number of text images cached by the system.  Also, the
inode now has a pointer to its text object, if any, which is used to implement
shared text images.
.XP ufs_mount.c
code related to demountable file systems
.XP ufs_nami.c
the \fInamei\fP routine (and related support routines) \- the routine that
maps pathnames to inode numbers.  The 2.9BSD \fInamei\fP() routine has been
completely reworked.  There are three major advantages to the new one:
first, it reads the entire path in from user space at once rather than
using multiple subroutine calls per character, second, it caches the
current position in the directory so that each subsequent request from a
process will continue searching at the place it stopped, and finally,
it handles symbolic links correctly.  \fINamei\fP,
and its associated routines, probably gave us more long-term trouble than
any other part of the kernel.  Be Very Careful if you modify anything here,
especially in terms of side-effects for \fImkdir\fP(2), \fIrmdir\fP(2),
and \fIrename\fP(2).
.XP ufs_subr.c
miscellaneous subroutines
.XP ufs_syscalls.c
file system related system calls, everything from \fIopen\fP
to \fIunlink\fP; many new system calls are found here:
\fIrename\fP, \fImkdir\fP, \fIrmdir\fP, \fItruncate\fP, etc.
.NH 3
Interprocess communication
.XP proto.c
protocol configuration table and associated routines
.XP sys_pipe.c
the pipe open, read, and write routines
.XP uipc_domain.c
code implementing the ``communication domain'' concept;
this file must be augmented to incorporate new domains
.XP uipc_mbuf.c
memory management routines for the ipc and network facilities;
refer to the document ``4.2BSD Networking Implementation Notes''
for a detailed description of the routines in this file
.XP mbuf11.c
the same as above, only more PDP-11 dependent.
.XP uipc_proto.c
.UX
ipc communication domain configuration definitions; contains
.UX
domain data structure initialization
.XP uipc_socket.c
top level socket support routines; these routines handle the
interface to the protocol request routines, move data between
user address space and socket data queues and understand the
majority of the logic in process synchronization as it relates
to the ipc facilities
.XP uipc_socket2.c
lower level socket support routines; provide nitty gritty bit
twiddling of socket data structures; manage placement of data on
socket data queues
.XP uipc_syscalls.c
user interface code to ipc system calls: \fIsocket\fP, \fIbind\fP,
\fIconnect\fP, \fIaccept\fP, etc.; concerned exclusively
with system call argument passing and validation
.XP uipc_usrreq.c
.UX
ipc domain support; user request routine and supporting
utility routines
.NH 3
Memory support
.PP
The code in the memory subsystem has changed very little
from 2.9BSD; most changes made in these files were either to
gain speed or to fix bugs.
.XP vm_proc.c
mainly code to manage memory allocation during
process creation and destruction.
.XP vm_sched.c
the code for process 0, the scheduler, lives here; other routines which
monitor and meter memory activity (used in implementing high level
scheduling policies) also are present; the scheduler has been perturbed
in minor ways to encourage the swapping of processes that have been in
a sleep state for a significant amount of time.
.XP vm_swap.c
code to swap a process in or out of core
.XP vm_swp.c
code to handle swap i/o
.XP vm_text.c
code to handle shared text segments \- the ``text'' table; this code has
been completely redone to allow the cacheing of text images.  The current
system will maintain the core image of a process until the core or swap
is required for another process.  Neither will a swap image of a process
be written until the image is forced out of core.  Basically, all programs
are now treated as ``sticky''.  A new routine, \fIxuncore\fP(), has been
added that frees up core space for the map allocation routines.
.NH 2
/sys/conf
.PP
This directory contains files used in configuring systems.
The format of configuration files has changed a great deal.
.XP :comm-to-bss
script to move the \fIproc\fP, \fIinode\fP and \fIfile\fP tables
from comm space to bss.
.XP :splfix.*
scripts to implement the spl functions for various
PDP-11 instruction sets.
.XP GENERIC
the generic configuration file; contains the standard
user tuneable system parameters.
.XP Make*
the system Makefiles; they've been broken up into six parts since
a single one cannot handle the dependencies generated for the kernel.
.XP VAX.compile
a directory containing a C preprocessor and C compiler that allows 2.10BSD
to be compiled on another machine.  If you're interested, the entire
networking kernel compiles from scratch in about 5 minutes on a VAX 8600.
.XP checksys.c
source for the kernel size checking program
.XP config
the actual kernel configuration script
.XP ioconf.c
I/O configuration file
.XP param.c
various kernel parameters and tables
.NH 2
/sys/pdpuba
.PP
This directory contains UNIBUS device drivers, and their related include
and autoconfiguration files.  The include files have moved from \fI/sys/h\fP
and the autoconfiguration files from \fIsys/autoconfig\fP in an effort to
isolate machine-dependent portions of the system.  The following device
drivers were not present in the 2.9BSD release.
.XP ram.c
a ram disk driver
.XP rx.c
a driver for the RX01/02 floppy disk
.XP si.c
a driver for the SI 9500 controller with CDC 9766 disks
.XP ra.c
a driver for the UDA controller
.PP
In addition to the above device drivers, many drivers present
in 2.10BSD now sport corresponding include files which contain
device register definitions.  For example, the DH11 driver
is now broken into three files: dh.c, dhreg.h, and dmreg.h.
.PP
The following device drivers have been significantly modified, or
had bugs fixed in them, since the 2.9BSD release:
.XP dh.c
the 4.3BSD driver has been installed
.XP dz.c
the 4.3BSD driver has been installed
.XP dhu.c
the 4.3BSD driver has been installed
.XP lpr.c
the line printer driver has had some per-character i/o removed
.XP xp.c
the bad blocking has been fixed
.PP
Additionally, the \fIrl\fP, \fIra\fP and \fIxp\fP drivers support the
22-bit QBUS.  The tty drivers also do intelligent auto siloing, switching
between siloing and interrupt per character based on input load.
.PP
In addition the \fIsys/OTHERS\fP directory has had several ``new'' device
drivers added to it that may or may not work.  It is extremely probable
that they do not handle the new ioctl protocols and it is also likely that
they do not correctly map buffers in and out of kernel space correctly.
For more information regarding the installation of device drivers,
see the file \fIsys/OTHERS/README\fP.  This is a rambling, disjointed
``must'' for the driver hacker.  You should also see this directory if you
are having problems with a driver that's currently in place in 2.10BSD.
There are several different versions of drivers, bug fixes etc. that we
just didn't have time to install and/or test out.  A great deal of work
has been done on the \fIra\fP and \fIxp\fP drivers.  They are known to work,
and work reliably.  You should use them, if at all possible.
.NH 2
/sys/pdpmba
.NH 2
/sys/pdp
.PP
The following files are new in 2.10BSD:
.XP clock.c
the clock start and busy-delay routines
.XP cons.c
the console driver, originally \fIsys/dev/kl.c\fP
.XP frame.h
the PDP-11 calling frame definition
.XP genassym.c
a program to generate structure offset definitions for the
assembly files, originally \fIsys/conf/genassym.c\fP
.XP in_cksum.c
checksum routine for the DARPA Internet protocols
.XP kern_pdp.c
PDP-11 dependent system calls
.XP machparam.h
machine-dependent portion of \fIsys/h/param.h\fP
.XP mem.c
the memory device driver
.XP mscp.h
definitions for the Mass Storage Control Protocol
.XP psl.h
definitions for the PDP-11 program status register
.XP reg.h
definitions for locating the users' registers, relative to R0
in the user call frame on the kernel stack
.XP scb.s
system control block file, containing the low core
vector interrupt definitions, originally \fIconf/l.s\fP.
.XP vmparam.h
machine-dependent memory portion of \fIsys/h/param.h\fP
.PP
The file \fImch.s\fP has been split up into several different
files.  The following files all contain code originally part
of \fImch.s\fP.  The \fIlibc_\fP files all contain code duplicated
in the C library.
.XP libc_bcmp.s
the bcmp routine
.XP libc_bcopy.s
the bcopy routine
.XP libc_bzero.s
the bzero routine
.XP libc_csv.s
the csave and creturn routines
.XP libc_ffs.s
the find first set routine
.XP libc_htonl.s
the host to network long routine
.XP libc_htons.s
the host to network short routine
.XP libc_insque.s
the insert queue routine
.XP libc_ldiv.s
the long division routine
.XP libc_lmul.s
the long multiplication routine
.XP libc_lrem.s
the long remainder routine
.XP libc_remque.s
the remove queue routine
.XP libc_strlen.s
the string length routine
.XP mch_backup.s
routines to back up the user CPU state after an aborted instruction
.XP mch_click.s
routines to move click size areas
.XP mch_copy.s
routines to move various data sizes to and from user space
.XP mch_dump.s
the automatic tape dump routines
.XP mch_dzpdma.s
the DZ-11 pseudo-DMA interrupt routines
.XP mch_fpsim.s
floating point simulation routines
.XP mch_profile.s
system profiling routines; these are untested since 2.9BSD.
.XP mch_start.s
checkout, setup and startup routines
.XP mch_trap.s
interrupt interface code to C.
.XP mch_vars.s
variable definition code
.XP mch_xxx.s
various other routines that needed to be in assembly
.XP DEFS.h
definitions and common macros for all assembly files; emulates the C
library DEFS.h file for the benefit of the \fIlibc_\fP files.
.PP
The supervisory networking support files are as follows:
.XP mch_KScall.s
kernel to supervisor call support
.XP net_SKcall.s
supervisor to kernel call support
.XP net_copy.s
version of mch_copy.s for the supervisor space
.XP net_csv.s
version of mch_csv.s for the supervisor space
.XP net_mac.h
\fI#defines\fP to convert standard kernel calls into calls into supervisor
space
.XP net_mbuf.s
routines to copy mbuf's in and out
.XP net_scb.s
entry points for network interrupt vectors
.XP net_trap.s
trap routine for supervisor space networking
.XP net_xxx.s
various other networking routines that needed to be in assembly
.NH 2
/sys/autoconfig
.PP
The autoconfiguration directory is mostly as it was in 2.9BSD with the
exception that the probe routines for the various devices have been
broken out into either \fIsys/pdpuba\fP or one of the networking directories.
To find a probe routine, look for a file named \fBxx\fIauto\fR(), where
\fBxx\fP is the standard device abbreviation.

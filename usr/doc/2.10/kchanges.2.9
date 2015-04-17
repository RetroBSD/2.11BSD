.if \nv .rm CM
.TL
Changes in the Kernel in 2.9BSD
.AU
Michael J. Karels
.AI
Department of Molecular Biology
University of California, Berkeley
Berkeley, California  94720
.AU
Carl F. Smith
.AI
Department of Mathematics
University of California, Berkeley
Berkeley, California  94720
.AU
William F. Jolitz
.AI
Symmetric Computer Systems
Los Gatos, California
.PP
This document summarizes changes in the PDP-11\(dg UNIX\(dd kernel between
.FS
\u\(dg\d\s-2DEC\s0, \s-2PDP-11\s0, \s-2MASSBUS\s0, and \s-2UNIBUS\s0
are trademarks of Digital Equipment Corporation.
.br
\u\(dd\d\s-2UNIX\s0 is a trademark of Bell Laboratories.
.FE
the July 1981 \s-12.8BSD\s0 release and the July 1983 \s-12.9BSD\s0
distribution.  The kernel remains highly tunable, and
changing \fI#define\fP\^d options may affect the validity of remarks
in this paper.
.PP
The major changes fall into these categories:
.IP [1]
The new signal mechanism needed for process control
has been added to the system,
making the job control facilities of \s-14.1BSD\s0 available.
.IP [2]
\fIVfork\fP, a form of \fIfork\fP which spawns a new process without fully
copying the address space of the parent, is available to
create a new context for an \fIexec\fP much more efficiently.
.IP [3]
The system can reboot itself automatically,
after crashes or manually.  The system is more crash-resistant 
and is able to take crash dumps before rebooting.
.IP [4]
A fast and reliable method of accessing mapped buffers and clists
without increasing processor priority
is now available.
.IP [5]
The protocols for allocation of the UNIBUS map have been changed,
and DMA into system buffers with 18-bit addressing devices is also different.
.IP [6]
Changes have been made in code organization, so that more
than one system configuration may be built from a single set of sources.
Each system is described by a single file that includes parameters such
as system size, devices, etc.
Most of the ``magic numbers'' such as device register addresses and
disk partitions are in one file, ioconf.c, and the number of devices
of each type are in header files local to that system.
.IP [7]
Most devices are configured at boot time rather than at
compilation time, reducing the work in system configuration
and making it possible for one binary to work on several similar systems.
References to nonexistent devices are now rejected rather than causing
a crash.
.IP [8]
System diagnostics have been changed to a standard, readable format;
file system diagnostics refer to file systems by name rather than
device number.  Device diagnostics refer to devices by name and print
error messages mnemonically as well as in octal.
.PP
Many other performance enhancements and bug fixes have been made.
Some conditional compilation flags have been removed because the
feature they control is now considered standard (e.g. UCB_BUFOUT).
Other features have been grouped together and
are now controlled by the same flag
(e.g. code previously conditional on UCB_SMINO now depends on UCB_NKB).
.PP
Many of the changes in 2.9BSD are based on work by many other people.
Several features are modeled on those of the 4.1BSD VMUNIX system,
and much of the code comes directly from that source.
.SH
Converting local software
.PP
Most local changes should be easily ported to the new
system.
The actual system configuration is much simpler than with previous kernels.
.PP
There are many changes that affect the device drivers.
The appendices give the details of the conversions necessary.
Device drivers that used the kernel's in-address-space buffers
must be rewritten to use mapped buffers or their own dedicated buffers.
``Abuffers'' have been removed from the current system.
.PP
Appendix A contains a description of the new data mapping
protocols used to access mapped buffers, clists, and some
tables.
.PP
The UNIBUS map is allocated dynamically.  Kernel data space
is no longer guaranteed to be mapped by any portion of the UNIBUS
map.  Any local software making such assumptions must now
explicitly allocate a section of the UNIBUS map;
\fImapalloc\fP and \fImapfree\fP may be used for objects with buffer
headers.
See Appendix B for a description of the new UNIBUS map protocols.
.PP
The line discipline switch has been reorganized slightly to
make it cleaner.  Some unused
fields in the \fIlinesw\fP structure have been removed.  There is
a default line discipline, DFLT_LDISC, which may not be assumed
to be 0.  See Appendix C for a description of the new terminal
and line discipline protocols.
.PP
As part of the implementation of \fIvfork\fP, process images are
scatter loaded.  Standard system monitoring
programs (e.g. \fIps\fP and \fIw\fP) have been modified.
Local software must be changed accordingly.
See Appendix D for a more detailed description of \fIvfork\fP.
.PP
Sites may wish to convert their device drivers to use the new
autoconfiguration features described in Appendix E.
.PP
Processors are described by capabilities rather than cpu type.
Separate I/D spaces and UNIBUS maps are detected and supported independently.
Thus it is much easier to describe machines with foreign
hardware enhancements.
In particular, the Able ENABLE/34\(dg is automatically
.FS
\u\(dg\d\s-2ENABLE/34\s0 is a trademark of \s-2ABLE\s0 Computer, Inc.
.FE
detected and supported.
.PP
A new bootstrap loader that loads all object files
except 0405 replaces the old version that loaded only 0407,
0411, and 0430.
The kernel assumes that \fIboot\fP has already set the kernel mode
segmentation registers and cleared bss.
Other bootstraps that do not do so will not work.
.SH
Organizational changes
.PP
The system compilation procedure has been changed so that more than one
set of binaries may be made with a single set of source code.  System
sources are kept in the directories \fBsys/sys\fP and \fBsys/dev\fP.  No
binaries are kept in either of these directories.
.PP
The directory \fBsys/conf\fP contains several files related to system
configuration.  For each machine to be configured, a single file should
be created in this directory.  Each such file describes all the parameters
of the machine necessary for building a system.  The format of the
configuration files is described in \fIconfig\fP\|(8)\(dd.
.FS
\u\(dd\dReferences of the form \fIX\fP(\fIY\fP) mean the subsection
named \fIX\fP in section \fIY\fP of the Berkeley \s-2PDP-11\s0
\s-2UNIX\s0 Programmer's manual.
.FE
This procedure is more fully described in
``Installing and Operating 2.9BSD.''
.PP
Corresponding to each system to be configured, there is a subdirectory
of \fBsys\fP.  One prototype directory, \fBGENERIC\fP,
is already there.  This directory is created and the appropriate files
are installed by \fIconfig\fP, based on information in the machine
description file.
The configuration program processes
the information in the configuration file and produces:

.IP 1)
A set of header files (e.g. \fBdh.h\fP) which contain the number of
devices available to the target system.  These definitions force conditional
compilation of drivers, resulting in the inclusion or exclusion of
driver code and the sizing of driver tables.  This technique, based
on compilation, is more powerful than a loader-based technique, since
small sections of code may be easily conditionalized.
Only drivers that
are needed are included in the resulting system.
Option flags that are specific to individual drivers are also placed in
these header files.
.IP 2)
The assembly language vector interface, \fBl.s\fP,
which turns the hardware generated UNIBUS interrupt sequences
into C calls to the driver interrupt routines.
.IP 3)
A table file, \fBioconf.c\fP, which defines controller addresses for each
disk controller in the configured system, and the partition tables
for the larger disks.
.IP 4)
The files \fBlocalopts.h\fP, \fBparam.c\fP, \fBparam.h\fP, and
\fBwhoami.h\fP.
These can be edited if local taste so dictates.
\fBWhoami.h\fP contains the definition of PDP11, which will have one
of the following values: 23, 34, 40, 44, 45, 60, 70, or GENERIC.
The distributed binary is compiled with PDP11=GENERIC, allowing
the system to support most of the hardware on any supported processor.
The definitions for the optional features of the system
are in \fBlocalopts.h\fP.
Finally, the files \fBparam.c\fP and \fBparam.h\fP contain the tunable sizes
and parameters.  These are mostly dependent on the definitions of PDP11
and MAXUSERS (in the Makefile).
\fBParam.c\fP contains most of the commonly-changed parameters, so that only
this file need be recompiled to retune the system.
Also, because these parameters are now in global variables,
system utilities may easily determine the current values by examining
the running system.
.IP 5)
The Makefile contains the default compilation and load rules
for the type of kernel being made (overlaid or not overlaid).
It also contains the specification of an editor script that implements
in-line expansions of calls to spl, depending on the instruction
set available.
The makefile may need editing to change the overlay structure or
to include optional device drivers in the load rules.
MAXUSERS is defined here and used in \fBparam.c\fP 
to gauge the sizes of data structures.
.PP
In order to add new files or device drivers to the system, it is necessary
to explicitly add them to the Makefile load rules, to its extension Depend
(used in the ``make depend'' command to rebuild the Makefile dependency rules),
to the configuration file \fBc.c\fP and optionally to \fIautoconfig\fP\|(8)
and \fIconfig\fP\|(8) or \fBl.s\fP.
.SH
Header files
.PP
Many new files have been added for use in device drivers.  They contain
definitions of the device structure and mnemonics used in referencing
registers and printing diagnostics.
Most files have been reorganized slightly to improve modularity or
readability.
.IP \fBacct.h\fP 1.5i
The UCB_XACC option has been separated into UCB_LOGIN and UCB_SUBM.
.IP \fBbuf.h\fP
Unused flags have been deleted and the others compacted.
Two flags have been added.  B_RH70 indicates that a device is
on an RH70 controller.  B_UBAREMAP indicates that the buffer's address
is being interpreted as UNIBUS virtual, not physical.
.IP \fBconf.h\fP
A \fId_root\fP field has been added to the \fIbdevsw\fP structure.
The unused fields \fIl_rend\fP and \fIl_meta\fP have been deleted from
the \fIlinesw\fP structure.  \fIL_rint\fP has been renamed \fIl_input\fP.
\fIL_start\fP has been deleted and a new field, \fIl_output\fP added
for \fIuprintf\fP.  See Appendix C.
.IP \fBcpu.h\fP
New file.  Contains mnemonics for fields in the cache and memory
control registers of various processors.
.IP \fBdkbad.h\fP
New file.  Contains mnemonics and structures used to implement DEC
standard 144 bad sector forwarding.
.IP \fBfilsys.h\fP
Two fields in the \fIfilsys\fP structure, \fIs_fname\fP and \fIs_fpack\fP,
have been replaced by \fIs_fsmnt\fP.  The new field is used by
the kernel to print diagnostics and by \fIfsck\fP(8).
.IP \fBinline.h\fP
New file.  Definitions of inline expansions and macro replacements designed
to speed up  file system accesses at the cost of code expansion.
.IP \fBlstat.h\fP
Renamed \fIqstat.h\fP.  The structure previously names \fIlstat\fP is
now names \fIqstat\fP and all structure fields previously named
\fIls\_*\fP have been renamed \fIqs\_*\fP.
.IP \fBkoverlay.h\fP
New file.  Contains definitions relating to kernel text overlays.
Both nonseparate I/D (0430) and separate I/D (0431) kernels can be overlaid.
Most of the information in this file cannot be changed easily.  It is
provided to clarify the way kernel overlays work.
.IP \fBmtio.h\fP
An \fImt_type\fP field has been added to the \fImtget\fP structure.
Tape drivers may be interrogated to determine formatter type.
See \fImt\fP\|(4).
.IP \fBparam.h\fP
Many configuration constants (e.g. NINODE, NPROC) have moved from
here to param.c and are referenced by global variables rather than
manifest constants.  Thus only one file need be recompiled to change
them.
.IP \fBproc.h\fP
Numerous changes have been made to support job control and \fIvfork\fP\|s.
The \fIxproc\fP structure is in a union in the \fIproc\fP structure so
that it is easily possible to determine which fields are overlaid.
.IP \fBqstat.h\fP
Used to be called \fIlstat.h\fP.  Contains declarations for the \fIqstat\fP
and \fIqfstat\fP system calls (for quotas).
.IP \fBreboot.h\fP
New file.  Contains options for the \fIreboot\fP system call.
.IP \fBreg.h\fP
The (unused) definition of ROV has been deleted.
.IP \fBseg.h\fP
New macros and definitions have been added to support the remapping
of kernel data to access buffers and clists.
Changes have been made to allow dynamic support of the ENABLE/34.
.IP \fBtrap.h\fP
New file.  Used in l.s, mch.s, and trap.c to encode trap types mnemonically.
.IP \fBtty.h\fP
Contains a macro for \fIlookc\fP if UCB_NTTY is defined and UCB_CLIST
is not defined.
.IP \fBtypes.h\fP
More typedefs have been added.
.IP \fBuba.h\fP
New file.  Most UNIBUS map specific structures and macros are
collected here.
.IP \fBuser.h\fP
Numerous changes have been made to support job control and \fIvfork\fP\^s.
.IP \fBvcmd.h\fP
New file.  Contains commands used by the vp driver and user \fIioctl\fP
definitions.
.SH
System files:  sys/sys
.PP
Major changes have taken place to support job control and \fIvfork\fP\^s.
The \fIfile\fP, \fIproc\fP, and \fItext\fP tables have been moved to
the end of kernel data space (possibly in the region
into which buffers and clists
are mapped) and thus are not necessarily accessible at interrupt
time; those functions that need to access these tables or the \fIu\fP.\&
from interrupt level (currently \fIclock\fP, \fIgsignal\fP, and
\fIwakeup\fP) must save and restore kernel mapping registers.
.PP
Inclusion of both the multiplexer and floating point support is
conditional, reducing the size of systems that do not require them.
Some consistency checks that we consider extremely unlikely
to fail, and the accompanying \fIpanic\fP\^s, are uniformly
conditional on the definition of DIAGNOSTIC.
Calls to \fIsplN\fP (where \fIN\fP is 0, ..., 7) that do not require the
previous priority to be returned have been changed to \fI_splN\fP
and are expanded in-line by editing the compiler's output.
.IP \fBacct.c\fP 1.5i
The \fIsysphys\fP routine has been moved from here to machdep.c.
.IP \fBalloc.c\fP
File system error messages are identified by file system name rather than
major/minor device number.  They are printed directly on a user's terminal
if that user causes a file system to run out of free space.
\fIGetfs\fP no longer \fIpanic\fP\^s if it cannot find a device in the
mount table.  Callers of \fIgetfs\fP have been modified to check for a
NULL return value.  This, together with a change to pipe.c, avoids
a panic if \fIpipedev\fP is a file system that is not
currently mounted.
.IP \fBclock.c\fP
\fIClock\fP has been modified to use the new remapping protocols.
Disk monitoring has been simplified and can monitor more (or fewer) than
three disks.  Free memory averaging is calculated in kilobytes, avoiding
overflow.
.IP \fBenable34.c\fP
New file.  Contains support routines for the ENABLE/34.  Two routines,
\fIfiobyte\fP and \fIfioword\fP
are used to help solve the problem of probing the I/O page on machines
with ENABLE/34 boards.  Wherever \fIfuibyte\fP and \fIfuiword\fP would
be used to probe a location \fIpossibly\fP on the I/O page, these
routines should be used instead.
.IP \fBfakemx.c\fP
This file is no longer necessary and has been deleted.
.IP \fBfio.c\fP
\fIFalloc\fP uses the \fItablefull\fP routine.
A bug in the \fIaccess\fP system call with the UCB_GRPMAST option
has been fixed.
.IP \fBiget.c\fP
After reading blocks of inodes, both the error flag and the residual count
are checked.  This avoids destroying whole blocks of inodes on failure.
The residual count is also checked in other places in the kernel
(\fIbmap\fP, etc.).
If an error occurs in \fIiget\fP,
\fIiput\fP is not called for an invalid inode.
\fIIget\fP uses the \fItablefull\fP routine.
.IP \fBl.s\fP
Both l.s and the old l40.s are merged into this file.  The code is
preprocessed with \fIcpp\fP, allowing
consistency with C files for conditional compilation.
.IP \fBmachdep.c\fP
A \fIboot\fP function has been added to cause the system to reboot
itself and (optionally) take a crash dump automatically.
The type of reboot is passed to /etc/init as an argument.
\fIMapalloc\fP and \fImapfree\fP use a resource map to dynamically
allocate sections of the UNIBUS map.
\fIMapalloc\fP translates physical addresses in buffer headers for cache buffers
to UNIBUS addresses for transfers on UNIBUS devices.
\fIMapalloc\fP is thus called for both buffered and raw transfers now.
\fIUbinit\fP initializes the UNIBUS map and the resource map describing
it.
\fIMapin\fP and \fImapout\fP no longer run at
elevated priorities to block interrupts.
\fIMapout\fP is eliminated if the kernel data segment is sufficiently small.
.IP
A new function, \fIdorti\fP, which is used by the new signal mechanisms
has been added.
.IP
Buffer space is uniformly \fImalloc\fP\^ed in \fIstartup\fP rather than
in \fIstart\fP (mch.s)
The same is true for clists if UCB_CLIST is defined.
.IP
On machines without UNIBUS maps, no attempt is made to detect memory past
0760000, avoiding crashes when device registers are found at this address.
.IP
\fIClkstart\fP calls \fIfioword\fP to probe for the line clock register.
It is not a panic if no clock register is found since 11/23s may not
have one; a message is printed in this case.
.IP \fBmain.c\fP
The name of the root file system (``/'') is copied into its superblock so that
the name will be available for error messages
(e.g. if the root file system becomes full).
.IP \fBmalloc.c\fP
All addresses and sizes in \fBmalloc.c\fP have been typedeffed
and are unsigned.
This makes it possible to use more than two megabytes of memory.
A new function, \fImalloc3\fP, efficiently allocates
memory for scatter loading, minimizing the cost of failing.
\fIMfree\fP contains many more consistency checks.
Resource maps have a new structure that includes a limit.
\fIMfree\fP prints a console error message when it must discard a piece of a map
because of fragmentation instead of overrunning the map or \fIpanic\fP\^ing.
When \fImalloc\fP cannot allocate enough
swap space, it frees the swap space belonging to saved text segments,
possibly avoiding panics caused by running out of swap space.
.IP \fBmch.s\fP
Both m40.s and the old mch.s have been merged into this file.
The C preprocessor is used to produce the right code for different
CPUs, including GENERIC.
It is able to reboot after power failures if the contents of memory
are intact.
.IP
\fICopyseg\fP and \fIclearseg\fP have been converted to \fIcopy\fP and
\fIclear\fP respectively. They take an
additional argument, a count of the number of clicks to copy or clear.
They remap the kernel to access the source and target more efficiently.
If real-time support is enabled, both are preemptible.
A new routine, \fIcopyu\fP, is available to copy the \fIu\fP.\&
in non-preemptible mode.
.IP
Most \fIspl\fP calls are now done in-line;  the old priorities are saved
and restored as bytes (to allow the use of \fImfps\fP/\fImtps\fP instructions
where available).  Kernel red stack violations are detected, allowing normal
\fIpanic\fP\^s.
.IP
System call traps are handled separately from other processor traps.
This results in a 22% decrease in system call overhead.
Emulator traps (used in automatic text overlays) are also handled
separately from general traps.  This decreases overlay switch overhead
by 45%.
On machines without hardware floating point, a fast illegal instruction trap
routine reduces system overhead for interpreted floating point by 90%.
.IP
The kernel overlay support has been changed to use new, smaller subroutine
entries (``thunks'') in the base segment that are compatible with
the loader used for user-level overlaid programs.  The management of
the kernel stack in the trap/interrupt code is simpler and faster.
.IP
The kernel text relocation that was done in mch.s if
UCB_CLIST or UCB_BUFOUT were defined is no longer necessary
and has been replaced by calls to \fImalloc\fP in \fIstartup\fP.
.IP \fBnami.c\fP
File names are not allowed to contain characters with the parity bit (0200) set.
File name comparisons stop at the first null.
A bug that caused permissions to be checked incorrectly when searching to ``..''
from the root of a mounted filesystem has been fixed.
The ``.. / u.u_rdir'' security hole has been fixed.
.IP \fBpipe.c\fP
Allocates inodes for pipes on the root device if \fIialloc\fP\^s on
\fIpipedev\fP fail.
Inodes for pipes are marked for special handling.
.IP \fBprf.c\fP
\fIPanic\fP causes the system to reboot.  A function, \fIuprintf\fP,
has been added to print error messages on the terminal of the user causing
the error rather than the console.  \fIPrintf\fP no longer uses recursion.
It supports a %c format to
print a single character, a %b format used to print register values
mnemonically, and a %X format for long hexadecimal.
\fIPrdev\fP has been eliminated.  \fIDeverror\fP is included only if
UCB_DEVERR is undefined.
.IP
The routines \fIprdev\fP and \fIdeverror\fP, that printed diagnostics
that were difficult to interpret, are replaced by 
\fIharderr\fP, that begins
a message about an unrecoverable device error, and the %b format
mentioned above.
\fITablefull\fP is a new function used to report that a table is full.
.IP \fBprim.c\fP
Uses new mapping protocols for \fICMAPIN\fP and \fICMAPOUT\fP.  \fIGetw\fP
has been discarded.  \fIPutw\fP is included only if needed for the
multiplexer driver.
\fICpaddr\fP has been deleted.  It is now a macro in dh.c.
Other routines that are used only by the dh driver are eliminated if there
are no dh's on a system.  \fILookc\fP is eliminated (replaced by a macro)
if UCB_CLIST is not defined.
.IP \fBrdwri.c\fP
Inodes allocated for pipes receive special handling:  \fIwritei\fP always
uses \fIbdwrite\fP and \fIreadi\fP cancels the disk write
if it has not yet occurred.  This results in a large improvement in
pipe throughput, especially if the UCB_FSFIX option is in use (for
more robust file systems).
.IP \fBsig.c\fP
This is now a dummy file that includes either sigjcl.c or signojcl.c
depending on whether MENLO_JCL is defined.
.IP \fBsigjcl.c\fP
A new file that supports the signal mechanisms necessary for job control.
The changes listed under \fIsignojcl.c\fP are also included.
.IP \fBsignojcl.c\fP
Used to be called sig.c.
A race condition that occasionally caused ignored signals to generate bus errors
has been fixed.
\fIPtrace\fP supports overlay changes, allowing breakpointing of overlaid
subprocesses.
If floating point arithmetic is being simulated by catching illegal instruction
traps,
traced subprocesses are allowed to process the signal normally without stopping.
Stack growth is rounded to 8K boundaries, to allow the maximum theoretical
stack size.
.IP \fBslp.c\fP
There are major changes in the \fIsleep\fP/\fIwakeup\fP mechanism for
process control.  Swapped processes are no longer kept on the run queue.
\fINewproc\fP has been modified to allow \fIvfork\fP\^s.
The scheduling algorithm has been modified to
avoid deadlocks possible
with \fIvfork\fP.  Processes are scatter loaded in three pieces
(data, stack and \fIu\fP. area; text is handled separately),
with changes in \fInewproc\fP, \fIexpand\fP and \fIswapin\fP.
.IP
The unused routine \fIdequeue\fP has been removed.
.IP \fBsubr.c\fP
\fIBcopy\fP may now be called with a count of 0.
.IP \fBsys1.c\fP
\fIFork\fP has been modified to allow \fIvfork\fP\^s and uses
the \fItablefull\fP routine.
Support has been added for \fIwait2\fP, used in job control.
\fIBdwrite\fP is used instead of \fIbawrite\fP
when copying out argument lists
in \fIexece\fP, in an attempt to avoid disk I/O.
A pointer to the last used proc table slot, \fIlastproc\fP, is used to
shorten searches for processes.  A message is printed if /etc/init
cannot be executed.
.IP \fBsys3.c\fP
\fISmount\fP copies the mounted file system's name (e.g. ``/usr'')
into the s_fsmnt field of the superblock.
The in-address-space buffers (abuffers) have been removed,
and the superblocks of mounted file systems are in the mount table itself.
.IP \fBsys4.c\fP
The mechanism for sending signals to all processes
has been changed so that the
process broadcasting the signal does not receive it itself.
This allows \fIreboot\fP\|(8) to shut down the system cleanly
before rebooting.
.IP
The \fI#ifdef\fP for UCB_STICKYDIR has been removed.  This is now standard.
\fISetpgrp\fP is included to support job control.
A bug in \fIutime\fP has been fixed.
.IP \fBsyslocal.c\fP
The old \fIsetpgrp\fP is replaced by the job control version.
\fIChfile\fP and \fIiwait\fP have been
removed.  A new system call, \fIvhangup\fP, is used by \fIinit\fP to
revoke access to terminals after logouts.
Another new system call, \fIucall\fP, allows \fIautoconfig\fP\|(8) to
call internal kernel routines.
Support for \fIqstat\fP, and \fIqfstat\fP (formerly \fIlstat\fP and
\fIlfstat\fP respectively)
is conditional on UCB_QUOTAS.
.IP \fBtext.c\fP
\fIXswap\fP has been modified for scatter loading.
\fIXumount\fP frees all saved text segments if called with
argument NODEV.  \fIMalloc\fP uses this to attempt to
avoid \fIpanic\fP\^s when swap space is exhausted.
\fIXalloc\fP uses the \fItablefull\fP routine.
.IP \fBtrap.c\fP
\fITrap\fP no longer handles system calls.  Instead, a new routine,
\fIsyscall\fP, is called from mch.s when a system call trap occurs.
\fITrap\fP saves the previous kernel mapping on kernel faults.
.IP \fBureg.c\fP
A new routine, \fIchoverlay\fP, has been added to change overlays
for user processes.  It is called
from mch.s when an overlay switch trap occurs.
The units of the variables describing the overlay region (ovbase and dbase)
have changed.
Segmentation register prototypes are no longer maintained for the overlay
region, necessitating a call to \fIchoverlay\fP from \fIsureg\fP.
\fIEstabur\fP and \fIsureg\fP support scatter loading.
A bug has been fixed that
caused overlaid processes to fail when the base
segment length was a multiple of 8192.
On machines without separate I/D space, \fIestabur\fP is simplified.
.SH
Device support:  sys/dev
.PP
All of the drivers have been modified to support autoconfiguration.
They have attach routines to record the csr addresses after
the device has been probed by \fIautoconfig\fP\|(8).
Appendix E describes the strategy.
Drivers with attach routines properly reject
attempts to access nonexistent controllers (instead of causing
a crash).
Each device driver has a corresponding header file indicating the
number of such devices present and other configuration dependent options.
.PP
Devices that do DMA on machines with UNIBUS maps must ensure that
their data areas are accessible through the UNIBUS map;
UNIBUS addresses are not necessarily the same as physical addresses.
see Appendix B.
Only buffers and clists are statically mapped.
It is possible to map in out-of-address space data at interrupt
level (this was previously risky) provided the previous map is saved
and restored; a mechanism is provided for this, as described in 
Appendix A.
The structure of the line switch has been reorganized
and the protocol to be used in opening a device and setting up a line
discipline is well defined.  See Appendix C.
.PP
Disks that are potentially \s-1RH70\s0 MASSBUS disks have been
provided with attach routines that detect \s-1RH70\s0s, as well
as root attach routines that force attachment before autoconfiguration
occurs.
Some disk drivers have been provided with crash dump routines.
See \fIrmdump\fP in rm.c or \fIhkdump\fP in hk.c for examples.
.PP
The format of device option flags is now consistent.
Optional device ioctls are enabled by XX_IOCTL (e.g. DH_IOCTL).
Optional watchdog timers are enabled by XX_TIMER (e.g. TM_TIMER).
The \fIdh\fP (respectively \fIdz\fP) driver, which is capable of
managing the input siilo to reduce interrupts,
does so if DH_SILO (respectively DZ_SILO) is defined.
The disk cache monitoring numbers used by \fIiostat\fP\|(8), formerly called
DK_N, have been renamed XX_DKN (e.g. HP_DKN) so that they can be placed in
the header files.
.PP
All drivers use include files to define the device structures
and register constants.  The drivers themselves uniformly use
mnemonics rather than magic numbers in device registers and error messages.
Initialized device register addresses and disk driver partition tables reside
in ioconf.c.
.IP \fBbio.c\fP 1.5i
\fIIodone\fP reverses the translation of buffer addresses
(done by \fImapalloc\fP) from
physical to UNIBUS virtual when doing block I/O on UNIBUS disks.
\fIBwrite\fP now correctly supports the B_AGE flag on asynchronous writes.
A portion of the disk monitoring code that was of questionable
usefulness has been discarded.
The \fIphysio\fP subroutine has been divided into separate routines,
allowing use of \fIbphysio\fP by drivers
that allow byte-oriented rather than word-oriented transfers
or don't use buffer headers.
.IP \fBbk.c\fP
The Berknet line discipline has been changed to use dedicated buffers
instead of abuffers.
It is still untested.
.IP \fBdh.c\fP
Changed to use the new UNIBUS map location of clists.  Ioctls for
setting and clearing \fIbreak\fP and \fIdtr\fP have been added.
If DH_SOFTCAR is defined, modem control is ignored for lines whose minor
device number is greater than or equal to 0200.
Dhdm.c is now part of dh.c; the appropriate dm support is included
only if needed.
.IP \fBdhdm.c\fP
This is now part of dh.c.
.IP \fBdhfdm.c\fP
This file is no longer necessary and has been deleted.
.IP \fBdvhp.c\fP
This driver is simplified if there is only one drive,
as no seek is needed before a transfer.
Error correction code has been added.
.IP \fBdz.c\fP
Optionally uses the dz silo.
Ioctls for setting and clearing \fIbreak\fP and \fIdtr\fP are available.
If DZ_SOFTCAR is defined, modem control is ignored for lines whose minor
device number is greater than or equal to 0200.
Pseudo-dma has been implemented.
.IP \fBhk.c\fP
New version of the RK06/7 driver.  Now performs disk sorts, ECC corrections,
and DEC standard 144 bad sector forwarding.  A dump routine has been added.
.IP \fBhp.c\fP
This driver is simplified if there is only one drive,
since no search is needed before a transfer.
Error correction code works with mapped buffers and 1024 byte blocks.
The driver waits for Drive Ready when doing positioning commands.
A dump routine has been added.
A preliminary, lightly tested version of DEC standard 144 bad sector
forwarding has been added.
.IP \fBht.c\fP
Tape ioctls are supported.
Uses \fIbphysio\fP for byte-oriented transfers.  \fIClrbuf\fP is no
longer called from interrupt level.
.IP \fBkl.c\fP
\fIPutchar\fP has been modified to support \fIuprintf\fP.
.IP \fBmem.c\fP
Some unneeded \fIspl\fP\^s have been deleted.  Routines used to read and
write memory set page protections correctly.
.IP \fBml.c\fP
New file.  A driver for the DEC ML11 solid state disk
courtesy of the DEC UNIX Engineering Group.
.IP \fBmux.c\fP
Dropped from this distribution.
.IP \fBrf.c\fP
New version of an old driver missing from \s-12.8BSD\s0.
.IP \fBrk.c\fP
Properly recovers the residual byte count at the end of a transfer.
.IP \fBrl.c\fP
Properly recovers the residual byte count at the end of a transfer.
.IP \fBrm.c\fP
This driver is simplified if there is only one drive;
the \fIrmustart\fP routine is merged with \fIrmstart\fP, and no search
is needed before a transfer.
Error correction code works with mapped buffers and 1024 byte blocks.
The software simulation of the current cylinder
register has been fixed.
The driver waits for Drive Ready when doing positioning commands.
A dump routine has been added.
A preliminary, lightly tested version of DEC standard 144 bad sector
forwarding has been added.
.IP \fBrp.c\fP
Properly recovers the residual byte count at the end of a transfer.
.IP \fBrx2.c\fP
New file.  A driver for the DEC RX211 floppy disk controller
courtesy of the DEC UNIX Engineering Group.
.IP \fBrx3.c\fP
New file.  A driver for the DSD480 floppy disk controller
courtesy of Tektronix.
.IP \fBtm.c\fP
Uses \fIbphysio\fP for byte-oriented transfers.  \fIClrbuf\fP is no
longer called from interrupt level.  Contains code for an optional
watchdog timer.  Checks for density changes in mid-tape.
.IP \fBts.c\fP
Tape ioctls are supported.
Uses \fIbphysio\fP for byte-oriented transfers.  \fIClrbuf\fP is no
longer called from interrupt level.
.IP \fBtty.c\fP
The \fIttioctl\fP subroutine calls the line discipline's ioctl
before any other processing.
\fITtioctl\fP has also been changed to eliminate code for the old
line discipline if it is not present, and when changing disciplines
it checks that the new discipline is supported.
These changes allow the old line discipline to be omitted.
It is possible to flush either the input or output queues (or both)
using TIOCFLUSH.
.IP \fBttynew.c\fP
Tandem mode is supported with raw mode in the new tty driver.
The t_char field is no longer disturbed by flow control in tandem mode.
Backslashes are no longer printed before capital letters on upper-case-only
terminals.
.IP \fBxp.c\fP
This driver (which supports an assortment of RP04/05/06, RM02/03/05,
Diva and other disks) now is able to manage more than one controller.
The probe routine is optional if the drive and controller structures
are initialized.
It is simplified if there is only one drive;
no search is needed before a transfer.
Error correction code works with mapped buffers and 1024 byte blocks.
The driver waits for Drive Ready when doing positioning commands.
A dump routine has been added.
A preliminary, lightly tested version of DEC standard 144 bad sector
forwarding has been added.
.br
.LP
.br
.bp
.ce
.I "Appendix A:  Kernel Data Mapping Protocols"
.sp 5
.NH
Introduction
.PP
These protocols ultimately address the question of how to
``expand'' the kernel's data space beyond the severe limitations
imposed by the \s-2PDP-11\s0 hardware.  This concern about
methods of expanding kernel data space stems from the desirability
of retaining large system buffer pools and clist areas despite
hardware limitations.  We do this by keeping certain
data objects resident in core but without guaranteeing that they will
be accessible through kernel virtual data space at all times.
In this way the same virtual address range can be used for several different
objects.
.NH 2
History
.PP
The original Berkeley \s-2PDP-11\s0 kernel distribution
(\s-12.8BSD\s0) provided the
ability to move buffers and clists out of kernel data space.  Buffers
were accessed by mapping them in through KDSA5.  A side effect was that
the data that normally resided there were unavailable until buffers
were mapped out again.  Clists were mapped in through KDSA1 with the
same side effect.
.PP
Because of this restriction, and the possibility of
interrupts at any time, sections in which a kernel data register
was repointed generally had to be protected by \fIspl6\fP\|()/\fIsplx\fP\|()
pairs.  (The exception is that \fIspl\fP\^s were unnecessary for buffer mapping
if KDSA5 was used only for that purpose, and this was not done from
interrupt level.)
This inevitably led to increased interrupt latency and sometimes
caused the system clock to lose time perceptibly.
.PP
It is not at all clear why these registers were special.
They were chosen after careful examination of the system namelist.
On our 11/70s, the inode table used all virtual addresses
referenced through KDSA1 and it was known that no part of the kernel
required simultaneous access to clists and inodes.  Similarly, it
was observed that data referenced through KDSA5 typically consisted of
tty structures and the kernel did not require simultaneous access
to tty structures and buffers.
.PP
It should be obvious how vulnerable this method is to even the most
trivial changes such as system load order or table sizes.  Clearly
something better was needed.
.NH 2
2.9BSD Methods
.PP
We chose four goals for our new remapping protocols:
.IP [1]
They must be fast. Interrupt latency should not be
increased by elevating the processor priority.
.IP [2]
They should be flexible, allowing objects other than buffers
and clists to be remapped easily. 
.IP [3]
Interrupt service routines should not be slowed unnecessarily by requiring
that the map be changed on all interrupts.
.IP [4]
There must be a well-defined class of objects that the
remapping will make inaccessible.  Furthermore, any section
of code that requires access to one of these objects during interrupt processing
must itself ensure that the object is mapped in.
.PP
The implementation we chose uses KDSA5 as the primary mapping register.
The only normally-resident objects allowed in this region (0120000 to 0140000)
are the \fIproc\fP, \fIfile\fP, and \fItext\fP tables.
These objects were chosen because they are rarely
accessed from interrupt level.
If kernel data space is small enough that these tables end before this region,
the code can be further simplified by defining the conditional-compilation
flag NOKA5.
In general, kernel functions are able to map in external data at will,
with the caveat that interrupt routines must save the previous map
(which may already point at some mapped-in object).
.PP
To make \fIcopy\fP (previously \fIcopyseg\fP)
as fast as possible, yet interruptible,
we also allow it to use KDSA6 as a mapping register.
This makes the normal kernel stack (which lies in the region
addressed by KDSA6) inaccessible, so the kernel uses a
temporary stack while in \fIcopy\fP.
.PP
Most of the segmentation map switching is done by macros
for speed; some of the macros test whether any work need be done
before calling a subroutine.
The data structures and macros used in this scheme are in
the include file \fIseg.h\fP, with the subroutines in \fImachdep.c\fP.
These macros must be used for all kernel remapping or races will ensue
(because the order in which registers are set is critical to the protocol).
.NH 3
Top Level Protocol
.PP
A global prototype page address/descriptor pair is maintained (if
necessary) for virtual addresses from 0120000 to 0140000.  It is
initialized in \fIstartup\fP.
KDSA5 may be repointed to access other objects from the top level
provided that the normal mapping is restored before the next
context switch.
The contents of KDSA5/KDSD5 are changed by the macro call
.br
.nf

	\fImapseg5(addr, desc);\fP

.fi
where \fIaddr\fP is the new value for KDSA5 and \fIdesc\fP
is the new value for KDSD5.
The default mapping for this page is restored by the macro call
.br
.nf

	\fInormalseg5();\fP

.fi
The \fImapin\fP and \fImapout\fP functions
use this method to provide access to a mapped buffer.
.PP
Unless the kernel data map has been explicitly reset by \fImapin\fP
or \fImapseg5\fP,
the \fIproc\fP, \fIfile\fP, and \fItext\fP tables are guaranteed
to be mapped in when the kernel is not at interrupt level.
.NH 3
Interrupt Level Protocol
.PP
Interrupt-level routines may not assume that the range controlled
by KDSA5 or KDSA6 contains valid data unless the map is explicitly
set to either the normal state
(for the \fIproc\fP, \fItext\fP or \fIfile\fP tables, or for the \fIu.\fP)
or to map external data.
.PP
Interrupt routines that wish to repoint KDSA5 must first save the
current contents of KDSA5 and KDSD5 in a local variable by
.br
.nf

	\fIsegm saveregs;
	saveseg5(saveregs);\fP

.fi
before changing their contents with \fImapseg5\fP.  Before returning,
the old contents must be restored by the call
.br
.nf

	\fIrestorseg5(saveregs);\fP

.fi
This method is used by \fIgetc\fP and \fIputc\fP to
access the clist area.
.PP
Note that \fImapin\fP does not save the current map in this way.
To use \fImapin\fP and \fImapout\fP from interrupt level, it is necessary
to save the map with \fIsaveseg5\fP before calling \fImapin\fP,
and then restore it with \fIrestorseg5\fP after the last \fImapout\fP.
.PP
If an interrupt routine must access either the \fIu\fP. or any of
the tables, it must save the previous PARs and PDRs for pages 5
and 6 in a local variable and set the map to the normal state using
.br
.nf

	\fImapinfo map;
	savemap(map);\fP

.fi
and restore the old contents with
.br
.nf

	\fIrestormap(map);\fP

.fi
This mechanism is used by \fIgsignal\fP and \fIwakeup\fP, which are frequently
called from interrupt level and must access the \fIproc\fP table,
and by \fIclock\fP, which needs access to the \fIproc\fP table and the
\fIuser\fP structure.
It is also used in \fItrap\fP, which saves the map data in the global
map \fIkernelmap\fP on kernel-mode traps for potential use in debugging.
.bp
.ce
.I "Appendix B:  UNIBUS Map Protocols"
.sp 5
.NH
Introduction
.PP
\s-2UNIX\s0 as distributed by Bell Labs and in previous Berkeley
releases made some tacit assumptions about the arrangement of
kernel data space and the use of the UNIBUS map (or machines with
22-bit addressing):
.IP \(bu
All kernel data space was statically covered by some portion of the UNIBUS
map.  This included mapped out objects such as buffers and clists.
Kernel virtual data space addresses needed no conversion to UNIBUS
or physical addresses.
Thus no special action was taken on, for example, DMA transfers from kernel
data space
to ensure that the source or target area was accessible through
the UNIBUS map.
.IP \(bu
The remaining portion of the UNIBUS map was dedicated to only one
I/O request at a time.  Thus a fixed portion of the UNIBUS map
was used for each physical I/O request.
.PP
Although these assumptions did result in much simpler code, they had
the unfortunate side effect of degrading system performance.  Two swaps
could not occur simultaneously.  When a slow device such as a tape drive
was used for physical I/O, all other physical I/O suffered severely.
This was most noticeable when file system dumps were occurring.
It also made the use of raw I/O for real-time data acquisition impossible.
.NH 2
2.9BSD Methods
.PP
The solution is to manage the UNIBUS map with a resource map,
allocating and freeing groups of registers as required by
the size of the I/O request.  This has already been implemented
independently at some sites.  Our code is modeled after several
of these.
.PP
In an effort to have as many UNIBUS map registers as possible
available for allocation, only the clist area
and buffer pool have statically allocated UNIBUS map registers.  The clist area
is mapped through UNIBUS register 0.  It may therefore be at most 8192 bytes
long, and begins at UNIBUS virtual address 0.
The global variable \fIclstaddr\fP contains the UNIBUS
address (in bytes) of clists (even if a UNIBUS map is not present).
The appropriate number of registers is dedicated to the buffer pool
at boot time and the rest are made available for allocation.
When there is a UNIBUS map,
the buffers begin at UNIBUS byte address BUF_UBADDR,
whereas their physical address (in clicks) is \fIbpaddr\fP.
.PP
Routines that manipulate the UNIBUS map must be prepared to be
called even if no UNIBUS map exists.  They should check the boolean
variable \fIubmap\fP, which is nonzero if a UNIBUS map is present.
For convenience, several useful macros have also been provided.
See the include file \fIuba.h\fP.
.PP
The code for block I/O dynamically supports both MASSBUS and UNIBUS
controllers.  A buffer header associated with the buffer cache
used for block I/O normally
contains the physical address of the buffer area.
This is translated into a UNIBUS address
before beginning the I/O operation
if the device does not use 22-bit addressing.
This translation is performed by \fImapalloc\fP;
thus, UNIBUS disk and tape drivers should call \fImapalloc\fP
for both raw operations (B_PHYS set) and those in the buffer cache.
While a buffer header
contains the UNIBUS virtual address of the buffer area instead of
the physical address, the B_UBAREMAP flag is set in its \fIb_flags\fP
field.
After the transfer is finished, \fIiodone\fP restores the physical
address in the buffer header.
Drivers for disks that may be either MASSBUS or UNIBUS generally
set the B_RH70 flag in the \fIb_flags\fP of their \fIdevtab\fP structures
if they are 22-bit MASSBUS devices and test it before calling \fImapalloc\fP.
.bp
.ce
.I "Appendix C:  Terminal and Line Discipline Changes"
.sp 5
.NH
Introduction
.PP
There have been several changes in the kernel terminal-handling routines.
The initial incentive for these changes was to allow
the old tty discipline to be removed. This required that line
disciplines be symmetric and equivalent.  Previously, line discipline 0
(the old tty driver) was treated specially and was assumed to exist.
.NH 2
Ttyopen and Ttyclose
.PP
The first group of changes is in the open and close sections.  The
routines \fIttyopen\fP and \fIttyclose\fP are no longer part of any
discipline, but do the necessary initialization at the first open and
the breakdown at the final close.  They call the line
discipline-specific open or close routine, and all the drivers (dh, dz,
kl etc.) need do is call \fIttyopen\fP and \fIttyclose\fP from their
open and close routines.
.NH 2
Ioctl Protocols
.PP
The second set of changes is in the ioctl-handling sections.  The line
disciplines are given the opportunity to reject or modify any
\fIioctl\fP call, or to do it themselves, before the common code
is reached.  Again, all the work is done by the
discipline-independent routine, \fIttioctl\fP, which calls the line
discipline's ioctl routine.  The device drivers thus call only
\fIttioctl\fP. There are three possible return conditions from
\fIttioctl\fP:
.IP \(bu
a command is returned that the device driver is
expected to execute
.IP \(bu
0 is returned with \fIu.u_error\fP clear, meaning that
the command completed successfully
.IP \(bu
0 is returned with \fIu.u_error\fP set, meaning that
the command completed abnormally
.KS
.PP
The typical device driver ioctl routine will thus look like this:
.nf
.sp 
	\fBswitch\fP (ttioctl(tp, cmd, addr, flag)) \fB{\fP
		\fBcase\fP TIOCSETP:
		\fBcase\fP TIOCSETN:
			setparam(unit);
			\fBbreak\fP;
		\fBcase\fP other_known_command:
			implement the command;
			\fBbreak\fP;
		\fBdefault\fP:
			u.u_error = ENOTTY;
		\fBcase\fP 0:
			\fBbreak\fP;
	\fB}\fP
.fi
.KE
.NH 2
Line Switch Changes
.PP
There are a few other differences in the terminal handlers from
previous systems.  The line discipline switch is no longer optional
(the defined constant UCB_LDISC is gone).  The linesw can have unused discipline
entries in it, so that line discipline numbering is independent of the
disciplines supported at any time; unused disciplines are marked by
using \fInodev\fP as their open routines, thus preventing entrance into
them.  This necessitates a new defined constant, DFLT_LDISC,
which is the line discipline that device drivers should set on initial
open.  Finally, the line discipline switch itself has been reorganized,
with three entries being deleted and one field added.  The
previously-unused \fIl_rend\fP and \fIl_meta\fP pointers have been
removed, and calls to \fIl_start\fP have been replaced with calls to
\fIttstart\fP.  The \fIl_rint\fP entry has been renamed \fIl_input\fP and
an \fIl_output\fP pointer has been added for the use of \fIuprintf\fP.
.bp
.ce
.I "Appendix D:  Vfork Implementation Notes"
.sp 5
.PP
The kernel changes for the \fIvfork\fP system call
are major and deserve a few notes.
Processes are no longer in one piece,
but instead the user structure, data segment, and stack segment
are separate.
They are located at \fIp\->p_addr\fP, \fIp\->p_daddr\fP, and \fIp\->p_saddr\fP
respectively (where \fIp\fP is a pointer to a proc entry)
and their sizes are USIZE, \fIp\->p_dsiz\fP and \fIp\->p_ssiz\fP.
The latter two are copies of the entries in the user structure.
All segments are swapped if any are,
and there is a new routine, \fImalloc3\fP, to allocate memory or swap
for all three segments at once.
When a \fIvfork\fP occurs, the \fIu\fP. is copied, and the data and stack
are passed to the child.
The parent sleeps until the child calls \fIexec\fP or \fIexit\fP.
At that time, the child locks itself in core and waits for the parent
to reclaim the data and stack.
.PP
The major advantages of these changes are the efficiency of avoiding the
copy in \fIfork\fP,
and more efficient utilization of memory, as processes are in smaller
segments.
The disadvantage is that swaps require three separate transfers
in each direction.
Except on heavily loaded systems with small main memory, the
result should be a net gain.
There is a potential for deadlock since the child must lock itself
into core; this can only be a problem with small memories when the parent
has been swapped out.
To help avoid problems, the swapping algorithm has been changed to
swap in the parent process in a vfork before any others.
.bp
.ce
.I "Appendix E:  Autoconfiguration
.sp 5
.PP
The kernel changes to add autoconfiguration are fairly small.
The most global change is that device CSR addresses and interrupt
vectors must be initialized only for disk drivers which service
root devices.
Most of the work of autoconfiguration is done in user mode by
\fIautoconfig\fP\|(8).  It reads the device table \fI/etc/dtab\fP,
then verifies the CSR address by reading from it (through /dev/kmem).
If the CSR is present, \fIautoconfig\fP then tries to make the device
interrupt in order to check that the vector specified is correct.
To facilitate this check, l.s has two interrupt catchers,
\fICBAD\fP and \fICGOOD\fP, that set the global variable \fI_conf_int\fP
to \-1 and 1 respectively when called.
\fIAutoconfig\fP sets all unused vectors to \fICBAD\fP, then sets the expected
vector to \fICGOOD\fP.
After the probe, \fIautoconfig\fP checks the contents of \fI_conf_int\fP
to see whether the device interrupted and whether it was through the expected
vector.  If everything is correct to this point, \fIautoconfig\fP calls
the device driver's attach routine with the unit number and address,
then sets up the interrupt vector.
.PP
The kernel support for autoconfiguration consists of two parts.
The first includes the interrupt catchers in l.s and a new routine
in syslocal.c that allows \fIautoconfig\fP to call the driver attach routines.
This new system call, \fIucall\fP (see \fIucall\fP\|(2)),
calls a specified kernel routine (by address) at a specified priority
with two user-supplied arguments.  The other group of changes is in the drivers.
Most drivers have new attach routines which simply place the address
specified into their address arrays, checking that the unit number
is in range.
Device open and/or strategy routines have been modified to test
that the device address has
been set before allowing the open, read, or write to succeed.
Drivers that need to probe the hardware to test its type may
do that as well in the attach routine.
The drivers that handle both MASSBUS and UNIBUS devices check for
bus address extension registers at this time.
A new routine, \fIfioword\fP, is provided to read a word from the
I/O page, returning -1 if the address does not exist.
Because the disks must be attached before \fIautoconfig\fP runs
if they are to be used for root file systems,
their addresses and vectors are still initialized.
A new entry in the block device switch, \fId_root\fP, is used
at boot time to call driver routines which disk drivers may use
to attach all known devices before \fIiinit\fP.
This allows them to determine controller and drive types.
Drivers currently fall into three classes:  UNIBUS only disks, MASSBUS/UNIBUS
disks, and others.  Prototypes of the attach and \fId_root\fP
routines for each class follow.
.PP
The probe routines that are used to make the devices interrupt
may be either in \fIautoconfig\fP or in the kernel.
If the kernel has a probe routine, that will be used, otherwise
\fIautoconfig\fP will use its own probe.  This mechanism is provided
because it may be difficult to address some devices properly
by reading and writing /dev/kmem.
All current probe routines are internal to \fIautoconfig\fP.
.PP
Device drivers that have no \fIattach\fP routines are ignored
by \fIautoconfig\fP.
Old drivers that have not been converted to use autoconfiguration
will thus work properly.
.bp
.KS
.nf
.vS
/*
 *	Example 1:  autoconfiguration prototype for devices other
 *	than disks.  Xxattach will be called by autoconfig(8).
 */

xxattach(addr, unit)
struct xxdevice *addr;
{
	if ((unsigned) unit >= NXX)
		return(0);
	xx_addr[unit] = addr;
	return(1);
}

/*ARGSUSED*/
xxopen(dev, flag)
dev_t	dev;
int	flag;
{
	register int	unit = XXUNIT(dev);

	if (xx_addr[unit] == (struct xxdevice *) NULL) {
		u.u_error = ENXIO;
		return;
	}
	if (unit >= NXX) {
		u.u_error = EINVAL;
		return;
	}
	.
	.
	.
}
.vE
.fi
.KE
.sp 10
.KS
.nf
.vS
/*
 *	Example 2:  autoconfiguration prototype for UNIBUS disks.
 *	Xxattach will be called by autoconfig(8).
 */

xxattach(addr, unit)
struct xxdevice *addr;
{
	if (unit != 0)
		return(0);
	XXADDR = addr;
	return(1);
}

xxstrategy(bp)
register struct buf *bp;
{
	if (XXADDR == (struct xxdevice *) NULL) {
		bp->b_error = ENXIO;
		goto errexit;
	}
	if (bp->b_blkno >= NXXBLK) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	.
	.
	.
}
.vE
.fi
.KE
.sp 10
.KS
.nf
.vS
/*
 *	Example 3:  autoconfiguration prototype for disks
 *	possibly on the MASSBUS.  Xxroot will be called
 *	from binit (main.c).
 */

void
xxroot()
{
	xxattach(XXADDR, 0);
}

xxattach(addr, unit)
register struct xxdevice *addr;
{
	if (unit != 0)
		return(0);
	if ((addr != (struct xxdevice *) NULL) && (fioword(addr) != -1)) {
		XXADDR = addr;
#if	PDP11 == 70 || PDP11 == GENERIC
		if (fioword(&(addr->xxbae)) != -1)
			xxtab.b_flags |= B_RH70;
#endif
		return(1);
	}
	XXADDR = (struct xxdevice *) NULL;
	return(0);
}

xxstrategy(bp)
register struct buf *bp;
{
	register unit;
	long	bn;

	if (XXADDR == (struct xxdevice *) NULL) {
		bp->b_error = ENXIO;
		goto errexit;
	}

	unit = minor(bp->b_dev) & 077;
	if (unit >= (NXX << 3) || bp->b_blkno < 0 ||
	   (bn = dkblock(bp)) + ((bp->b_bcount + 511) >> 9)
	   > xx_sizes[unit & 07].nblocks) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	.
	.
	.
}
.vE
.fi
.KE

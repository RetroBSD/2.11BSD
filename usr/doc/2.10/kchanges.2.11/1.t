.nr i \w'\fBkern_resource.c\0'u/1n
.de XP
.IP \\fB\\$1\\fP \\ni
..
.NH
Organizational changes
.PP
The directory organization and file names are almost identical
to that of \*(Ps.  This document summarizes the changes between
\*(Ps and \*(2B.  For a description of the differences between 2.9BSD
and \*(Ps refer to the documentation in /usr/doc/2.10/kchanges.2.10.
.PP
The following files in /sys/h have changed in \*(2B.
.XP uio.h
The 4.3BSD \fIuio\fP and \fIiovec\fP structures were ported.
scatter/gather i/o is implemented in \*(2B.
The compatibility routines have been replaced with system calls.
.XP buf.h
The device number was included in the buffer hashing function.  The
.B SMALL
conditional was removed.
.XP file.h
The \fIfileops\fP structure from 4.3BSD was added.
.XP dir.h
The 4.3BSD version was ported, the old V7 directory structure is gone.
.XP mbuf.h
Change made to the mbuf allocation macro to call the drain routines
if out of mbufs.  This is useful when a large number of connections
are in TIME_WAIT states.
.XP namei.h
The 4.3BSD \fInamei\fP argument encapsulation and inode cache
structures were ported.
.XP proc.h
The
.B SMALL
conditional was removed because it is now standard.
.XP user.h
Many changes made to the \fIuser\fP structure.  The members \fIu_offset\fP,
\fIu_count\fP, \fIu_segflg\fP, and \fIu_base\fP do not exist now.  
The \fIu_nd\fP and \fIu_ncache\fP are either new or changed.
.XP msgbuf.h
The 4.3BSD kernel error logger (\fI/dev/klog\fP) is now present in \*(2B,
the message buffer is now external to the kernel.
.XP inode.h
Both the on disk and incore inode structures have changed.  The incore
inode has had \fIi_id\fP added as part of the \fInamei\fP cache support.
The on disk structure defines block numbers as \fBdaddr_t\fP now, the
old 3 byte packing scheme is obsolete at last.
.NH 2
/sys/sys
.PP
The following files were changed for \*(2B.
.XP tty.c
Changes to use \fBuio/iovec\fP instead of fixed offsets in user
structure.
.XP vm_text.c
Use new \fIrdwri\fP kernel I/O routine.
.XP init_sysent.c
Added \fIreadv\fP and \fIwritev\fP.
.XP init_main.c
Use \fIrdwri\fP and \fInamei\fP argument encapsulation.
.XP ufs_namei.c
Totally rewritten from the 4.3BSD source.  Implements the argument
encapsulation and maintains the name translation cache.
.XP kern_exec.c
Use \fIrdwri\fP instead of \fIreadi\fP.
.XP sys_pipe.c
Use \fIrdwri\fP.  Defines a \fIfileops pipeops\fP table for use by
sys_generic.c in dispatching i/o requests.
.XP kern_acct.c
\fInamei\fP encapsulation when looking up accounting file name.
.XP sys_inode.c
Defines \fIfileops inodeops\fP, use \fIrdwri/rwip\fP, \fBQUOTA\fP
check ignores \fBPIPES\fP now.  Remove a couple extraneous 
\fIsaveseg5\fP calls.
.XP kern_clock.c
Autonice long running processes like 4.3BSD does.  Programs in an
endless loop impact the system less if this is done.
.XP kern_descrip.c
Uses the externally \fIfileops\fP tables to dispatch file requests.
\fIclosef\fP had the extra argument \fInouser\fP removed because it
was no longer used and the 4.3BSD sources did not refer to it any longer.
.XP ufs_fio.c
Use the \fInamei\fP argument encapsulation.
.XP ufs_inode.c
Extra \fIsaveseg5\fP call removed.  \fIigrab\fP ported from 4.3BSD.
\fBSMALL\fP conditional definition of HASHSIZE removed, the smaller
value made the default.
.XP sys_generic.c
\fIreadv\fP and \fIwritev\fP implemented.  \fIfileops\fP and \fIuio/iovec\fP
changes.
.XP kern_sig.c
\fInamei\fP argument encapsulation changes in \fIcore\fP.  The core
file written using \fIrdwri\fP instead of \fIwritei\fP.
.XP kern_subr.c
\fIuiomove\fP rewritten to use \fIuio/iovec\fP mechanism.
.XP kern_synch.c
\fBSMALL\fP conditional definition of \fBSQSIZE\fP removed, the smaller
value made the default.
.XP subr_prf.c
\fIlog\fP defined to provide the kernel interface to \fBsyslogd\fP.
The message buffer is now 4kb and resides external to the kernel.
.XP ufs_alloc.c
Extra include files needed due to changes in some of the \fI/sys/h\fP
files.
.XP tty_pty.c
Changes to use \fIuio/iovec\fP mechanism.
.XP vm_swp.c
\fIphysio\fP and \fIphysstrat\fP ported from 4.3BSD and now use
the \fIuio/iovec\fP mechanism.
.XP tty_tb.c
\fIuio/iovec\fP changes.
.XP ufs_mount.c
\fInamei\fP argument encapsulation changes.  Prevent \fImount\fP
on a directory which  is already itself a \fImount\fP point.
.XP ufs_bmap.c
Additional include files needed due to changes in \fI/sys/h\fP.
.XP tty_tty.c
\fIuio/iovec\fP changes.
.XP ufs_syscalls.c
\fIrename\fP ported fresh from 4.3BSD, \fInamei\fP argument
encapsulation changes, directory syscalls changed to handle the
new directory structures.  Many changes in this file.
.XP uipc_socket.c
\fIuio/iovec\fP changes.
.XP uipc_mbuf.c
Changes to call the drain routines on a mbuf shortage.  This is 
useful if many sockets are in the TIME_WAIT state at once due to
something like a \fIftp mget/mput\fP transferring many files in
a short period of time.  The storage for the starting addresses 
(both physical click
and UNIBUS) of the DMA I/O region are declared here, see 
\fIsys_net.c\fP for their use.
.XP uipc_usrreq.c
\fIuio/iovec\fP changes.
.XP quota_kern.c
\fIrdwri\fP used instead of \fIreadi\fP.
.XP sys_net.c
The ACC LH/DH-11 (if_acc.c) and Proteon proNET (if_vv.c) network
interfaces added.  \fIuiomove\fP rewritten to use \fIuio/iovec\fP
mechanism.  \fIputchar\fP redefined as \fI_pchar\fP and a macro
written to call the kernel \fIputchar\fP routine.  This causes
networking error messages to be passed thru the kernel logger to
\fIsyslogd\fP.  A missing third argument added in the \fISKcall\fP
to \fIputchar\fP.  To greatly reduce the number of UMRs consumed by
network interface drivers the DMA I/O region is mapped once using
the minimal number of UMRs required.  The starting click address and
UNIBUS virtual address are saved for use in \fIpdpif/if_uba.c\fP.
.XP sys_kern.c
\fInamei\fP argument encapsulation changes.
.XP subr_log.c
The kernel logger ported from 4.3BSD.
.XP uipc_syscalls.c
\fIuio/iovec\fP changes, \fIsendmsg\fP, etc ported fresh from 4.3BSD
to handle scatter/gather i/o correctly.
.NH 2
/sys/conf
.PP
The following files were changed for \*(2B.
.XP GENERIC
\fBSMALL\fP was moved to the always defined category to save kernel
D space.
.XP Make files
The Make.* files were modified to add the kernel logger and to
reflect the changes in some of the kernel file names.  With
file names greater than 14 characters supported, kern_resrce.c becomes
kern_resource.c, etc.
.XP checksys.c
There were several bugs fixed in the program's calculation of
how much memory the system would occupy.
.NH 2
/sys/pdpuba
.PP
Almost \fBALL\fP of the files in this directory were modified.  The
modifications were small, usually nothing more than passing an
extra \fIuio\fP argument in the \fIxxread\fP and \fIxxwrite\fP 
functions on thru to \fIphysio\fP.
.XP tmscp.c
This driver is new to \*(2B.  At present it has only been tested on 
an 11/73 with a TK50.
.PP
In addition the \fIsys/OTHERS\fP directory has had several ``new'' device
drivers added to it that may or may not work.
A cursory pass was made thru this directory to add the \fIuio/iovec\fP
changes - no guarantee is made that all necessary changes were made, or
that the changes made will work.
It is extremely probable
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
The following files were changed in \*(2B:
.XP conf.c
Entries added in the \fIcdevsw\fP and \fIbdevsw\fP tables for
\fI/dev/klog\fP and \fItmscp\fP tapes.
.XP seg.h
Changed the \fBnormalseg5\fP macro to not depend on \fBQUOTA\fP.  This
is safe since quota manipulation only occurs in the high kernel.
.XP scb.s
Define vectors for ACC LH/DH-11 and Proteon proNET network interfaces.
.XP machparam.h
Definitions from files in /usr/include/OLD were moved into this file.
Almost all of /usr/include/OLD has been removed.
.XP trap.c
Changes made to save more information if the network crashes and to
prevent further corruption from happening.
.XP machdep2.c
Allocate memory for the external kernel error message buffer and the
\fInamei\fP cache.
.XP mch_click.s
Extra \fBmov\fP instructions removed (to save I space) and the loop
count doubled.  Never did know why memory was copied in 4 loops of
8 \fBmov\fP instructions instead of 8 loops of 4 \fBmov\fP instructions.
.XP mem.c
Ported fresh from 4.3BSD.  Uses the \fIuio/iovec\fP mechanism now.
.XP cons.c
Changes to use the \fIuio/iovec\fP mechanism.
.XP mch_dump.s
Save area for extra information on network crash allocated.
.XP mch_xxx.s
Same changes as made to mch_click.s and the \fIdelay\fP routine is
no longer conditional on a networking system being defined.
.XP mch_var.s
The flag \fIubmap\fP was made an \fIint\fP instead of a \fIchar\fP to
force even alignment.  Even alignment is required for use by the
\fImfkd\fP function from the networking code.  The networking no longer
has its own private/wired copy of \fIubmap\fP, instead the kernel version
is examined in exactly one place:\fIpdpif/if_uba.c\fP.
.XP net_mac.h
\fIputchar\fP macro defined for the supervisor mode networking to
use when calling the kernel \fIputchar\fP routine.  The \fINETUBAA\fP
macro was modified to be a 0 if not on a UNIBUS system, this allows
code to be written which checks \fIubmap\fP and references NETUBAA for the
UNIBUS case but returns earlier for the Qbus case.
.XP net_trap.s
Network device interrupts now included in system interrupt counts.
.XP tmscp.h
This file is new and contains the definitions for the TMSCP driver.
.NH 2
/sys/mdec
.PP
\fBALL\fP of the bootstraps have been modified to read the new
on disk directory structure.  The changes to read the more complicated
directory format necessitated the removal of prompting from all
bootstraps.  If \fB/boot\fP can not be found you are in deep trouble.
.NH 2
/sys/netinet
.PP
The following were changed in \*(2B:
.XP raw_ip.c
Changes to support the \fItraceroute\fP utility.
.XP tcp_subr.c
Changes to support the \fItraceroute\fP utility.
.NH 2
/sys/pdpstand
.PP
The following were changed in \*(2B:
.XP maketape.c
Changes made to use \fImtio\fP(4) functions to write tapemarks instead
of doing a open/close/open sequence on the non-rewind tape.  This change
was necessitated by TU81s at 1600BPI.
.XP sys.c
New directory reading routine written.
.XP ra.c
Error checking corrected.
.XP tmscp.c
This is a newly ported standalone TMSCP driver (TK50/TU81).
.NH 2
/sys/netimp
.PP
The following were changed in \*(2B:
.XP if_imphost.h
.XP if_imp.c
.XP if_imphost.c
Porting to the supervisor mode networking, some changes to fix
compiler errors, other changes to fix bugs.
.NH 2
/sys/pdpif
.PP
The following were changed in \*(2B:
.XP if_vv.h
.XP if_vv.c
Under \*(Ps these files were accidentally placed in the supported
directory when they would not even successfully compile.
For \*(2B these were ported from 4.3BSD and the changes necessary
to operate in the supervisor mode networking were made.
.XP if_acc.c
Changes to run in supervisor mode were made as well as several
bugs (missing arguments, etc) being fixed.
.XP if_uba.c
The routine \fIubmalloc\fP() was rewritten to compute addresses using
the pre-allocated UMRs which map the DMA I/O region.  \fIubmalloc\fP()
does not allocate UMRs now.  Also, the calling convention of \fIuballoc\fP
and \fIubmalloc\fP has changed.
.XP if_de.c
.XP if_de.h
The calls to \fIuballoc\fP and \fIubmalloc\fP had to be changed.
The \fIdereset\fP routine was removed for two reasons:  1) The concept
of a UNIBUS being reset without a system reboot is meaningless on a PDP-11,
2) \fIdereset\fP wouldn't work even if it was called due to resource
exhaustion.
.XP if_il.c
The second call to \fIuballoc\fP() in \fIilinit\fP() was removed.  The
UNIBUS resources are allocated at attach time, the second call was 
allocating (and wasting) UMRs which were not needed.
.XP if_dmc.c
The reset routine was removed for the same reasons it was removed in
\fIif_de.c\fP.
.XP if_qe.c
The previous version of the driver was flaky and would hang at
unpredictable time.  The current version is marginally slower,
but is a fresh port of the 4.3BSD driver which no longer hangs or
grabs and holds 20 mbufs.  This driver will statically allocate 
a fixed number of buffers from main memory after first using the
the memory dedicated to the network DMA arena.
.NH 2
/sys/net
.PP
The skeletal support for PUP has been removed.

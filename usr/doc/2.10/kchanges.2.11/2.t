.de UX
UNIX\\$1
..
.FS
.IP \(ua
UNIX is a Trademark of Bell Laboratories.
.FE
.NH
Changes inside the kernel
.IP 1)
The user structure \fIu\fP has undergone several changes.  The members
\fIu_offset\fP, \fIu_count\fP, \fIu_segflg\fP and \fIu_base\fP have
been removed.  Drivers and programs which refer to them will no longer
compile.  The kernel now allocates a \fIstruct uio\fP dynamically
on the stack and refers to the \fIuio_offset\fP, \fIuio_iovec\fP,
\fIuio_segflg\fP members, exactly as 4.3BSD does.
.IP
The \fIu_ncache\fP structure was modified to remove the timestamp
member to save space - there were no references anywhere to it.
.IP
The changes to the \fIu\fP structure make old core images unusable
by the debugger \fIadb\fP.
.IP 2)
The \fInamei\fP function has a new calling convention
with its arguments, associated context, and side effects
encapsulated in a single structure.
\fInamei\fP has been extensively modified to implement the name cache
and to cache directory offsets for each process.
It may now return ENAMETOOLONG when appropriate,
and returns EINVAL if the 8th bit is set on one of the path name
characters.
.IP 
The automatic and silent truncation of file names to 14 characters is
not performed - a name longer than MAXNAMELEN (currently 63) will
produce the error ENAMETOOLONG.
.IP
A table of recent name-to-inode translations is maintained by \fInamei\fP,
and used as a look-aside cache when translating each component of each
file path name.
Each \fInamecache\fP entry contains the parent directory's device and inode,
the length of the name, and the name itself, and is hashed on the name.
It also contains a pointer to the inode for the file whose name it contains.
Unlike most inode pointers, which hold a ``hard'' reference
by incrementing the reference count,
the name cache holds a ``soft'' reference, a pointer to an inode
that may be reused.
In order to validate the inode from a name cache reference,
each inode is assigned a unique ``capability'' when it is brought
into memory.
When the inode entry is reused for another file,
or when the name of the file is changed,
this capability is changed.
This allows the inode cache to be handled normally,
releasing inodes at the head of the LRU list without regard for name
cache references,
and allows multiple names for the same inode to be in the cache simultaneously
without complicating the invalidation procedure.
An additional feature of this scheme is that when opening
a file, it is possible to determine whether the file was previously open.
This is useful when beginning execution of a file, to check whether
the file might be open for writing, and for similar situations.
.IP 3)
Compatibility with previous versions of PDP-11
.UX \(ua
is limited to partial binary compatibility with \*(Ps.  \*(Ps programs
which do not read directories or the password file should run without
change.  It is \fBstrongly\fP recommended that applications be recompiled
to obtain the benefit of the changes to the system libraries.
.IP
Because the file system has changed with \*(2B old file systems
can not be mounted.  There 
is a version of \fIdump\fP available in /usr/src/old/dump which understands
the old directory structure of 2.10.1BSD filesystems.
.IP
Old \fIdump\fP tapes from 2.10BSD or \*(Ps are automatically converted
by \*(2B's \fIrestor\fP on input.
.IP 4)
As mentioned earlier, the \fIuio/iovec\fP method of kernel I/O has
been implemented.  If you have local drivers, they will require some
work.  There are many working examples in \fI/sys/pdpuba\fP.  The
routines \fIreadi\fP and \fIwritei\fP do not exist any longer, having
been replaced with the general 4.3BSD \fIrdwri\fP routine.
.IP 5)
The ``real-time'' features of 2.9BSD should
probably go away, but for now they 
have been left in place, and, should
work as they always have, with one major exception.  The \fIfmove\fP()
routine has not been fixed to be interruptible.  See the \fIcopy\fP()
routine for examples of what needs to be done to make it behave correctly.
This, however, will be fairly difficult.  I suggest that if you want to
use \fBCGL_RTP\fP that you comment out the use of \fIuiofmove\fP() in
\fIuiomove\fP().
.IP 6)
The 4.3BSD kernel logger \fI/dev/klog\fP has been implemented.
Kernel messages are placed in the message buffer
and are read from there through the log device \fI/dev/klog\fP.
The \fIlog\fP routine is similar to \fIprintf\fP
but does not print on the console, thereby suspending system operation.
\fILog\fP takes a priority as well as a format, both of which are read from
the log device by the system error logger \fIsyslogd\fP.
\fIUprintf\fP was modified to check its terminal output queue
and to block rather than to use all of the system clists;
it is now even less appropriate for use from interrupt level.
\fITprintf\fP is similar to \fIuprintf\fP but prints to the tty specified
as an argument rather than to that of the current user.
\fITprintf\fP does not block if the output queue is overfull, but logs only
to the error log;
it may thus be used from interrupt level.
Because of these changes, \fIputchar\fP and \fIprintn\fP require
an additional argument
specifying the destination(s) of the character.
The \fItablefull\fP error routine was changed to use \fIlog\fP rather than
\fIprintf\fP.  Some of the other drivers \fIdh\fP and \fIbr\fP also
have been modified to use \fIlog\fP.
.IP
The message buffer is now 4kb in size and is external to the kernel.  The
message buffer is mapped when data is written or read from the buffer.  This
obsoletes \fIdmesg\fP(8) which has been modified to find the external
message buffer on the off chance anyone still wants to run it.
.IP 7)
Most of the conditional compilation defines in the 2.9BSD kernel have
been removed because the features they controlled are now either standard
The following table lists \fI#defines\fP that are now a standard part
of \*(2B.
.TS
center box;
l | l | l
l | l | l.
define name	feature	comment
_
SMALL	use smaller hash table and queues to save D space
.TE
.IP 8)
Directory truncation now is performed the same way as 4.3BSD does it,
and directories are always a multiple of 512 bytes.  The old method
of truncating directories with 10 or more trailing empty slots has
disappeared.  There is a new version of \fIfsck\fP which can
automatically create and extend (up to the number of direct blocks
allowed, currently 4) the \fIlost+found\fP directory.
.IP 9)
Again, it must be mentioned that this document summarizes the changes
from \*(Ps to \*(2B.  If you are upgrading from a earlier version than
\*(Ps or 2.10BSD you will want to format and read the documentation
in /usr/doc/2.10/kchanges.2.10 and /usr/doc/2.10/setup.2.10.

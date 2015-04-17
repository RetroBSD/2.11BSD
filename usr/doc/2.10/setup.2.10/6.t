.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)6.t	6.2 (Berkeley) 10/1/88
.\"
.de IR
\fI\\$1\fP\|\\$2
..
.ds LH "Installing/Operating \*(2B
.nr H1 6
.nr H2 0
.ds RH "System Operation
.ds CF \*(DY
.bp
.LG
.B
.ce
6. SYSTEM OPERATION
.sp 2
.R
.NL
.PP
This section describes procedures used to operate a PDP UNIX system.
Procedures described here are used periodically, to reboot the system,
analyze error messages from devices, do disk backups, monitor
system performance, recompile system software and control local changes.
.NH 2
Bootstrap and shutdown procedures
.PP
In a normal reboot, the system checks the disks and comes up multi-user
without intervention at the console.
Such a reboot
can be stopped (after it prints the date) with a ^C (interrupt).
This will leave the system in single-user mode, with only the console
terminal active.
It is also possible to allow the filesystem checks to complete
and then to return to single-user mode by signaling \fIfsck\fP
with a QUIT signal (^\).
.PP
If booting from the console command level is needed, then the command
.DS
\fB>>>\fP B
.DE
will boot from the default device and
ask for the name of the system to be booted.
Typing a carriage return will cause the default system (as compiled in
in section 4.1), to be booted.
In any case, the system selected will come up in single-user mode.
.PP
To bring the system up to a multi-user configuration from the single-user
all you have to do is hit ^D on the console.  The system
will then execute /etc/rc,
a multi-user restart script (and /etc/rc.local),
and come up on the terminals listed as
active in the file /etc/ttys.
See
\fIinit\fP\|(8)
and
\fIttys\fP\|(5).
Note, however, that this does not cause a file system check to be performed.
Unless the system was taken down cleanly, you should run
``fsck \-p'' or force a reboot with
\fIreboot\fP\|(8)
to have the disks checked.
.PP
To take the system down to a single user state you can use
.DS
\fB#\fP kill 1
.DE
or use the
\fIshutdown\fP\|(8)
command (which is much more polite, if there are other users logged in.)
when you are up multi-user.
Either command will kill all processes and give you a shell on the console,
as if you had just booted.  File systems remain mounted after the
system is taken single-user.  If you wish to come up multi-user again, you
should do this by:
.DS
\fB#\fP cd /
\fB#\fP /etc/umount -a
\fB#\fP ^D
.DE
.PP
Each system shutdown, crash, processor halt and reboot
is recorded in the file /usr/adm/shutdownlog
with the cause.
.NH 2
Device errors and diagnostics
.PP
When serious errors occur on peripherals or in the system, the system
prints a warning diagnostic on the console.
These messages are collected
by the system error logging process
.IR dmesg (8)
and written into a system error log file
\fI/usr/adm/messages\fP
(\fIdmesg\fP is executed periodically by cron to collect system messages
from a message buffer within the kernel.)
.PP
Error messages printed by the devices in the system are described with the
drivers for the devices in section 4 of the programmer's manual.
If errors occur suggesting hardware problems, you should contact
your hardware support group or field service.  It is a good idea to
examine the error log file regularly
(e.g. with ``tail \-r \fI/usr/adm/messages\fP'').
.NH 2
File system checks, backups and disaster recovery
.PP
Periodically (say every week or so in the absence of any problems)
and always (usually automatically) after a crash,
all the file systems should be checked for consistency
by
\fIfsck\fP\|(1).
The procedures of
\fIreboot\fP\|(8)
should be used to get the system to a state where a file system
check can be performed manually or automatically.
.PP
Dumping of the file systems should be done regularly,
since once the system is going it is easy to
become complacent.
Complete and incremental dumps are easily done with
\fIdump\fP\|(8).
You should arrange to do a towers-of-hanoi dump sequence; we tune
ours so that almost all files are dumped on two tapes and kept for at
least a week in most every case.  We take full dumps every month (and keep
these indefinitely).
.PP
More precisely, we have three sets of dump tapes: 10 daily tapes,
5 weekly sets of 2 tapes, and fresh sets of three tapes monthly.
We do daily dumps circularly on the daily tapes with sequence
`3 2 5 4 7 6 9 8 9 9 9 ...'.
Each weekly is a level 1 and the daily dump sequence level
restarts after each weekly dump.
Full dumps are level 0 and the daily sequence restarts after each full dump
also.
.PP
Thus a typical dump sequence would be:
.br
.ne 6
.KS
.TS
center;
c c c c c
n n n l l.
tape name	level number	date	opr	size
_
FULL	0	Nov 24, 1979	jkf	137K
D1	3	Nov 28, 1979	jkf	29K
D2	2	Nov 29, 1979	rrh	34K
D3	5	Nov 30, 1979	rrh	19K
D4	4	Dec 1, 1979	rrh	22K
W1	1	Dec 2, 1979	etc	40K
D5	3	Dec 4, 1979	rrh	15K
D6	2	Dec 5, 1979	jkf	25K
D7	5	Dec 6, 1979	jkf	15K
D8	4	Dec 7, 1979	rrh	19K
W2	1	Dec 9, 1979	etc	118K
D9	3	Dec 11, 1979	rrh	15K
D10	2	Dec 12, 1979	rrh	26K
D1	5	Dec 15, 1979	rrh	14K
W3	1	Dec 17, 1979	etc	71K
D2	3	Dec 18, 1979	etc	13K
FULL	0	Dec 22, 1979	etc	135K
.TE
.KE
We do weekly dumps often enough that daily dumps always fit on one tape.
.PP
Dumping of files by name is best done by
\fItar\fP\|(1)
but the amount of data that can be moved in this way is limited
to a single tape.
Finally if there are enough drives entire
disks can be copied with
\fIdd\fP\|(1)
using the raw special files and an appropriate
blocking factor; the number of sectors per track is usually
a good value to use, consult \fI/etc/disktab\fP.
.PP
It is desirable that full dumps of the root file system be
made regularly.
This is especially true when only one disk is available.
Then, if the
root file system is damaged by a hardware or software failure, you
can rebuild a workable disk doing a restore in the
same way that the initial root file system was created.
.PP
Exhaustion of user-file space is certain to occur
now and then; since quotas aren't available on \*(2B
try using the programs
\fIdu\fP\|(1),
\fIdf\fP\|(1),
\fIquot\fP\|(8),
combined with threatening
messages of the day, and personal letters.
.NH 2
Moving file system data
.PP
If you have the equipment,
the best way to move a file system
is to dump it to magtape using
\fIdump\fP\|(8),
use
\fInewfs\fP\|(8)
to create the new file system,
and restore the tape, using \fIrestor\fP\|(8).
If for some reason you don't want to use magtape,
dump accepts an argument telling where to put the dump;
you might use another disk.
Filesystems may also be moved by piping the output of a \fItar\fP\|(1)
to another \fItar\fP.
The \fIrestor\fP program accesses the raw device, laying down
inodes and blocks in the same place they came from as recorded by dump.
Care must therefore be taken when restoring a dump into
a file system smaller than the original file system.
.PP
If you have to shrink a file system or merge a file system into another,
existing one, the best bet is to use \fItar\fP\|(1).
If you
are playing with the root file system and only have one drive,
the procedure is more complicated.
If the only drive is a Winchester disk, this procedure may not be used
without overwriting the existing root or another partition.
What you do is the following:
.IP 1.
GET A SECOND PACK!!!!
.IP 2.
Dump the root file system to tape using
\fIdump\fP\|(8).
.IP 3.
Bring the system down and mount the new pack.
.IP 4.
Load the distribution tape and install the new
root file system as you did when first installing the system.
.IP 5.
Boot normally
using the newly created disk file system.
.PP
Note that if you add new disk
drivers they should also be added to the standalone system in
\fI/sys/pdpstand\fP.
If you change the disk partition tables the default disk partition tables
in \fI/etc/disktab\fP should be modified.
.NH 2
Recompiling and reinstalling system software
.PP
It is easy to regenerate the system, and it is a good
idea to try rebuilding pieces of the system to build confidence
in the procedures.
The system consists of two major parts:
the kernel itself (/sys) and the user programs
(/usr/src and subdirectories).
The major part of this is /usr/src.
.PP
The three major libraries are the C library in /usr/src/lib/libc
and the \s-2FORTRAN\s0 libraries /usr/src/usr.lib/libI77 and
/usr/src/usr.lib/libF77.  In each
case the library is remade by changing into the corresponding directory
and doing
.DS
\fB#\fP make
.DE
and then installed by
.DS
\fB#\fP make install
.DE
Similar to the system,
.DS
\fB#\fP make clean
.DE
cleans up.
.PP
The source for all other libraries is kept in subdirectories of
/usr/src/usr.lib; each has a makefile and can be recompiled by the above
recipe.
.PP
If you look at /usr/src/Makefile, you will see that
you can recompile the entire system source with one command.
To recompile a specific program, find
out where the source resides with the \fIwhereis\fP\|(1)
command, then change to that directory and remake it
with the makefile present in the directory.
For instance, to recompile ``date'', 
all one has to do is
.DS
\fB#\fP whereis date
\fBdate: /usr/src/bin/date.c /bin/date /usr/man/man1/date.1\fP
\fB#\fP cd /usr/src/bin
\fB#\fP make date
.DE
this will create an unstripped version of the binary of ``date''
in the current directory.  To install the binary image, use the
install command as in
.DS
\fB#\fP install \-s date /bin/date
.DE
The \-s option will insure the installed version of date has
its symbol table stripped.  The install command should be used
instead of mv or cp as it understands how to install programs
even when the program is currently in use.
.PP
If you wish to recompile and install all programs in a particular
target area you can override the default target by doing:
.DS
\fB#\fP make
\fB#\fP make DESTDIR=\fIpathname\fP install
.DE
.PP
To regenerate all the system source you can do
.DS
\fB#\fP cd /usr/src
\fB#\fP make
.DE
.PP
If you modify the C library, say to change a system call,
and want to rebuild and install everything from scratch you
have to be a little careful.
You must insure that the libraries are installed before the
remainder of the source, otherwise the loaded images will not
contain the new routine from the library.  The following
sequence will accomplish this.
.DS
\fB#\fP cd /usr/src
\fB#\fP make clean
\fB#\fP make build
\fB#\fP make installsrc
.DE
The first \fImake\fP removes any existing binaries in the source trees
to insure that everything is reloaded.
The next \fImake\fP compiles and installs the libraries and compilers,
then compiles the remainder of the sources.
The final line installs all of the commands not installed in the first phase.
This will take about 12 hours on a reasonably configured 11/44.
.NH 2
Making local modifications
.PP
To keep track of changes to system source we migrate changed
versions of commands in /usr/src/bin, /usr/src/usr.bin, and /usr/src/ucb
in through the directory /usr/src/new
and out of the original directory into /usr/src/old for
a time before removing them.
(/usr/new is also used by default for the programs that constitute
the contributed software portion of the distribution.)
Locally written commands that aren't distributed are kept in /usr/src/local
and their binaries are kept in /usr/local.  This allows /usr/bin, /usr/ucb,
and /bin to correspond to the distribution tape (and to the manuals that
people can buy).  People wishing to use /usr/local commands are made
aware that they aren't in the base manual.  As manual updates incorporate
these commands they are moved to /usr/ucb.
.PP
A directory, /usr/junk, to throw garbage into, as well as binary directories,
/usr/old and /usr/new, are useful.  The man command supports manual
directories such as /usr/man/mano for old and /usr/man/manl for local
to make this or something similar practical.
.NH 2
Accounting
.PP
UNIX optionally records two kinds of accounting information:
connect time accounting and process resource accounting.  The connect
time accounting information is stored in the file \fI/usr/adm/wtmp\fP, which
is summarized by the program
.IR ac (8).
The process time accounting information is stored in the file
\fI/usr/adm/acct\fP after it is enabled by
.IR accton (8),
and is analyzed and summarized by the program
.IR sa (8).
.PP
If you need to recharge for computing time, you can develop
procedures based on the information provided by these commands.
A convenient way to do this is to give commands to the clock daemon
.I /etc/cron
to be executed every day at a specified time.  This is done by adding
lines to \fI/usr/adm/crontab\fP; see
.IR cron (8)
for details.
.NH 2
Resource control
.PP
Resource control in \*(2B is more
elaborate than in previous PDP UNIX systems.
The resources consumed
by any single process can be limited by the mechanisms of
\fIsetrlimit\fP\|(2).  As distributed, the mechanism
is voluntary, though sites may choose to modify the login
mechanism to impose limits.
.NH 2
Files that need periodic attention
.PP
We conclude the discussion of system operations by listing
the files that require periodic attention or are system specific
.de BP
.IP \fB\\$1\fP
.br
..
.TS
center;
lb a.
/etc/fstab	how disk partitions are used
/etc/disktab	disk partition sizes
/etc/printcap	printer data base
/etc/gettytab	terminal type definitions
/etc/remote	names and phone numbers of remote machines for \fItip\fP(1)
/etc/group	group memberships
/etc/motd	message of the day
/etc/passwd	password file; each account has a line
/etc/rc.local	local system restart script; runs reboot; starts daemons
/etc/inetd.conf	local internet servers
/etc/hosts	host name data base
/etc/networks	network name data base
/etc/services	network services data base
/etc/hosts.equiv	hosts under same administrative control
/etc/syslog.conf	error log configuration for \fIsyslogd\fP\|(8)
/etc/ttys	enables/disables ports
/usr/lib/crontab	commands that are run periodically
/usr/lib/aliases	mail forwarding and distribution groups
/usr/adm/acct	raw process account data
/usr/adm/messages	system error log
/usr/adm/shutdownlog	log of system reboots
/usr/adm/wtmp	login session accounting
.TE

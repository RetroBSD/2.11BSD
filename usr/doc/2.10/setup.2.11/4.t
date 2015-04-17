.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)4.t	2.3 (2.11BSD GTE) 1996/11/27
.\"
.de IR
\fI\\$1\fP\|\\$2
..
.ds LH "Installing/Operating \*(2B
.nr H1 4
.nr H2 0
.ds CF \*(DY
.ds RH "System setup
.bp
.LG
.B
.ce
4. SYSTEM SETUP
.sp 2
.R
.NL
.PP
This section describes procedures used to set up a PDP-11 UNIX system.
These procedures are used when a system is first installed
or when the system configuration changes.  Procedures for normal
system operation are described in the next section.
.NH 2
Creating a UNIX boot
.PP
\fI/boot\fP uses the device information passed to it from the bootstrap
in determining the device, unit and file to load.   If an autoreboot
is being done the kernel will have passed the device information to the 
bootstrap as well as setting the autoreboot flag.
.PP
\fI/boot\fP does not require recompilation to adapt to a new autoreboot
device.
.NH 2
Kernel configuration
.PP
This section briefly describes the layout of the kernel code and
how files for devices are made.
.NH 3
Kernel organization
.PP
As distributed, the kernel source is in a 
separate tar image.  The source may be physically
located anywhere within any file system so long as
a symbolic link to the location is created for the
file /sys
(many files in /usr/include are normally symbolic links
relative to /sys).  In further discussions of the
system source all path names will be given relative to
/sys.
.PP
The directory /sys/sys
contains the mainline machine independent
operating system code.
Files within this directory are conventionally
named with the following prefixes:
.DS
.TS
lw(1.0i) l.
init_	system initialization
kern_	kernel (authentication, process management, etc.)
quota_	kernel portion of disk quota system
subr_	misc. subroutines used throughout the kernel
sys_	system calls and the like
tty_	terminal handling
ufs_	file system
uipc_	interprocess communication
vm_	memory management
.TE
.DE
.PP
The remaining directories are organized as follows:
.DS
.TS
lw(1.0i) l.
/sys/h	machine independent include files
/sys/conf	site configuration files and basic templates
/sys/net	network independent, but network related code
/sys/netinet	DARPA Internet code
/sys/netimp	IMP support code
/sys/netns	Xerox NS support code
/sys/pdp	PDP-11 specific mainline code
/sys/pdpif	PDP-11 network interface code
/sys/pdpmba	PDP-11 MASSBUS device drivers and related code
/sys/pdpuba	PDP-11 UNIBUS device drivers and related code
.TE
.DE
.PP
Many of these directories are referenced through /usr/include with
symbolic links.  For example, /usr/include/sys is a symbolic
link to /sys/h.  The system code, as distributed, is mostly
independent of the include files in /usr/include.  Unfortunately
not all references to /usr/include have been eradicated, so
compiling the system requires the /usr file system to be mounted.
.NH 3
Devices and device drivers
.PP
Devices supported by UNIX are implemented in the kernel
by drivers whose source is kept in /sys/pdp, /sys/pdpuba,
or /sys/pdpmba.  These drivers are loaded
into the system when included in a cpu specific configuration file
kept in the conf directory.  Devices are accessed through special
files in the file system, made by the
.IR mknod (8)
program and normally kept in the /dev directory.
For all the devices supported by the distribution system, the
files in /dev are created by the /dev/MAKEDEV
shell script.
.PP
Determine the set of devices that you have and create a new /dev
directory by running the MAKEDEV script.
First create a new directory
/newdev, copy MAKEDEV into it, edit the file MAKEDEV.local
to provide an entry for local needs,
and run it to generate a /newdev directory.
For instance, if your machine has a single DZ11, a single
DH11, an RM03 disk, an EMULEX UNIBUS SMD disk controller, an
AMPEX 9300 disk, and a TE16 tape drive you would do:
.DS
\fB#\fP cd /
\fB#\fP mkdir newdev
\fB#\fP cp dev/MAKEDEV newdev/MAKEDEV
\fB#\fP cd newdev
\fB#\fP MAKEDEV dz0 dh0 xp0 xp1 ht0 std LOCAL
.DE
Note the ``std'' argument causes standard devices
such as \fI/dev/console\fP, the machine console, \fI/dev/null\fP,
\fI/dev/tty\fP, \fI/dev/klog\fP, etc.
to be created.
.PP
You can then do
.DS
\fB#\fP cd /
\fB#\fP mv dev olddev ; mv newdev dev
\fB#\fP sync
.DE
to install the new device directory.\(ua
.FS
.IP \(ua
You must reboot your system before you can remove the /olddev directory.
.FE
As distributed almost all of
the device nodes are already present and you may wish to remove
unused entries from /dev to speed up scanning of the directory.
The terminal nodes are almost certainly incorrect for your site and
will need to be deleted and recreated.  Directly connected terminals
should have the softcarrier bit on in their minor device numbers.  Since
MAKEDEV by default creates terminal (dh, dz, etc) nodes with the softcarrier
bit off you will have to delete those nodes and recreate those terminal
nodes which are directly connected:
.DS
\fB#\fP cd /dev
\fB#\fP rm ttyh0
\fB#\fP mknod ttyh0 c 3 128
.DE
.NH 3
Building new system images
.PP
The kernel configuration of each UNIX system is described by
a single configuration file, stored in the \fI/sys/conf\fP directory.
The format of this file is very simple consisting of lines starting with
an \fIidentifier\fP followed by a \fIvalue\fP.  Blank lines and anything
past a ``#'' (including the #) are comments.  This file is processed by the
shell script \fIconfig\fP in the same directory.
The manual pages in section 4 of the UNIX manual specify the configuration
lines necessary for various devices.  A comprehensive list of system
options with descriptions of their meanings and effects can be found in
appendix A.
.PP
The configuration file \fIGENERIC\fP in the conf directory was used to
build the generic distribution kernel.  To build a local configuration
file, copy GENERIC to a new file \fISYSTEM\fP, edit \fISYSTEM\fP for your
local system configuration, and then type "./config \fISYSTEM\fP".  This
will create the directory \fI../SYSTEM\fP and copy specially edited files
into based on the definitions in \fISYSTEM\fP.  Change directory into the
new system directory and type "make all".\(ua,
.FS
.IP \(ua
note that non-separate systems are not currently supported
.FE
.DS
\fB#\fP cp GENERIC \fISYSTEM\fP
\fB#\fP TERM=\fIterminal_type\fP; export TERM
\fB#\fP vi \fISYSTEM\fP
\fB#\fP ./config \fISYSTEM\fP
\fB#\fP cd ../\fISYSTEM\fP
\fB#\fP make
.DE
.PP
Note that the overlay scheme in the Makefile copied into the new system
directory may fail because either the \fIbase segment\fP is too small, too
large or one or more \fIoverlay segments\fP are too large.  If this
happens the system objects will have to be re-arranged in the \fIbase\fP
and \fIoverlay\fP segments.  The comments in the Makefile should make it
fairly clear what the restrictions on object placement are in the system.
.PP
The configured system image ``unix''\(dd
.FS
.IP \(dd
on networked systems there are two images \fIunix\fP, and \fInetnix\fP.
.FE
should be
copied to the root, and then booted to try it out.
It is best to save the old kernel to a known name so as not to destroy
the working system until you're sure the new one does work.  It is an
\fBbetter\fP idea to have a non network kernel (/emergencyunix) always
kept on the system:
.DS
\fB#\fP cp /unix /oldunix
\fB#\fP make install
\fB#\fP sync
.DE
To boot the new version of the system you should follow the
bootstrap procedures outlined in section titled ``\fBBootstrap and
shutdown procedures\fP''
A systematic scheme for numbering and saving old versions
of the system may be useful.
.NH 2
Disk configuration
.PP
This section describes how to layout file systems to make use
of the available space and to balance disk load for better system
performance.
.NH 3
Disk naming and divisions
.PP
Each physical disk drive can be divided into up to 8 partitions;
UNIX typically
uses only 3 or 4 partitions.
For instance, on an RP06
the first partition, xp0a,
is used for a root file system, a backup thereof,
or a small file system like, /tmp;
the second partition, xp0b,
is used for swapping or a small file system; and
a combination of the remaining partitions (xp0d, xp0e, xp0f, xp0g, xp0h)
would hold user file systems.
.sp
.RS
.PP
\fBWarning:\fP  for disks on which DEC standard 144 bad sector forwarding
is supported, the last track and up to 126 preceding sectors
contain replacement sectors and bad sector lists.
Disk-to-disk copies should be careful to avoid overwriting this information.
See \fIbad144\fP\|(8).
Bad sector forwarding is optional in the \fBhk\fP
and \fBxp\fP drivers.
The partition sizes listed in \fI/etc/disktab\fP that \fInewfs\fP\|(8)
uses automatically reserve the maximum amount of room that may be used
by bad block forwarding on a disk.
.PP
Note also that bad144 style bad block forwarding \fIcan not\fP be used
with SI controllers on the xp driver as the controllers use their own
internal scheme for bad block forwarding, and you can in fact make your
disks unusable on the SI controllers if you write anything in the last
five cylinders.  The partition sizes in \fI/etc/disktab\fP also handle
this constraint automatically.
.PP
The generic distribution kernel does not do bad block forwarding.  There is
unfortunately no way to run bad144 style bad block forwarding on some of your
disks, but not others.  As a final bug, the hk and xp drivers do not reread
the bad sector forwarding information when disk packs are changed and so
will erroneously use bad block forwarding information from the wrong packs!
.RE
.sp
.PP
The space available on a disk varies, not surprisingly, per device.  
Disklabels make a table giving sizes meaningless since there are no
predefined partition sizes embedded in the kernel any longer.  The root
filesystem (\fBa\fP) must be at least 4Mb, preferably 6 to 7Mb if possible.
The swap area (almost always the \fBb\fP partition) should be about 3Mb or
so.  If your system has a small amount (less than 2Mb) of memory you will
need more swap space, perhaps 4 or 5Mb.  It is a rare case where more than
5 or 6Mb of swap space is required.   The system will run out of other
resources by the time enough activity is generated to need that much swap
space.
.PP
The system (boot) disk has a swapping area and
a root file system.  Other drives may use those partitions for data.
\fBRemember:\fP the \fBa\fP partition must start at sector 0 or
\fIdisklabel\fP\|(8) or else the kernel will not be able to read/write the
label.
.PP
The distributed system binaries occupy about 34 Megabytes
while the major sources occupy another 36 Megabytes.  Adding in the
miscellaneous sources, a few locate works of art bring the total for
a complete system to about 90 Megabytes.
This overflows RK07, RL02 and RM03 systems,
but fits easily on most other hardware configurations.  \*(2B is quite
happy on RD54 or larger.  Simply fitting the distribution isn't enough,
there must still be space left for user files, objects when compiling
programs, spooling directories, usw.
.PP
Be aware that the disks have their sizes measured in disk sectors (512
bytes), while the UNIX file system blocks are 1024 bytes each.  Thus if a
disk partition has 10000 sectors (disk blocks), it will have only 5000
UNIX file system blocks, and you \fImust\fP divide by 2 to use 5000 when
specifying the size to the \fImkfs\fP command for instance.  The 
\fInewfs\fP\|(8) program performs this calculation automatically.   You
should \fBnever\fP need to run \fImkfs\fP manually.
All user
programs report disk space in kilobytes and, where needed, disk sizes are
always specified in units of sectors.  The /etc/disktab file used in
making file systems specifies disk partition sizes in sectors; the
default sector size may be overridden with the ``se'' attribute.  \fBNote\fP
that the only sector size currently supported is NBPG as defined in 
\fI/sys/pdp/machparam.h\fP.  This restriction is enforced in several places
in the disklabeling process as a safeguard against specifying a sector
size other than NBPG (512).  Any other sector size would produce strange
results and almost certainly curdled filesystems.
.NH 3
Layout considerations
.PP
There are several considerations in deciding how
to adjust the arrangement of things on your disks.
The most important is making sure that there is adequate space
for what is required; secondarily, throughput should be maximized.
Swap space is an important parameter since it defines the maximum
process image load that may be run.  If, for instance, your swap
area were smaller than the amount of main memory available after
the kernel took its share, some of your memory would never
be used.
.PP
Many common system programs (C, the editor, the assembler etc.)
create intermediate files in the /tmp directory,
so the file system where this is stored also should be made
large enough to accommodate
most high-water marks; if you have several disks, it makes
sense to mount this in a ``root'' (i.e. first partition)
file system on another disk.
All the programs that create files in /tmp take
care to delete them, but are not immune to rare events
and can leave dregs.
The directory should be examined every so often and the old
files deleted.
.PP
The efficiency with which UNIX is able to use the CPU
is often strongly affected by the configuration of disk controllers.
For general time-sharing applications,
the best strategy is to try to split the most actively-used sections
among several disk arms.
.PP
It is critical for good performance to balance disk load.
There are at least five components of the disk load that you can
divide between the available disks:
.DS
1. The root file system.
2. The /tmp file system.
3. The /usr file system.
4. The user files.
5. The swapping activity.
.DE
The following possibilities are ones that have been used at times
when 2, 3 and 4 disks were available:
.TS
center doublebox;
l | c s s
l | lw(5) | lw(5) | lw(5).
	disks
what	2	3	4
_
/	0	0	0
tmp	1	2	3
usr	1	1	1
swapping\(ua	0	2	2
users	0	0+2	0+2
archive	x	x	3
.TE
.FS
.IP \(ua
Note also, that only a single swapping area is supported.  The
\fIswapon\fP\|(2) system call and multiple swapping areas have \fBnot\fP
been implemented under \*(2B (yet.  no real need since enough other resources
are exhausted by the time a 4mb 11/73 needs additional swap space).
.FE
.PP
The most important things to consider are to
even out the disk load as much as possible, and to do this by
decoupling file systems (on separate arms) between which heavy copying occurs.
Note that a long term average balanced load is not important; it is
much more important to have an instantaneously balanced
load when the system is busy.
When placing several busy file systems on the same disk,
it is helpful to group them together to minimize arm movement,
with less active file systems off to the side.
.PP
Intelligent experimentation with a few file system arrangements can
pay off in much improved performance.  It is particularly easy to
move the root, the
/tmp
file system and the swapping area.  Note, though, that the disks
containing the root and swapping area can never be removed while UNIX is
running.  Place the
user files and the
/usr
directory as space needs dictate and experiment
with the other, more easily moved file systems.
.NH 3
Implementing a layout
.PP
To put a chosen disk layout into effect, you should use the
.IR newfs (8)
command to create each new file system.
Each file system must also be added to the file
/etc/fstab
so that it will be checked and mounted when the system is bootstrapped.
.PP
As an example, consider a system with RA80's.  On the first RA80, ra0,
we will put the root file system in ra0a, and the /usr
file system in ra0c, which has enough space to hold it and then some.
The /tmp directory will be part of the root file system,
as no file system will be mounted on /tmp.
If we had only one RA80, we would put user files
in the ra0c partition with the system source and binaries.
.PP
If we had a second RA80, we would place \fI/usr\fP in ra1c.
We would put user files in ra0c, calling the file system /mnt.
We would put swap on \fIra0b\fP.
We would keep a backup copy of the root
file system in the \fBra1a\fP disk partition and
put /tmp on \fIra1b\fP.
\fI/etc/fstab\fP would then contain
.DS
/dev/ra0a:/:rw:1:1
/dev/ra0b::sw::
/dev/ra0c:/mnt:rw:1:2
/dev/ra1b:/tmp:rw::
/dev/ra1c:/usr:rw:1:2
.DE
.PP
To make the /mnt file system we would do:
.DS
\fB#\fP cd /dev
\fB#\fP MAKEDEV ra1
\fB#\fP newfs ra1c ra80
(information about file system prints out)
(to specify an alternate \fIm\fP value: newfs -m # ra1c ra80)
(where # is between 1 and 31)
\fB#\fP mkdir /mnt
\fB#\fP mount /dev/ra1c /mnt
.DE
.NH 2
Configuring terminals
.PP
If UNIX is to support simultaneous
access from directly-connected terminals other than the console,
the file \fI/etc/ttys\fP (\fIttys\fP\|(5)) must be edited.
.PP
Terminals connected via DZ11 interfaces are conventionally named \fBttyDD\fP
where DD is a decimal number, the ``minor device'' number.
The lines on dz0 are named /dev/tty00, /dev/tty01, ... /dev/tty07.
By convention, all other terminal names are of the form \fBtty\fPCX, where
C is an alphabetic character according to the type of terminal multiplexor
and its unit number,
and X is a digit for the first ten lines on the interface
and an increasing lower case letter for the rest of the lines.
C is defined for the number of interfaces of each type listed below.
Since tty structures are approximately 78 bytes each, it is
highly doubtful that more than 3 or 4 terminal interface boards
will ever be attached to a PDP-11 (especially in a BA23 cabinet).
.DS
.TS
center box;
c c c c
c c c c
l c n n.
Interface		Number of lines	Number of
Type	Characters	per board	Interfaces
_
DZ11	see above	8	10
DH11	h-o	16	8
DHU11	S-Z	16	8
pty	p-u	16	6
.TE
.DE
.PP
To add a new terminal device, be sure the device is configured into the system
and that the special files for the device have been made by /dev/MAKEDEV.
Then, enable the appropriate lines of /etc/ttys by setting the ``status''
field to \fBon\fP (or add new lines).
Note that lines in \fI/etc/ttys\fP are one-for-one with entries
in the file of current users (\fI/var/run/utmp\fP),
and therefore it is best to make changes
while running in single-user mode
and to add all of the entries for a new device at once.
.PP
The format of the /etc/ttys file is the same in \*(2B as in \*(Ps and 4.3BSD.
Each line in the file is broken into four tab separated
fields (comments are shown by a `#' character and extend to
the end of the line).  For each terminal line the four fields
are:
the device (without a leading /dev),
the program /etc/init should startup to service the line
(or \fBnone\fP if the line is to be left alone),
the terminal type (found in /etc/termcap),
and optional status information describing if the terminal is
enabled or not and if it is ``secure'' (i.e. the super user should
be allowed to login on the line).  All fields are character strings
with entries requiring embedded white space enclosed in double
quotes.
Thus a newly added terminal /dev/tty00 could be added as
.DS
tty00 	"/usr/libexec/getty std.9600"	vt100	on secure	# Steve's office
.DE
The std.9600 parameter provided
to /usr/libexec/getty is used in searching the file /etc/gettytab; it specifies
a terminal's characteristics (such as baud rate).
To make custom terminal types, consult 
.IR gettytab (5)
before modifying /etc/gettytab.
.PP
Dialup terminals should be wired so that carrier is asserted only when the
phone line is dialed up.
For non-dialup terminals from which modem control
is not available, you must either wire back the signals so that
the carrier appears to always be present, or show in the minor device number
that carrier is to be assumed to be present
by adding 128 decimal to the minor device number when creating the
device node.  This differs from 4.3BSD where the softcarrier state is
specified at kernel configuration time.
.PP
For network terminals (i.e. pseudo terminals), no program should
be started up on the lines.  Thus, the normal entry in /etc/ttys
would look like
.DS
ttyp0 	none	network
.DE
(Note the fourth field is not needed here.)
.PP
When the system is running multi-user, all terminals that are listed
in /etc/ttys as \fBon\fP have their line are enabled.
If, during normal operations, it is desired
to disable a terminal line, you can edit the file
/etc/ttys
to change the terminal's status to \fBoff\fP and
then send a hangup signal to the \fIinit\fP process, by doing
.DS
\fB#\fP kill \-1 1
.DE
Terminals can similarly be enabled by changing the status field
from \fBoff\fP to \fBon\fP and sending a hangup signal to \fIinit\fP.
.PP
Note that if a special file is inaccessible when \fIinit\fP tries
to create a process for it, init will log a message to the
system error logging process (/usr/sbin/syslogd)
and try to reopen the terminal every minute, reprinting the warning
message every 10 minutes.  Messages of this sort are normally
printed on the console, though other actions may occur depending
on the configuration information found in /etc/syslog.conf.
.PP
Finally note that you should change the names of any dialup
terminals to ttyd?
where ? is in [0-9a-zA-Z], as some programs use this property of the
names to determine if a terminal is a dialup.
Shell commands to do this should be put in the /dev/MAKEDEV.local
script.
.NH 2
Adding users
.PP
New users can be added to the system by adding a line to the
password file
/etc/passwd.
The procedure for adding a new user is described in
.IR adduser (8).
.PP
You should add accounts for the initial user community, giving
each a directory and a password, and putting users who will wish
to share software in the same groups.
.PP
Several guest accounts have been provided on the distribution
system; these accounts are for people at Berkeley, 
Bell Laboratories, and others
who have done major work on UNIX in the past.  You can delete these accounts,
or leave them on the system if you expect that these people would have
occasion to login as guests on your system.
.NH 2
Site tailoring
.PP
All programs that require the site's name, or some similar
characteristic, obtain the information through system calls
or from files located in /etc.  Aside from parts of the
system related to the network, to tailor the system to your
site you must simply select a site name, then edit the file
.DS
/etc/netstart
.DE
At or about line 25 in /etc/netstart you should find a line similar to:
.DS
/bin/hostname \fImyname.my.domain\fP
.DE
defines the value returned by the 
.IR gethostname (2)
system call.  Your hostname should be your fully qualified 
domain name.  Programs such as
.IR getty (8),
.IR mail (1),
.IR wall (1),
.IR uucp (1),
and
.IR who (1)
use this system call so that the binary images are site
independent.
.NH 2
Setting up the mail system
.PP
The mail system consists of the following commands:
.DS
.TS
l l.
/bin/mail	old standard mail program, \fIbinmail\fP\|(1)
/usr/ucb/mail	UCB mail program, described in \fImail\fP\|(1)
/usr/sbin/sendmail	mail routing program
/usr/spool/mail	mail spooling directory
/etc/aliases	mail forwarding information
/usr/bin/newaliases	command to rebuild binary forwarding database
/usr/ucb/biff	mail notification enabler\(ua
/usr/libexec/comsat	mail notification daemon\(ua
.TE
.DE
Mail is normally sent and received using the
.IR mail (1)
command, which provides a front-end to edit the messages sent
and received, and passes the messages to
.IR sendmail (8)
for routing.
The routing algorithm uses knowledge of the network name syntax,
aliasing and forwarding information, and network topology, as
defined in the configuration file /etc/sendmail.cf, to
process each piece of mail.
Local mail is delivered by giving it to the program /bin/mail
that adds it to the mailboxes in the directory /usr/spool/mail/\fIusername\fP,
using a locking protocol to avoid problems with simultaneous updates.
After the mail is delivered, the local mail delivery daemon /usr/libexec/comsat
is notified, which in turn notifies
users who have issued a ``\fIbiff\fP y'' command that mail has arrived\(ua.
.FS
.IP \(ua
comsat and biff are only available under systems configured
for networking support.
.FE
.PP
To set up the mail facility you should read the instructions in the
file READ_ME in the directory /usr/src/usr.lib/sendmail and then adjust
the necessary configuration files.
You should also set up the file /etc/aliases for your installation,
creating mail groups as appropriate.  Documents describing 
.IR sendmail 's
operation and installation are also included on the distribution tape.
.NH 3
Setting up a UUCP connection
.PP
The version of \fIuucp\fP included in \*(2B is an
enhanced version of the one originally distributed with 32/V\(ua.
.FS
.IP \(ua
The \fIuucp\fP included in this distribution is the result
of work by many people; we gratefully acknowledge their
contributions, but refrain from mentioning names in the 
interest of keeping this document current.
.FE
The enhancements include:
.IP \(bu 3
support for many auto call units and dialers
in addition to the DEC DN11,
.IP \(bu 3
breakup of the spooling area into multiple subdirectories,
.IP \(bu 3
addition of an \fIL.cmds\fP file to control the set
of commands that may be executed by a remote site,
.IP \(bu 3
enhanced ``expect-send'' sequence capabilities when
logging in to a remote site,
.IP \(bu 3
new commands to be used in polling sites and
obtaining snap shots of \fIuucp\fP activity,
.IP \(bu 3
additional protocols for different communication media.
.LP
This section gives a brief overview of \fIuucp\fP
and points out the most important steps in its installation.
.PP
To connect two UNIX machines with a \fIuucp\fP network link using modems,
one site must have an automatic call unit
and the other must have a dialup port.
It is better if both sites have both.
.PP
You should first read the paper in the UNIX System Manager's Manual:
``Uucp Implementation Description''.
It describes in detail the file formats and conventions,
and will give you a little context.
In addition,
the document ``setup.tblms'',
located in the directory /usr/src/usr.bin/uucp/UUAIDS,
may be of use in tailoring the software to your needs.
.PP
The \fIuucp\fP support is located in three major directories:
/usr/bin,
/etc/uucp,
and /usr/spool/uucp.
User commands are kept in /usr/bin,
operational commands in /etc/uucp,
and /usr/spool/uucp is used as a spooling area.
The commands in /usr/bin are:
.DS
.TS
l l.
/usr/bin/uucp	file-copy command
/usr/bin/uux	remote execution command
/usr/bin/uusend	binary file transfer using mail
/usr/bin/uuencode	binary file encoder (for \fIuusend\fP)
/usr/bin/uudecode	binary file decoder (for \fIuusend\fP)
/usr/bin/uulog	scans session log files
/usr/bin/uusnap	gives a snap-shot of \fIuucp\fP activity
/usr/bin/uupoll	polls remote system until an answer is received
/usr/bin/uuname	prints a list of known uucp hosts
/usr/bin/uuq	gives information about the queue
.TE
.DE
The important files and commands in /etc/uucp are:
.DS
.TS
l l.
/etc/uucp/L-devices	list of dialers and hard-wired lines
/etc/uucp/L-dialcodes	dialcode abbreviations
/etc/uucp/L.aliases	hostname aliases
/etc/uucp/L.cmds	commands remote sites may execute
/etc/uucp/L.sys	systems to communicate with, how to connect, and when
/etc/uucp/SEQF	sequence numbering control file
/etc/uucp/USERFILE	remote site pathname access specifications
/usr/sbin/uucico	\fIuucp\fP protocol daemon
/etc/uucp/uuclean	cleans up garbage files in spool area
/usr/libexec/uuxqt	\fIuucp\fP remote execution server
.TE
.DE
while the spooling area contains the following important files and directories:
.DS
.TS
l l.
/usr/spool/uucp/C.	directory for command, ``C.'' files
/usr/spool/uucp/D.	directory for data, ``D.'', files
/usr/spool/uucp/X.	directory for command execution, ``X.'', files
/usr/spool/uucp/D.\fImachine\fP	directory for local ``D.'' files
/usr/spool/uucp/D.\fImachine\fPX	directory for local ``X.'' files
/usr/spool/uucp/TM.	directory for temporary, ``TM.'', files
/usr/spool/uucp/LOGFILE	log file of \fIuucp\fP activity
/usr/spool/uucp/SYSLOG	log file of \fIuucp\fP file transfers
.TE
.DE
.PP
To install \fIuucp\fP on your system,
start by selecting a site name.
A \fIuucp\fP account must be created in the password file and a password set up.
Then,
create the appropriate spooling directories with mode 755
and owned by user \fIuucp\fP, group \fIdaemon\fP.
.PP
If you have an auto-call unit,
the L.sys, L-dialcodes, and L-devices files should be created.
The L.sys file should contain
the phone numbers and login sequences
required to establish a connection with a \fIuucp\fP daemon on another machine.
For example, my L.sys file looks something like:
.sp
.DS
elisa Any ACU 1200 7064297  "" \\\\r\\\\c ogin-EOT-ogin-\\\\r\\\\c-ogin xelisa assword: XXX
etn-ra Any ACU 1200 8891237  "" \\\\r\\\\c ogin nuucp assword XXX
anagld Never ACU 2400 8894517 name:-EOT-name: uucp assword: XXX
.DE
.sp
The first field is the name of a site,
the second shows when the machine may be called,
the third field specifies how the host is connected
(through an ACU, a hard-wired line, etc.),
then comes the phone number to use in connecting through an auto-call unit,
and finally a login sequence.
The phone number
may contain common abbreviations that are defined in the L-dialcodes file.
The device specification should refer to devices
specified in the L-devices file.
Listing only ACU causes the \fIuucp\fP daemon, \fIuucico\fP,
to search for any available auto-call unit in L-devices.
Our L-dialcodes file is of the form:
.DS
ny	1-315-
nj	1-201-
bostn	1-617-
.DE
while our L-devices file is:
.DS
ACU cul0 unused 1200 ventel
.DE
Refer to the README file in the \fIuucp\fP source directory
for more information about installation.
.PP
As \fIuucp\fP operates it creates (and removes) many small
files in the directories underneath /usr/spool/uucp.
Sometimes files are left undeleted;
these are most easily purged with the \fIuuclean\fP program.
The log files can grow without bound unless trimmed back;
\fIuulog\fP maintains these files.
Many useful aids in maintaining your \fIuucp\fP installation
are included in a subdirectory UUAIDS beneath /usr/src/usr.bin/uucp.
Peruse this directory and read the ``setup'' instructions also located there.

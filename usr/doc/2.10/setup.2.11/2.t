.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)2.t	1.10 (GTE) 1997/8/11
.\"
.ds lq ``
.ds rq ''
.ds LH "Installing/Operating \*(2B
.ds RH Bootstrapping
.ds CF \*(DY
.bp
.nr H1 2
.nr H2 0
.bp
.LG
.B
.ce
2. BOOTSTRAP PROCEDURE
.sp 2
.R
.NL
.PP
This section explains the bootstrap procedure that can be used
to get the kernel supplied with this distribution running on your machine.
It is mandatory to do a full bootstrap since the filesystem has changed
from \*(1B to \*(2B.
.PP
The safest route is to use \fItar\fP\|(1) to dump all of your current
file systems, do a full bootstrap of \*(2B and then restore user files
from the backups.
There is also an untested version of \fI512restor\fP\|(8) available for
V7 sites that need to read old dump tapes.
.PP
It is also desirable to make a convenient copy of system configuration
files for use as guides when setting up the new system; the list of files
to save from earlier PDP-11 UNIX systems, found in chapter 3, may be used
as a guideline.
.PP
\*(2B \fIrestor\fP\|(8) is able to read and automatically convert to the
new on disk directory format
\fIdump\fP\|(8) tapes made under 2.9BSD, \*(Ps and \*(1B.
.NH 2
Booting from tape
.PP
The tape bootstrap procedure used to create a
working system involves the following major
steps:
.IP 1)
Load the tape bootstrap monitor.
.IP 2)
Create the partition tables on the disk using \fIdisklabel\fP.
.IP 3)
Create a UNIX ``root'' file system system on disk using \fImkfs\fP\|(8).
.IP 4)
Restore the full root file system using \fIrestor\fP\|(8).
.IP 5)
Boot the UNIX system on the new root file system and copy the
appropriate \fIsector 0 boot block\fP to your boot device.
.IP 6)
Build and restore the /usr file system from tape
with \fItar\fP\|(1).
.IP 7)
Restore the include and kernel sources from tape.
.IP 8)
Extract the remaining source from the second tape.
.IP 9)
Tailor a version of UNIX to your specific hardware (see section 4.2).
.PP
Certain of these steps are dependent on your hardware
configuration.  If your disks require formatting, standard DEC
diagnostic utilities will have to be used, they are not supplied on the
\*(2B distribution tape.
.NH 3
Step 1: loading the tape bootstrap monitor
.PP
To load the tape bootstrap monitor, first
mount the magnetic tape on drive 0 at load point, making
sure that the write ring is not inserted.
Then use the normal bootstrap ROM, console monitor or other
bootstrap to boot from the tape.
.PP
NOTE: The boot blocks expect the CSR of the booting controller in r0
and the unit number in r1.  \fBboot\fP may be booted from any controller
or unit, the earlier restrictions of controller 0 and unit 0 have been
lifted.
.PP
If no other means are available, the following code can be keyed in
and executed at (say) 0100000 to boot from a TM tape drive (the magic number
172526 is the address of the TM-11 current memory address register;
an adjustment may be necessary if your controller is at a nonstandard
address):
.DS
.TS
l l.
012700	(mov $unit, r0)
000000	(normally unit 0)
012701	(mov $172526, r1)
172526
010141	(mov r1, -(r1))
012741	(mov $60003, -(r1))
060003	(if unit 1 use 060403, etc)
000777	(br .)
.TE
.DE
A toggle-in routine which has been used with a TS tape drive (this should
be entered at 01000):
.DS
.TS
l l.
012700	mov $unit,r0
000000
012701	mov $172522,r1
172522
005011	clr (r1)
105711	1b:tstb (r1)
100376	bpl 1b
012761	mov $setchr,-2(r1)
001040
177776
105711	2b:tstb (r1)
100376	bpl 2b
012761	mov $read,-2(r1)
001060
177776
000000	halt
140004	setchr: TS_ACK|TS_CVC|TS_SETCHR
001050	char
000000	high order address
000010	number of bytes
001070	char: status
000000
000016
000000
140001	read: TS_ACK|TS_CVC|TS_READ
000000	low order of address
000000	high order of address
001000	number of bytes to read
000000	status:
.TE
.DE
When this is executed, the first block of the tape will be read into memory.
Halt the CPU and restart at location 0.  The register \fBr1\fP \fBMUST\fP
be left pointing at the device \fIcsr\fP.  For the default/first TM or TS
this is 0172522.  The register \fBr0\fP \fBMUST\fP contain the unit number
(usually 0).
.PP
The console should type
.DS
.B
\fInn\fPBoot from \fIxx\fP(\fIctlr\fP,\fIdrive\fP,\fIpart\fP) at \fIcsr\fP
:
.R
.DE
where \fInn\fP is the CPU type on which it believes it is running.
The value will be one of 23, 24, 40, 44, 45, 53, 60, 70, 73, 83, 84, 93
or 94 depending whether
separate instruction and data (separate I/D) and/or a UNIBUS map are detected.
For KDJ-11 systems the System Maintenance Register is examined to determine
the cpu type.  At present \*(2B runs on the 44, 53, 70, 73, 83, 84, 93 and 94
\fBonly\fP.  It must be emphasized that \*(2B requires separate I/D.
.sp
\fIctlr\fP is the controller number that \fBBoot\fP
was loaded from.  It is 0 unless booting from a non-standard CSR.
.sp
\fIdrive\fP is the drive unit number.
.sp
The \fIpart\fP
number is disk partition or tapefile number booted from.  This will always
be 0 for the tape \fBBoot\fP program.
.sp
\fIcsr\fP is an octal number telling the CSR of the controller from which
\fBBoot\fP was loaded.
.sp
.PP
You are now talking to the tape bootstrap monitor.
At any point in the following procedure you can return
to this section, reload the tape bootstrap, and restart.
Through the rest of this section,
substitute the correct disk type for \fIdk\fP
and the tape type for \fItp\fP.
.NH 3
Step 2: creating the disk label
.PP
The standalone \fIdisklabel\fP program is then run:
.DS
.TS
lw(1.5i) l.
\fB:\|\fP\fItp\|\fP(0,1)	(\fIdisklabel\fP is tape file 1)
\fBBoot: bootdev=0nnnn bootcsr=0mmmmmm\fP
\fBdisklabel\fP
\fBDisk?\fP \fIdk\|\fP(0,0)	(drive 0, partition 0)
d(isplay) D(efault) m(odify) w(rite) q(uit)?
 ...
\fB:\fP	(back at tape boot level)
.TE
.DE
The \fIdisklabel\fP program is meant to be fairly intuitive.  When prompted
with a line of choices entering the key just before the left parenthesis
selects the entry.  
.PP
If there is an existing label present on \fIdk\fP\|(0,0) it will be 
used as the default.  To have \fIdisklabel\fP create a new default based
on its idea of what the drive is select \fBD\fP.  Then enter \fBm\fP to
modify/edit the label.
.PP
The MSCP driver is quite good at identifying drives because it can query the
controller.  Other drivers (notably the SMD (\fBxp\fP) driver) have to
deal with a much wider range of controllers which do not all have the
same capabilities for drive identification.  When dealing with SMD
drives you must know the geometry of the drive so you can verify  and
correct \fIdisklabel\fP's choices.
.PP
You can however, if using non-DEC SMD controllers, make things easy for
\fIdisklabel\fP to determine what type of drive is being used.  If your
controller offers the choice of RM02 emulation you should select that choice.
The standalone \fBxp\fP driver uses RM02 as the indication that drive 
identification capabilities
not offered by DEC controllers are present.  The driver will be able to
determine the geometry of the drive in this case.   This is \fBoptional\fP
because you can explicitly specify all of the parameters to the standalone
\fIdisklabel\fP program.
.PP
A full description of the standalone \fIdisklabel\fP program is in Appendix
B of this document.
.NH 3
Step 3: creating a UNIX ``root'' file system
.PP
Now create the root file system using the following procedure.\(ua
.FS
.IP \(ua
\fBNote:\fP These instructions have changed quite a bit during the
evolution of the system from \*(1B.  Previously,
if the disk on which you are creating a root file system was an \fBxp\fP
disk you would have been asked to check the drive type register and possibly
halt the processor to patch a location (hopefully before the driver 
accessed the drive).  \fBThis is no longer needed\fP.
All geometry and partition information is obtained from the disklabel
created in step 2.
We also used to give tables of \fBm\fP and \fBn\fP values for various
disks, which are now purposely omitted.
.FE
.PP
The size of the root ('a') filesystem was assigned in step 2 (creating the 
disk label).  \fImkfs\fP will not allow a filesystem to be created if there
is not a label present or if the partition size is 0.  \fImkfs\fP 
looks at partition 0 ('a') in the disklabel for the root file system size.
.PP
Finally, determine the proper interleaving factors \fIm\fP and \fIn\fP
for your disk.  Extensive testing has demonstrated
that the choice of \fIm\fP is non critical (performance of a file system
varying only by 3 to 4% for a wide range of \fIm\fP values).  Values for
\fIm\fP within the range from  2 to 5 give almost identical performance.
Increasing \fIm\fP too much actually causes degraded performance because
the free blocks are too far apart.  Slower processors (such as the 73 and
44) may want to start with a \fIm\fP of 3 or 4, faster processors (such as the
70 and 84) may start with a \fIm\fP of 2 or 3.
On the other
hand, the \fIn\fP value is moderately important.  It should be the number
of filesystem blocks contained by one cylinder of the disk, calculated
by dividing the number of sectors per cylinder by 2, rounding down if
needed.  (This is what \fImkfs\fP does by default, based on the
geometry information in the disk label.)
These numbers determine the layout of the free list that will be constructed;
the proper interleaving will help increase the speed of the file system.
.PP
The number of bytes per inode determines how many inodes will be allocated
in the filesystem.  The default of 4096 bytes per inode is normally enough
(resulting in about 2000 inodes for a 8mb root filesystem and 1000 inodes
for the 4mb distribution ``generic'' root filesystem).  If more inodes are
desired then a lower value (perhaps 3072) should be specified when prompted
for the number of bytes per inode.
.PP
Then run the standalone version of the \fImkfs\fP (8) program.
The values in square brackets at the size prompt is the default from 
the disklabel.  Simply hit a return to accept the default.  \fImkfs\fP
will allow you to create a smaller filesystem but you can not enter a
larger number than the one in brackets.
In the following procedure, substitute the correct types
for \fItp\fP and \fIdk\fP and the size determined above for \fIsize\fP:
.DS
.TS
lw(1.5i) l.
\fB:\|\fP\fItp\|\fP(0,2)	(\fImkfs\fP is tape file 2)
\fBBoot: bootdev=0nnnn bootcsr=0mmmmmm\fP
\fBMkfs\fP
\fBfile system:\fP \fIdk\|\fP(0,0)	(root is the first file system on drive 0)
\fBfile system size:\fP [NNNN] \fIsize\fP	(count of 1024 byte blocks in root)
\fBbytes per inode:\fP [4096] \fIbytes\fP	(number of bytes per inode)
\fBinterleaving factor (m, 2 default):\fP \fIm\fP	(interleaving, see above)
\fBinterleaving modulus (n, 127 default):\fP \fIn\fP	(interleaving, see above)
\fBisize = XX\fP	(count of inodes in root file system)
\fBm/n = \fP\fIm n\fP	(interleave parameters)
\fBExit called\fP
\fInn\fP\fBBoot\fP
\fB:\fP	(back at tape boot level)
.TE
.DE
.sp
The number \fBnnnn\fP is the device number of the device (high byte is the
major device number and the low byte is the unit number).  The \fBmmmmmm\fP
number is the CSR of the device.  This information is mainly used as 
a reminder and diagnostic/testing purposes.
.sp
You now have an empty UNIX root file system.
.NH 3
Step 4: restoring the root file system
.PP
To restore the root file system onto it, type
.DS
.TS
lw(1.5i) l.
\fB:\|\fP\fItp\|\fP(0,3)	(\fIrestor\fP is tape file 3)
\fBBoot: bootdev=0nnnn bootcsr=0mmmmmm\fP
\fBRestor\fP
\fBTape?\fP \fItp\|\fP(0,5)	(root \fIdump\fP is tape file 5)
\fBDisk?\fP \fIdk\|\fP(0,0)	(into root file system)
\fBLast chance before scribbling on disk.\fP	(type a carriage return to start)
.B
"End of tape"	\fR(appears on same line as message above)\fP
Exit called
\fInn\fPBoot
\fB:\fR	(back at tape boot level)
.R
.TE
.sp
This takes about 8 minutes with a TZ30 on a 11/93 and about 15 minutes using
a TK50 on a 11/73.
.DE
If you wish, you may use the \fIicheck\fP program on the tape,
\fItp\|\fP(0,4), to check the consistency of the file system you have just
installed.  This has rarely been useful and is mostly for the voyeuristic.
.NH 3
Step 5: booting UNIX
.PP
You are now ready to boot from disk.
Type:
.DS
.TS
lw(1.5i) l.
\fB:\fP\fIdk\|\fP(0,0)unix	(bring in unix from the root system)
\fBBoot: bootdev=0nnnn bootcsr=0mmmmmm\fP
.TE
.DE
The standalone boot program will then load unix from
the root file system you just created, and the system should boot:
.DS
.B
.\"CHECK
\*(2B BSD UNIX #1: Sat Jul 4 01:33:03 PDT 1992
    root@wlonex.iipo.gtegsc.com:/usr/src/sys/GENERIC
phys mem  = \fI???\fP
avail mem = \fI???\fP
user mem  = \fI???\fP

configure system
\fI\&... information about available devices ...\fP
.R
(Information about various devices will print;
most of them will probably not be found until
the addresses are set below.)
.B
erase=^?, kill=^U, intr=^C
#
.R
.DE
.PP
UNIX itself then runs for the first time and begins by printing out a banner
identifying the release and
version of the system that is in use and the date that it was compiled.  
.PP
Next the
.I mem
messages give the amount of real (physical) memory, the amount of memory
left over after the system has allocated various data structures, and the
amount of memory available to user programs in bytes.
.PP
The information about different devices being attached or not being found
is produced by the \fIautoconfig\fP\|(8) program.  Most of this is not
important for the moment, but later the device table, \fI/etc/dtab\fP,
can be edited to correspond to your hardware.  However, the tape drive of
the correct type should have been detected and attached.
.PP
The \*(lqerase ...\*(rq message is part of /.profile
that was executed by the root shell when it started.  This message
is present to remind you that the character erase,
line erase, and interrupt characters are set to what is
standard for DEC systems; this insures that things are
consistent with the DEC console interface characters.
.PP
UNIX is now running single user on the installed root file system,
and the `UNIX Programmer's Manual' applies.
The next section tells how to complete
the installation of distributed software on the /usr file system.
The `#' is the prompt from the shell,
and lets you know that you are the super-user,
whose login name is \*(lqroot\*(rq.
.PP
The disk with the new root file system on it will not be bootable
directly until the block 0 bootstrap program for your disk has been installed.
There are copies of the bootstraps in /mdec.
Use \fIdd\fP\|(1) to copy the right boot block onto block 0 of the disk.
.DS
\fB#\fP dd if=/mdec/\fIboot\fP of=/dev/r\fIdk\^\fP0a count=1
.DE
Block zero bootstraps and the devices they support are:
.DS
.TS
l l l.
boot	driver	devices
_
hkuboot	hk	RK06/07
rauboot	ra	All RA, RD, RZ, RX (except RX01,02) and RC25 drives
rkuboot	rk	RK05
rluboot	rl	RL01/02
si95uboot	si	SI 9500, CDC 9766
dvhpuboot	xp	Diva Comp V, Ampex 9300
hpuboot	xp	RP04/05/06
rm03uboot	xp	RM03
rm05uboot	xp	RM05 or SI 9500, CDC 9766
si51uboot	xp	SI 6100, Fujitsu Eagle 2351A
si94uboot	xp	Emulex SC01B/SC03B or SI 9400, Fujitsu 160
.TE
.DE
.B NOTE:
If none of the above are correct (most likely with a SMD drive with differing
geometry) then you will have to use a tape/floppy boot proceedure rather than
a sector 0 bootblock.  This can be fixed by creating a customized sector 0
boot program once the system sources have been loaded.
.PP
Once this is done, booting from this disk will load and execute the block
0 bootstrap, which will in turn load /boot.  \fB/boot\fP will print
on the console:
.DS
.TS
lw(1.5i) l.
.B
\fInn\fPBoot from \fIdk\fP(\fIctlr\fP,\fIunit\fP,\fIpart\fP) at \fIcsr\fP\fR
:
.R
.TE
.DE
The bootblock automatically loads and runs /\fIboot\fP for you;
if /\fIboot\fP is not found, the system will hang/loop forever.
The block 0 program is very small (has to fit in 512 bytes) and simple 
program, however, and can only
boot the second-stage boot from the first file system.
Once /boot is running and prints its ``: '' prompt,
boot unix as above.
.PP
As distributed /\fIboot\fP will load \fIdk\fP(0,0)unix by default if a
carriage return is typed at the \fB:\fP prompt.
.PP
\fBNOTE:\fP NONE the primary bootstraps have a prompt or alternate program
name capability because of space considerations.  No diagnostic message
results if the file cannot be found.
.NH 3
Step 6: setting up the /usr file system
.PP
First set a shell variable to the name of your disk, so
the commands used later will work regardless of the disk you
have; do one of the following:
.DS
.TS
l l.
\fB#\fP disk=hk	(if you have RK06's or RK07's)
\fB#\fP disk=rl	(if you have RL01's or RL02's)
\fB#\fP disk=ra	(if you have an MSCP drive)
\fB#\fP disk=xp	(if you have an RP06, RM03, RM05, or other SMD drive)
.TE
.DE
.PP
The next thing to do is to extract the rest of the data from the tape.
You might wish to review the disk configuration information in section
4.3 before continuing; you will have
to select a partition to restore the /usr file system into which is
at least \fB25\fP Megabytes in size (this is just barely enough for
the system binaries and such and leaves no room for the system
source.)\(ua
.FS
.IP \(ua
\fBNote:\fP Previously a lengthy table of partition names organized by
specific disk type was given.
With the introduction of disklabels this is no longer
necessary (or possible since each site can select whatever partitioning
scheme they desire).  In step 2 (creating the disklabel) a partition should
have been created for \fI/usr\fP.  If this was not done then it may be easier
to perform step 2 now than to use the more complex \fIdisklabel\fP\|(8)
program and \fIed\fP\|(1).
.FE
.PP
In the command below \fIpart\fP is the partition name (a-h) for the partition
which will hold /usr.
.DS
.TS
l l.
\fBname=${disk}0${part}
.TE
.DE
Next, find the tape you have in the following table and execute the
commands in the right hand portion of the table:
.DS
.TS
l l.
DEC TM02/03, TE16/TU45/TU77	\fB#\fP cd /dev; rm *mt*; ./MAKEDEV ht0; sync
DEC TS11, TK25/TU80/TS05	\fB#\fP cd /dev; rm *mt*; ./MAKEDEV ts0; sync
DEC TM11, TU10/TE10/TS03	\fB#\fP cd /dev; rm *mt*; ./MAKEDEV tm0; sync
DEC TMSCP, TK50/TZ30/TU81	\fB#\fP cd /dev; rm *mt*; ./MAKEDEV tu0; sync
EMULEX TC11	\fB#\fP cd /dev; rm *mt*; ./MAKEDEV tm0; sync
.TE
.DE
Then execute the following commands:
.br
.ne 5
.sp
.DS
.TS
lw(2i) l.
\fB#\fP date \fIyymmddhhmm\fP	(set date, see \fIdate\fP\|(1))
\&....
\fB#\fP passwd root	(set password for super-user)
\fBNew password:\fP	(password will not echo)
\fBRetype new password:\fP
\fB#\fP hostname \fImysitename\fP	(set your hostname)
\fB#\fP newfs ${name} 	(create empty user file system)
(this takes a minute)
\fB#\fP mount /dev/${name} /usr	(mount the usr file system)
\fB#\fP cd /usr	(make /usr the current directory)
\fB#\fP mt rew
\fB#\fP mt fsf 6
\fB#\fP tar xpbf 20 /dev/rmt12 	(extract all of usr except usr/src)
(this takes about 15-20 minutes except for the
TK50 and TZ30 which are \fBmuch\fP slower)
.TE
.DE
The data on the seventh tape file has now been extracted.
All that remains on the first tape is a small archive containing
source for the kernel and include files.
.PP
If you have an existing/old password file to be merged back into
\*(2B, special steps are necessary to convert the old password file
to the shadow password file format (shadow password file 
and password aging were ported from 4.3BSD and are standard in \*(2B ).
.DS
.TS
lw(2i) l.
\fB#\fP mt -f /dev/rmt12 fsf	(position tape at beginning of next tape file)
\fB#\fP mkdir src	(make directory for source)
\fB#\fP cd src	(make /usr/src the current directory)
\fB#\fP tar xpbf 20 /dev/rmt12 	(extract the system and include source)
(this takes about 5-10 minutes)
\fB#\fP cd /	(back to root)
\fB#\fP chmod 755  /  /usr  /usr/src /usr/src/sys
\fB#\fP rm \-f sys
\fB#\fP ln \-s usr/src/sys sys	(make a symbolic link to the system source)
\fB#\fP umount /dev/${name}	(unmount /usr)
.TE
.DE
.PP
The first tape has been been completely loaded.
You can check the consistency of the /usr file system by doing
.DS
\fB#\fP fsck /dev/r${name}
.DE
The output from
.I fsck
should look something like:
.DS
.B
** /dev/r\fIxx\fP0g
File System: /usr

NEED SCRATCH FILE (179 BLKS)
ENTER FILENAME:  /tmp/xxx
** Last Mounted on /usr
** Phase 1 - Check Blocks and Sizes
** Phase 2 - Check Pathnames
** Phase 3 - Check Connectivity
** Phase 4 - Check Reference Counts
** Phase 5 - Check Free List
671 files, 3497 used, 137067 free
.R
.DE
.PP
If there are inconsistencies in the file system, you may be prompted
to apply corrective action; see the document describing
.I fsck
for information.
.PP
To use the /usr file system, you should now remount it by
saying
.DS
\fB#\fP mount /dev/${name} /usr
.DE
.NH 3
Step 7: extracting remaining source from the second tape
.PP
You can then extract the source code for the commands from the
second distribution tape\(ua
.FS
.IP \(ua
On the TK50 the remaining source is the 9th file on the cartridge.
.FE
(with the exception of RK07's, RM03's, and RD52's and other small disks
this will fit in the /usr file system):
.DS
\fB#\fP cd /usr/src
\fB#\fP tar xpb 20
.DE
If you get an error at this point, most likely it was
a problem with tape positioning.  Rewind the tape and
use the \fBmt\fP command to skip files, then retry the
\fBtar\fP command.
.NH 2
Additional conversion information
.PP
After setting up the new \*(2B filesystems, you may restore the user
files that were saved on tape before beginning the conversion.  Note that
the \*(2B \fIrestor\fP program does its work by accessing the raw file
system device and depositing inodes in the appropriate locations on
disk.  This means that file system dumps might not restore correctly if
the characteristics of the file system have changed (eg. if you're
restoring a dump of a file system into a file system smaller than the
original.)  To restore a dump tape for, say, the /u file system something
like the following would be used:
.DS
\fB#\fP restor r /dev/rxp1e
.DE
.PP
If \fItar\fP images were written instead of doing a dump, you should
be sure to use the `p' option when reading the files back.
No matter how you restore a file system, be sure and check its
integrity with \fIfsck\fP when the job is complete.
.PP
\fItar\fP tapes are preferred (when possible) because the inode
allocation is performed by the kernel rather than the \fIrestor\fP\|(8)
program.  This has the benefit of allocating inodes sequentially starting
from the beginning of the inode portion of the filesystem rather than
preserving the fragmented/randomized order of the old filesystem.

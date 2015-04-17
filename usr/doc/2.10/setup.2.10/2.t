.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)2.t	6.2 (Berkeley) 10/1/88
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
If you are not currently running \*(Ps it's easiest
to do a full bootstrap since \*(Ps has changed so much from all
previous versions of UNIX on the PDP but for the really dedicated
chapter 3 attempts to describe how to upgrade an existing 2.8BSD or
2.9BSD UNIX system running the \fI1K file system\fP.
Chapter 3 also describes how to upgrade an existing \*(Ps system.
An understanding of the operations used in a full bootstrap
is very helpful in performing an upgrade as well.
In either case, it is highly desirable to read and understand
the remainder of this document before proceeding.
.NH 2
Converting pre-\*(Ps Systems
.PP
The file system format was changed with 2.8BSD and 2.9BSD (as a kernel
option UCB_NKB), where the fundamental \fIlogical\fP block size of the
file system was changed from 512 bytes to 1024 bytes.  \*(2B uses the
same 1Kb file system of 2.[89]BSD.  Because of this it is absolutely
impossible to do anything except a full bootstrap of \*(2B on any system
other than 2.[89]BSD running the 1K file system.  It \fIis\fP possible to
upgrade a 2.[89]BSD system running the 1K file system, but you will
probably want to do a full bootstrap in any case as 2.[89]BSD and \*(2B
\fBcannot\fP execute each others binaries and device numbering has
changed.
.PP
The safest route is to use \fItar\fP\|(1) to dump all of your current
file systems, do a full bootstrap of \*(2B and then restore user files
from the dumps.  Sites running 2.9BSD with the 1K file system may
use \fIdump\fP\|(8) since 2.9BSD and \*(2B dump formats are identical.
There is also an untested version of \fI512restor\fP\|(8) available for
V7 sites that need to read old dump tapes.
.PP
It is also desirable to make a convenient copy of system configuration
files for use as guides when setting up the new system; the list of files
to save from earlier PDP UNIX systems, found in chapter 3, may be used
as a guideline.
.NH 2
Booting from tape
.PP
The tape bootstrap procedure used to create a
working system involves the following major
steps:
.IP 1)
Loading the tape bootstrap monitor.
.IP 2)
Creating a UNIX ``root'' file system system on disk using \fImkfs\fP\|(8)..
.IP 3)
Restore the full root file system using \fIrestor\fP\|(8).
.IP 4)
Boot the UNIX system on the new root file system and copy it and an
appropriate \fIdead start boot block\fP to your boot device.
.IP 5)
Build and restore the /usr file system from tape
with \fItar\fP\|(1).
.IP 6)
Restore the include and kernel source from tape.
.IP 7)
Extract the remaining source from the second tape.
.IP 8)
Build a \fI/boot\fP configured to locate your root file system for
auto-rebooting (see section 4.1).  Tailor a version of UNIX to your
specific hardware (see section 4.2).
.PP
Certain of these steps are dependent on your hardware
configuration.  If your disks require formatting, standard DEC
diagnostic utilities will have to be used that are not supplied on the
distribution tape.
.NH 3
Step 1: loading the tape bootstrap monitor
.PP
To load the tape bootstrap monitor, first
mount the magnetic tape on drive 0 at load point, making
sure that the write ring is not inserted.
Then use the normal bootstrap ROM, console monitor or other
bootstrap to boot from the tape.
If no other means are available, the following code can be keyed in
and executed at (say) 0100000 to boot from a TM tape drive (the magic number
172526 is the address of the TM-11 current memory address register;
an adjustment may be necessary if your controller is at a nonstandard
address):
.DS
.TS
l l.
012700	(mov $172526, r0)
172526
010040	(mov r0, -(r0))
012740	(mov $60003, -(r0))
060003
000777	(br .)
.TE
.DE
When this is executed, the first block of the tape will be read into memory.
Halt the CPU and restart at location 0.
.PP
The console should type
.DS
.B
\fInn\fPBoot
:
.R
.DE
where \fInn\fP is the CPU class on which it believes it is running.
The value will be one of 24, 40, 44, 45, 70 or 73, depending on whether
separate instruction and data (separate I/D) and/or a UNIBUS map are detected.
The CPUs in each class are:
.DS
.TS
c l c c.
Class	PDP11s	Separate I/D	UNIBUS map
24	24	-	+
40	23, 34, 34A, 40, 60	-	-
45	45, 55	+	-
44	44	+	+
70	70	+	+
73	73	+	-
.TE
.DE
The bootstrap can be forced to set up the machine for a different
class of PDP11 by placing an appropriate value in the console switch register
(if there is one) while booting it.
The value to use is the PDP11 class, interpreted as an \fIoctal\fP number
(use, for example, 070 for an 11/70).
\fBWarning:\fP  some old DEC bootstraps use the switch
register to indicate where to boot from.
On such machines, if the value in the switch register indicates
an incorrect CPU, be sure to reset the switches immediately after
initiating the tape bootstrap.
.PP
You are now talking to the tape bootstrap monitor.
At any point in the following procedure you can return
to this section, reload the tape bootstrap, and restart.
Through the rest of this section,
substitute the correct disk type for \fIdk\fP
and the tape type for \fItp\fP.
.NH 3
Step 2: creating a UNIX ``root'' file system system
.PP
Now create the root file system using the following procedures.
First determine the size of your root file system from the
following table:
.DS
.TS
l l.
Disk	Root File System Size
	  (1K-byte blocks)

br	9120 (T300)
	9196 (T200)
	9200 (T80)
	9130 (T50)
hk	4158\(dg
ra	7942 (RA60/80/81, RC25)
	4850 (RD51/52/53)
rl01	5120\(dd
rl02	10240\(dd
xp	4800 (RM02/RM03)\(dg
	4560 (RM05)
	5120 (CDC 9775)
	4807 (RP04/RP05/RP06)
	4800 (Fujitsu Eagle)
	4702 (DIVA, Ampex 9300)
	8192 (Ampex Capricorn)
	5760 (SI Eagle)
.TE
.sp
\(dg These partitions cover what used to be partitions \fIa\fP and \fIb\fP in 2.9BSD.
\(dd These partitions cover the entire pack.
.DE
.PP
If the disk on which you are creating a root file system is an \fBxp\fP
disk, you should check the drive type register at this time to make sure
it holds a value that will be recognized correctly by the driver.  There
are numbering conflicts; the following numbers are used internally:
.DS
.TS
c l
c l.
Drive Type Register	Drive Assumed
Low Byte (standard address: 0776726)

020	RP04
021	RP05
022	RP06
024	RM03
025	RM02
027	RM05
072	Ampex Capricorn
073	SI, CDC 9775 (direct)
074	SI 6100, Fuji Eagle 2351A
075	Emulex SC01B or SI 9400, Fuji 160
076	Emulex SC-21, Ampex 815 cylinder RM05
077	Diva Comp V, Ampex 9300
.TE
.DE
Check the drive type number in your controller manual,
or halt the CPU and examine this register.
If the value does not correspond to the actual drive type,
you must place the correct value in the switch
register after the tape bootstrap is running
and before any attempt is made to access the drive.
This will override the drive type register.
This value must be present at the time each program
(including the bootstrap itself) first tries to access the disk.
On machines without a switch register, the \fIxptype\fP 
variable can be patched in memory.  After starting each utility
but before accessing the disk, halt the CPU, place the new drive type
number at the proper memory location with the console switches or monitor,
and then continue.  The location of \fIxptype\fP in each utility is
.\"CHECK - XXX
mkfs:  035264, restor:  034140, icheck:  032476 and boot:  0632402
(the location for boot is higher because it relocates itself).
Once UNIX itself is booted (see below) you must
patch it also.
.PP
Finally, determine the proper interleaving factors \fIm\fP and \fIn\fP
for your disk and CPU combination from the following table.
These numbers determine the layout of the free list that will be constructed;
the proper interleaving will help increase the speed of the file system.
If you have a non-DEC disk that emulates one of the disks listed,
you may be able to use these numbers as well, but check that
the actual disk geometry is the same as the emulated disk
(rather than the controller mapping onto a different physical disk).
Also, the rotational speed must be the same as the DEC disk
for these numbers to apply.
.DS
.TS
cB s s s s s s s s
l l l l l l l l l.
Disk Interleaving Factors for Disk/CPU Combinations (\fIm\fP/\fIn\fP)
CPU	23	24	34	40	44	45	53	55
DISK
RL01/2	7/10	6/10	6/10	6/10	4/10	5/10	4/10	5/10
RK06/7	8/33	7/33	6/33	6/33	4/33	5/33	4/33	5/33
RM02	11/80	10/80	8/80	8/80	6/80	7/80	6/80	7/80
RM03	16/80	15/80	12/80	12/80	8/80	11/80	8/80	11/80
RM05	16/304	15/304	12/304	12/304	8/304	11/304	8/304	11/304
RP04/5/6	11/209	10/209	8/209	8/209	6/209	7/209	6/209	7/209
RA60	21/84	21/84	17/84	17/84	12/84	15/84	12/84	15/84
RA80	16/217	16/217	13/217	13/217	9/217	11/217	9/217	11/217
RA81	26/357	26/357	21/357	21/357	14/357	18/357	14/357	17/357
RD51	1/36	1/36	1/36	1/36	1/36	1/36	1/36	1/36
RQDX2*	2/36	2/36	2/36	2/36	2/36	2/36	2/36	2/36
RQDX3*	7/36	7/36	7/36	7/36	7/36	7/36	7/36	7/36
RC25	15/31	15/31	13/31	13/31	9/31	11/31	9/31	11/31
.TE

.TS
cB s s s s s
l l l l l l.
Disk Interleaving Factors for Disk/CPU Combinations (\fIm\fP/\fIn\fP)
CPU	60	70	73	83	84
DISK
RL01/2	5/10	3/10	4/10	4/10	3/10
RK06/7	5/33	3/33	4/33	4/33	3/33
RM02	7/80	5/80	6/80	6/80	5/80
RM03	11/80	7/80	9/80	9/80	7/80
RM05	11/304	7/304	8/304	8/304	7/304
RP04/5/6	7/209	5/209	6/209	6/209	5/209
RA60	15/84	10/84	12/84	12/84	10/84
RA80	11/217	7/217	9/217	9/217	7/217
RA81	18/357	12/357	14/357	14/357	12/357
RD51	1/36	1/36	1/36	1/36	1/36
RQDX2*	2/36	2/36	2/36	2/36	2/36
RQDX3*	7/36	7/36	7/36	7/36	7/36
RC25	11/31	7/31	9/31	9/31	7/31
.TE
.sp
* \fIm\fP/\fIn\fP numbers for RD52/53's are based on controller type
(RQDX2 or RQDX3) rather than drive type.
.DE
For example, for an RP06 on an 11/70, \fIm\fP is 7 and \fIn\fP is 209.
See
\fImkfs\fP\|(8)
for more explanation of the values of \fIm\fP and \fIn\fP.
For \fIm\fP/\fIn\fP numbers for other drive types see \fI/etc/disktab\fP.
.PP
Then run the standalone version of the \fImkfs\fP (8) program.
In the following procedure, substitute the correct types
for \fItp\fP and \fIdk\fP and the size determined above for \fIsize\fP:
.DS
.TS
lw(1.5i) l.
\fB:\|\fP\fItp\|\fP(0,1)	(\fImkfs\fP is tape file 1)
\fBMkfs\fP
\fBfile system:\fP \fIdk\|\fP(0,0)	(root is the first file system on drive 0)
\fBfile system size:\fP \fIsize\fP	(count of 1024 byte blocks in root)
\fBinterleaving factor (m, 5 default):\fP \fIm\fP	(interleaving, see above)
\fBinterleaving modulus (n, 10 default):\fP \fIn\fP	(interleaving, see above)
\fBisize = XX\fP	(count of inodes in root file system)
\fBm/n = \fP\fIm n\fP	(interleave parameters)
\fBExit called\fP
\fInn\fP\fBBoot\fP
\fB:\fP	(back at tape boot level)
.TE
.DE
You now have an empty UNIX root file system.
.NH 3
Step 3: restoring the root file system
.PP
To restore the root file system onto it, type
.DS
.TS
lw(1.5i) l.
\fB:\|\fP\fItp\|\fP(0,2)	(\fIrestor\fP is tape file 2)
\fBRestor\fP
\fBTape?\fP \fItp\|\fP(0,4)	(root \fIdump\fP is tape file 4)
\fBDisk?\fP \fIdk\|\fP(0,0)	(into root file system)
\fBLast chance before scribbling on disk.\fP
.B
end of tape
Exit called
\fInn\fPBoot
\fB:\fR	(back at tape boot level)
.R
.TE
.DE
If you wish, you may use the \fIicheck\fP program on the tape,
\fItp\|\fP(0,3), to check the consistency of the file system you have just
installed.
.NH 3
Step 4: booting UNIX
.PP
You are now ready to boot from disk.  It is best to read the rest
of this section first, since some systems must be patched while booting.
Then type:
.DS
.TS
lw(1.5i) l.
\fB:\fP\fIdk\|\fP(0,0)\fIdk\^\fPunix	(bring in \fIdk\^\fPunix off root system)
.TE
.DE
The standalone boot program should then read \fIdk\^\fPunix from
the root file system you just created, and the system should boot:
.DS
.B
.\"CHECK
\*(2B BSD UNIX #1: Sun Sep 6 01:33:03 PDT 1987
    root@kazoo.Berkeley.EDU:/usr/src/sys/GENERIC
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
If you are booting from an \fIxp\fP with a drive type
that is not recognized, it will be necessary to patch the system before
it first accesses the root file system.
Halt the processor after it
has begun printing the version string but before it has finished printing
the ``mem = ...'' strings.
Place the drive type number corresponding to your drive
.\"CHECK - XXX
at location 052624;
the addresses for
drive 1 is 052640.
If you plan to use any drives other than 0 before you recompile
the system, you should patch these locations.
Make the patches and continue the CPU.
The value before patching must be zero.  If it is not, you have halted too
late and should try again.
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
On an 11/23 with no clock control register, a message ``no clock!!''
will print next; this is a reminder to turn on the clock switch if it is
not already on, since UNIX cannot enable the clock itself.
.PP
The information about different devices being attached or not being found
is produced by the \fIautoconfig\fP\|(8) program.  Most of this is not
important for the moment, but later the device table, \fI/etc/dtab\fP,
can be edited to correspond to your hardware.  However, the tape drive of
the correct type should have been detected and attached.
.PP
The \*(lqerase ...\*(rq message is part of /.profile
that was executed by the root shell when it started.  This message
is present to remind you that the line character erase,
line erase, and interrupt characters are set to be what
is standard on DEC systems; this insures that things are
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
There are a number of copies of \fIunix\fP on the root file system,
one for each possible type of root file system device.
All of the systems were created from \fI/genunix\fP by the
shell script \fI/GENALLSYS\fP.
If you had to patch the \fIxp\fP type as you
booted, you may want to use \fIadb\fP (see
.IR adb (1))
to make the same patch in a copy of \fIxpunix\fP.  See \fI/GENALLSYS\fP
for examples of using \fIadb\fP to patch the system.  See appendix A for
a description of the generic kernels.  The system load images for disk
types other than your own can be removed.
.PP
The disk with the new root file system on it will not be bootable
directly until the block 0 bootstrap program for the disk has been installed.
There are copies of the bootstraps in /mdec.
Use \fIdd\fP\|(1) to copy the right boot block onto block 0 of the disk;
the first form of the command is for small disks (\fBrk\fP, \fBrl\fP)
and the second form for disks with multiple partitions
(\fBhk\fP, \fBrp\fP, \fBxp\fP),
substituting as usual for \fIdk\fP:
.DS
\fB#\fP dd if=/mdec/\fIboot\fP of=/dev/r\fIdk\^\fP0h count=1

or

\fB#\fP dd if=/mdec/\fIboot\fP of=/dev/r\fIdk\^\fP0a count=1
.DE
Block zero bootstraps and the devices they support are:
.DS
.TS
l l l.
boot	driver	devices
_
bruboot	br	Eaton BR1538/BR1711
hkuboot	hk	RK06/07
rauboot	ra	RA60/80/81, RC25, RD51/52/53, RX50
rkuboot	rk	RK05
rluboot	rl	RL01/02
si95uboot	si	SI 9500, CDC 9766
dvhpuboot	xp	Diva Comp V, Ampex 9300
hpuboot	xp	RP04/05/06
rm03uboot	xp	RM02/03
rm05uboot	xp	RM05 or SI 9500, CDC 9766
si51uboot	xp	SI 6100, Fujitsu Eagle 2351A
si94uboot	xp	Emulex SC01B or SI 9400, Fujitsu 160
.TE
.DE
Once this is done, booting from this disk will load and execute the block
0 bootstrap, which will in turn load /boot (actually, the boot program on
the file system starting at block zero of the disk, which is normally
root).  The console will print
.DS
.TS
lw(1.5i) l.
\fB>boot\fP	(printed by some block 0 boots)

.B
\fInn\fPBoot	\fR(printed by /boot)\fP
:
.R
.TE
.DE
The '>' is the prompt from the first bootstrap.
It automatically boots /\fIboot\fP for you;
if /\fIboot\fP is not found, it will prompt again and allow another
name to be tried.
It is a very small and simple program, however, and can only
boot the second-stage boot from the first file system.
Once /boot is running and prints its ``: '' prompt,
boot unix as above, using \fIdk\^\fPunix or unix as appropriate.
.PP
N.B. some primary bootstraps have no prompt because of
space considerations (and some, like the RA boot, can't even ask for
alternate program names.)  No diagnostic message results if the file cannot
be found, and no provision is made for correcting typographical errors when
entering alternate names.  Hitting a return will cause an error and allow
restarting.
.NH 3
Step 5: setting up the /usr file system
.PP
First set a shell variable to the name of your disk, so
the commands we give will work regardless of the disk you
have; do one of the following:
.DS
.TS
l l.
\fB#\fP disk=hk	(if you have RK06's or RK07's)
\fB#\fP disk=rl	(if you have RL01's or RL02's)
\fB#\fP disk=ra	(if you have UDA50 or other MSCP storage module drives)
\fB#\fP disk=xp	(if you have an RP06, RM03, RM05, or other SMD drive)
.TE
.DE
.PP
The next thing to do is to extract the rest of the data from the tape.
You might wish to review the disk configuration information in section 4.3
before continuing; you will have to select a partition to restore the /usr
file system into which is at least \fB25\fP Megabytes in size (this is
just barely enough for the system binaries and such and leaves no room
for the system source.)  The partitions used below are those most
appropriate in size.  Find the disk you have in the following table and
execute the commands in the right hand portion of the table:
.DS
.TS
l l.
DEC RM02/03	\fB#\fP name=xp0c; type=rm03
DEC RM05	\fB#\fP name=xp0e; type=rm05
DEC RP04/05	\fB#\fP name=xp0c; type=rp04
DEC RP06	\fB#\fP name=xp0c; type=rp06
DEC RK07	\fB#\fP name=hk0c; type=rk07
DEC RA60	\fB#\fP name=ra0c; type=ra60
DEC RA80	\fB#\fP name=ra0c; type=ra80
DEC RA81	\fB#\fP name=ra0c; type=ra81
DEC RC25	\fB#\fP name=ra0c; type=rc25
DEC RD52	\fB#\fP name=ra0g; type=rd52-rqdx\fIn\fP*
DEC RD53	\fB#\fP name=ra0d; type=rd53-rqdx\fIn\fP*
-
CDC 9766	\fB#\fP name=xp0e**
CDC 9775	\fB#\fP name=xp0e**
AMPEX 300M	\fB#\fP name=xp0e**
FUJITSU 160M	\fB#\fP name=xp0d**
FUJITSU 2351A	\fB#\fP name=xp0d**
AMPEX 330M	\fB#\fP name=xp0c**
.TE
.FS
* The type of controller the RD52 and RD53 are on (RQDX2 or RQDX3) must
be specified for proper file system free list spacing to be computed.  If
you don't know what controller your RD is on, it is all right to guess,
though the created file system may not perform as well.
.FE
.FS
** Unfortunately the \fInewfs\fP(8) program is relatively primitive and
doesn't know the free list spacing for these non-DEC drives.  An
appropriate \fImkfs\fP(8) command will have to be substituted for the
\fInewfs\fP command specified below.  See \fI/etc/disktab\fP for
[hopefully] appropriate free list \fIm/n\fP numbers for some non-DEC
disks.
.FE
.DE
Find the tape you have in the following table and execute the
commands in the right hand portion of the table:
.DS
.TS
l l.
DEC TM02/03, TE16/TU45/TU77	\fB#\fP cd /dev; MAKEDEV ht0; sync
DEC TS11, TU80/TS05	\fB#\fP cd /dev; MAKEDEV ts0; sync
DEC TM11, TU10/TE10/TS03	\fB#\fP cd /dev; MAKEDEV tm0; sync
EMULEX TC11	\fB#\fP cd /dev; MAKEDEV tm0; sync
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
\fB#\fP newfs ${name} ${type}	(create empty user file system)
(this takes a few minutes)
\fB#\fP mount /dev/${name} /usr	(mount the usr file system)
\fB#\fP cd /usr	(make /usr the current directory)
\fB#\fP mt rew
\fB#\fP mt fsf 5
\fB#\fP tar xpbf 20 /dev/rmt12 	(extract all of usr except usr/src)
(this takes about 15-20 minutes)
.TE
.DE
The data on the sixth tape file has now been extracted.
All that remains on the first tape is a small archive containing
source for the kernel and include files.
.DS
.TS
lw(2i) l.
\fB#\fP mt fsf		(position tape at beginning of next tape file)
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
** /dev/r\fIxx\fP0h
** Last Mounted on /usr
** Phase 1 - Check Blocks and Sizes
** Phase 2 - Check Pathnames
** Phase 3 - Check Connectivity
** Phase 4 - Check Reference Counts
** Phase 5 - Check Cyl groups
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
\fB#\fP /etc/mount /dev/${name} /usr
.DE
.NH 3
Step 7: extracting remaining source from the second tape
.PP
You can then extract the source code for the commands from the
second distribution tape
(except on RK07's, RM03's, and RD52's this will fit in the /usr file system):
.DS
\fB#\fP cd /usr/src
\fB#\fP tar xpb 20
.DE
If you get an error at this point, most likely it was
a problem with tape positioning.
You can reposition the tape by rewinding it.
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
original.)  To restore a dump tape for, say, the /a file system something
like the following would be used:
.DS
\fB#\fP restor r /dev/xp1e
.DE
.PP
If \fItar\fP images were written instead of doing a dump, you should
be sure to use the `p' option when reading the files back.
No matter how you restore a file system, be sure and check its
integrity with \fIfsck\fP when the job is complete.

.\"	Placed in the public domain June 17, 1995.
.\"
.\"	@(#)b.t	1.1 (2.11BSD) 1995/07/10
.\"
.de IR
\fI\\$1\fP\|\\$2
..
.ds LH "Installing/Operating \*(2B
.nr H1 8
.nr H2 0
.ds RH "Appendix B \- standalone disklabel program
.ds CF \*(DY
.bp
.LG
.B
.ce
APPENDIX B \- STANDALONE DISKLABEL PROGRAM
.sp 2
.R
.NL
.NH 2
Standalone disklabel example
.PP
This is a real example of using the disklabel program to place a label
on a disk.  User input is in \fBbold\fP type.  The disklabel program was
loaded from a bootable TK50.  The disk being labeled in a RD54.  The BOOT> 
prompt is from the 11/73 console ODT, if you are using an 11/44 the prompt 
will be >>>.
.PP
The first thing that is done is request disklabel to create a default
partition ('a') which spans the entire disk.  Some disk types have fixed
sizes and geometries, for example RK05 (rk), RK06/7 (hk) and RL02 (rl)
drives.  With this type of disk the standalone disklabel program will generate
a label with the correct geometry and 'a' partition size.  With MSCP ('ra') 
disks disklabel will query the controller for the information it needs.  The
last type of disk, SMD (xp), presents many problems, disklabel will attempt
to determine the drive type and geometry but you will have to verify the
information.
.sp
.in +0.6i
Indented paragraphs like this one are explanatory comments and 
are not part of the output from the disklabel program.  In the case of MSCP
drives the number of cylinders may be 1 too low.  This is discussed in the
example below.
.br
.in -0.6i
.nf

BOOT> \fBMU 0\fP

73Boot from tms(0,0,0) at 0174500
: \fBtms(0,1)\fP
Boot: bootdev=06001 bootcsr=0174500
disklabel
Disk? \fBra(0,0)\fP
d(isplay) D(efault) m(odify) w(rite) q(uit)? \fBD\fP
d(isplay) D(efault) m(odify) w(rite) q(uit)? \fBd\fP

type: MSCP
disk: RD54
flags:
bytes/sector: 512
sectors/track: 17
tracks/cylinder: 15
sectors/cylinder: 255
cylinders: 1220
rpm: 3600
drivedata: 0 0 0 0 0

1 partitions:
#      size   offset   fstype   [fsize bsize]
  a: 311200 0 2.11BSD 1024 1024  # (Cyl. 0 - 1220*)

.fi
.in +0.6i
The columns do not line up nicely under the headings due to limitations
of the sprintf() routine in the standalone I/O package.  There is no
capability to justify the output.  It should be obvious which column
belongs under which heading.  The '*' says that the partition does not
end on a cylinder boundary.  This is due to the peculiar way in which
MSCP returns the geometry information:  sectors/track * tracks/cylinder *
cylinders != sectors per volume.
.sp
.in -0.6i
.nf
d(isplay) D(efault) m(odify) w(rite) q(uit)? \fBm\fP
modify
d(isplay) g(eometry) m(isc) p(artitions) q(uit)? \fBm\fP

.fi
.in +0.6i
It is normally not necessary to change the geometry of an MSCP disk.  On
the other hand it will almost always be necessary to specify the geometry
of an SMD drive (one which uses the XP driver).  Since the drive being
labeled is an MSCP drive the next step is to set the pack label to something
other than DEFAULT.
.br
.in -0.6i
.nf

modify misc
d(isplay) t(ype) n(ame) l(able) f(lags) r(pm) D(rivedata) q(uit)? \fBl\fP
label [DEFAULT]: \fBTESTING\fP
modify misc
d(isplay) t(ype) n(ame) l(able) f(lags) r(pm) D(rivedata) q(uit)? \fBq\fP
modify
d(isplay) g(eometry) m(isc) p(artitions) q(uit)? \fBp\fP
modify partitions
d(isplay) n(umber) s(elect) q(uit)? \fBd\fP

type: MSCP
disk: RD54
flags:
bytes/sector: 512
sectors/track: 17
tracks/cylinder: 15
sectors/cylinder: 255
cylinders: 1220
rpm: 3600
drivedata: 0 0 0 0 0

1 partitions:
#      size   offset   fstype   [fsize bsize]
  a: 311200 0 2.11BSD 1024 1024  # (Cyl. 0 - 1220*)

modify partitions
d(isplay) n(umber) s(elect) q(uit)? \fBs\fP
a b c d e f g h q(uit)? \fBa\fP
sizes and offsets may be given as sectors, cylinders
or cylinders plus sectors: 6200, 32c, 19c10s respectively
modify partition 'a'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBs\fP
\'a' size [311200]: \fB15884\fP
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBq\fP
modify partitions
d(isplay) n(umber) s(elect) q(uit)? \fBs\fP
a b c d e f g h q(uit)? \fBb\fP
sizes and offsets may be given as sectors, cylinders
or cylinders plus sectors: 6200, 32c, 19c10s respectively
modify partition 'b'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBo\fP
\'b' offset [0]: \fB15884\fP
modify partition 'b'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBs\fP
\'b' size [0]: \fB16720\fP
modify partition 'b'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBt\fP
\'b' fstype [unused]: \fBswap\fP
modify partition 'b'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBq\fP
modify partitions
d(isplay) n(umber) s(elect) q(uit)? \fBs\fP
a b c d e f g h q(uit)? \fBc\fP
sizes and offsets may be given as sectors, cylinders
or cylinders plus sectors: 6200, 32c, 19c10s respectively
modify partition 'c'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBo\fP
\'c' offset [0]: \fB0\fP
modify partitions 'c'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBs\fP
\'c' size [0]: \fB311200\fP
modify partitions 'c'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBt\fP
\'c' fstype [unused]: \fBunused\fP
modify partitions 'c'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBq\fP
modify partitions
d(isplay) n(umber) s(elect) q(uit)? \fBs\fP
a b c d e f g h q(uit)? \fBg\fP
sizes and offsets may be given as sectors, cylinders
or cylinders plus sectors: 6200, 32c, 19c10s respectively
modify partition 'g'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBo\fP
\'g' offset [0]: \fB32604\fP
modify partition 'g'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBs\fP
\'g' size [0]: \fB278596\fP
modify partition 'g'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBt\fP
\'g' fstype [unused]: \fB2.11BSD\fP
modify partition 'g'
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBq\fP
modify partitions
d(isplay) n(umber) s(elect) q(uit)? \fBn\fP
Number of partitions (8 max) [7]? \fB7\fP
modify partitions
d(isplay) n(umber) s(elect) q(uit)? \fBq\fP
modify
d(isplay) g(eometry) m(isc) p(artitions) q(uit)? \fBd\fP

type: MSCP
disk: RD54
label: TESTING
flags:
bytes/sector: 512
sectors/track: 17
tracks/cylinder: 15
sectors/cylinder: 255
cylinders: 1220
rpm: 3600
drivedata: 0 0 0 0 0

7 partitions:
#      size   offset   fstype   [fsize bsize]
  a: 15884   0  2.11BSD 1024 1024  # (Cyl. 0 - 62*)
  b: 16720 15884 swap              # (Cyl. 62*- 127*)
  c: 311200  0   unused 1024 1024  # (Cyl. 0 - 1220*)
  g: 278596 32604 2.11BSD 1024 1024 # (Cyl. 127- 1220*)

modify
d(isplay) g(eometry) m(isc) p(artitions) q(uit)? \fBq\fP

.fi
.in +0.6i
On MSCP disks it is possible you will see a warning error like this:
.sp
partition c: extends past end of unit 0 311200 311100
.br
partition g: extends past end of unit 32604 278596 311100
.sp
This is not cause for panic.  What this is saying is that the number of
cylinders is one too low.  MSCP devices do not necessarily use all of the
last cylinder.  The total number of blocks is precisely known for MSCP
devices  (it is returned in the act of bringing the drive online).  
However the number of sectors on the volume is not necessarily evenly
divisible by the number of sectors per track (311200 divided by 17*15 gives
1220.392).  Basically the last cylinder is not fully used.  What must be done
is raise the number of cylinders by 1.
.sp
\fBNOTE:\fP For any other disk type it is cause for concern if the warning
above is issued \- it means that incorrect partition or geometry information
was entered by the user and needs to be corrected.
.sp
.in -0.6i
.nf
d(isplay) D(efault) m(odify) w(rite) q(uit)? \fBm\fP
modify
d(isplay) g(eometry) m(isc) p(artitions) q(uit)? \fBg\fP
modify geometry
d(isplay) s(ector/trk) t(rk/cyl) c(yl) S(ector/cyl) q(uit)? \fBc\fP
cylinders [1220]: \fB1221\fP
modify geometry
d(isplay) s(ector/trk) t(rk/cyl) c(yl) S(ector/cyl) q(uit)? \fBq\fP
modify
d(isplay) g(eometry) m(isc) p(artitions) q(uit)? \fBq\fP
d(isplay) D(efault) m(odify) w(rite) q(uit)? \fBw\fP
d(isplay) D(efault) m(odify) w(rite) q(uit)? \fBq\fP

73Boot from tms(0,0,1) at 0174500
: \fBra(1,0,0)unix\fP
ra1 csr[00]: \fB0172154\fP

.fi
.in +0.6i
The last string entered shows how I boot from an alternate controller.  In
normal use, i.e. with a single MSCP controller, the string would simply
be \fBra(0,0)unix\fP.
.NH 2
Standalone disklabel program
.PP
The standalone disklabel program is the second file on a boot tape (after
the bootblocks and boot program).  It is used to place an initial label
on a disk
describing the disk and its partitions.  The program is also used when the
root ('a') or swap ('b') partitions of a previously labeled system disk must
be modified.  The second use is mandated because the 
root and swap partitions can not be modified while the kernel has them
open.
.PP
\fIdisklabel\fP effectively runs in CBREAK mode \- you do not need to
hit the RETURN key except when prompted for a multicharacter response
such as a string (the pack label) or a number (partition size).
Defaults are placed inside square brackets ([default]).   Entering 
RETURN accepts the default.
.PP
The program is organized into several levels.  \fIdisklabel\fP prints the
current level out before prompting.  At each level there is
always the choice of d(isplaying) the current label and q(uit)ing the current
level and returning to the previous level.  If you are at the top level
and enter \fBq\fP the program will exit back to \fBBoot\fP unless you have made
changes to the disklabel.  In that case you will be asked if you wish
to discard the changes, if you answer \fBy\fP the changes will be discarded.
If the answer is \fBn\fP the \fBq\fP is ignored and \fIdisklabel\fP does
not exit.
.PP
In the following paragraphs the convention is to \fBbold\fP the user input
while leaving the output from \fIdisklabel\fP in normal type.  The devices
used were a TK50 and an RD54, thus the tape device is \fBtms\fP and the 
disk device is \fBra\fP.
.PP
The TK50 was booted resulting in the usual message from \fBBoot\fP:
.LP
73Boot from tms(0,0,0) at 0174500
.br
: \fBtms(0,1)\fP
.NH 2
Disklabel \- tour of the levels.
.LP
Boot: bootdev=06001 bootcsr=0174500
.br
disklabel
.br
Disk? \fBra(0,0)\fP
.br
d(isplay) D(efault) m(odify) w(write) q(uit)? \fBm\fP
.sp
.in +0.6i
The 'D' option will request \fIdisklabel\fP to create a default label based
on what the program can determine about the drive.  For some devices, such
as RL01/02, RK06/07, MSCP (RD54, RA81, usw.), \fIdisklabel\fP can
determine what the drive type is and how many sectors it has.  For other 
devices, such as SMD drives supported by the \fBxp\fP driver, the task is 
complicated by the number of different controllers and emulations supported.
Some 3rd party controllers have capabilities that DEC controllers do not and
the \fBxp\fP has no way of knowing exactly which type of controller is
present.  In this case \fIdisklabel\fP will \fBguess\fP and then depend on
you to enter the correct data.
.br
.in -0.6i
.sp
modify
.br
d(isplay) g(eometry) m(isc) p(artitions) q(uit)? \fBg\fP
.br
modify geometry
.br
d(isplay) s(ector/trk) t(rk/cyl) c(yl) S(ector/cyl) q(uit)? \fBq\fP
.sp
.in +0.6i
The Sector/cyl entry is rarely used.  \fIdisklabel\fP will calculate
this quantity for you from the sector/trk and trk/cyl quantities.
.br
.in -0.6i
.sp
modify
.br
d(isplay) g(eometry) m(isc) p(artitions) q(uit)? \fBm\fP
.br
d(isplay) t(ype) n(ame) l(abel) f(lags) r(pm) D(rivedata) q(quit)? \fBf\fP
.sp
.in +0.6i
Type is one of: SMD, MSCP, old DEC, SCSI, ESDI, ST506, floppy.
.sp
Name is a string up to 16 characters in length.  It is typically
something like \fBrd54\fP or \fBrm03\fP but may be any meaningful string.
.sp
Label is an arbitrary string up to 16 characters in length \- nothing in 
the system checks for or depends on the contents of the pack label string.
.sp
Rpm is the rotational speed of the drive.  Nothing in the system uses
or depends on this at the present time.  Default is 3600.
.sp
Drivedata consists of 5 longwords of arbitrary data.  Reserved for future use.
.br
.in -0.6i
.sp
modify misc flags
.br
d(isplay) c(lear) e(cc) b(adsect) r(emovable) q(uit)? \fBq\fP
.sp
.in +0.6i
Ecc says that the controller/driver can correct errors.
.sp
Badsect indicates that the controller/driver supports bad sector replacement.
.sp
Removable indicates that the drive uses removable media (floppy, RL02,
RA60 for example).
.br
.in -0.6i
.sp
modify misc
.br
d(isplay) t(type) n(ame) l(abel) f(lags) r(pm) D(rivedata) q(uit)? \fBq\fP
.br
modify
.br
d(isplay) g(eometry) m(isc) p(artitions) q(uit)? \fBp\fP
.br
modify partitions
.br
d(isplay) n(umber) s(elect) q(uit)? \fBn\fP
.br
Number of partitions (8 max) [7]? \fB7\fP
.sp
.in +0.6i
This is the highest partition number considered to be valid.
\fIdisklabel\fP will adjust this parameter semi-automatically at the
\fBp\fP level but it may be necessary to use \fBn\fP in cases where
some partitions are not to be used or contain invalid information.
.br
.in -0.6i
.sp
modify partitions
.br
d(isplay) n(umber) s(elect) q(uit)? \fBs\fP
.br
a b c d e f g h q(uit)? \fBa\fP
.br
sizes and offsets may be given as sectors, cylinders
.br
or cylinders plus sectors:  6200, 32c, 19c10 respectively
.br
modify partition 'a'
.br
d(isplay) z(ero) t(ype) o(ffset) s(ize) f(rag) F(size) q(uit)? \fBq\fP
.sp
.in +0.6i
Zero clears the size and offset fields of a partition entry and sets the
filesystem type to \fBunused\fP.
.sp
Type is the filesystem type and of the possible choices only \fB2.11BSD\fP,
\fBswap\fP and \fBunused\fP make any sense to specify.
.sp
Offset is the number of sectors from the beginning of the disk at which the
partition starts.
.sp
Size is the number of sectors which the partition occupies.
.sp
Frag is the number of fragments a filesystem block can be broken into.
It is not presently used and should be left at the default of 1.
.sp
Fsize is the filesystem blocksize and should be left at the default of
1024.
.sp
modify partitions
.br
d(isplay) n(umber) s(elect) q(uit)? \fBq\fP
.br
modify
.br
d(isplay) g(eometry) m(isc) p(artitions) q(uit)? \fBq\fP
.br
d(isplay) D(efault) m(odify) w(write) q(uit)? \fBq\fP
.br
Label changed.  Discard changes [y/n]? \fBy\fP
.br
.sp 2
73Boot from tms(0,0,1) at 0174500
.NH 2
Disklabel \- helpful hints and tips.
.PP
Define only those partitions you actually will use.  There is no need to
set up all 8 partitions.  Drives less than 200Mb probably will only have
3 partitions defined, 'a', 'b' and 'd' for /, swap and /usr respectively.
Remember to set the number of partitions.  Disklabel will attempt to do this
for you by keeping track of the highest partition you modify but this is
not foolproof.
.PP
Do not define overlapping partitions unless you are sure what you are 
doing.  \fIdisklabel\fP will warn you of overlapping partitions but will
not prohibit you from writing such a label to disk.
.PP
Remember that the prompt levels nest in \fIdisklabel\fP.  It will be necessary
in several cases to enter multiple \fBq\fP commands to get back to the top
level.
.PP
.B IMPORTANT:
Keep at least 1, preferably more,  bootable tape or floppy with 
\fIdisklabel\fP on it present at all times.  If the label on a disk
ever becomes corrupted the kernel will be very unhappy and probably won't
boot.  If this happens you will need to boot the standalone \fIdisklabel\fP
program and relabel the disk.  At least \*(2B provides a standalone 
\fIdisklabel\fP \- previous 4BSD systems which implemented disklabels did not
and the cold-start of those systems was painful indeed.
.PP
.B IMPORTANT:
Write down in at least one place, and keep with the tape/floppy mentioned
above, the geometry and partition layout you assign to the disk.  The
\fIdisklabel\fI\|(8) program should be used to produce a hardcopy of the
disklabel.

.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)a.t	6.2 (Berkeley) 10/1/88
.\"
.de IR
\fI\\$1\fP\|\\$2
..
.ds LH "Installing/Operating \*(2B
.nr H1 6
.nr H2 0
.ds RH "Appendix A \- bootstrap details
.ds CF \*(DY
.bp
.LG
.B
.ce
APPENDIX A \- KERNEL CONFIGURATION OPTIONS
.sp 2
.R
.NL
.NH 2
Kernel configuration options
.PP
The \*(2B kernel has a number of parameters and options that can be
used to tailor the kernel to site specific needs.
This appendix lists the parameters and options used in the kernel.  The
parameters have numeric values, usually table sizes.  The options flags
are either defined or undefined (via the values YES or NO respectively.)
.PP
Prototypes for all the following options can be found in the generic
kernel configuration file \fI/sys/conf/GENERIC\fP.  The process of configuring
a new kernel consists simply of copying the generic configuration
file to a new file, \fISYSTEM\fP and then editing the options in
\fISYSTEM\fP to reflect your needs.  You can treat the items copied from
GENERIC as a ``grocery list'', checking off those options you want,
crossing out those you don't and setting numeric parameters to reasonable
values.
.NH 2
Configuring the number of mountable file systems (NMOUNT)
.PP
Because of time constraints we were unable to move the
.B NMOUNT
constant into the kernel configuration file where it belongs.
.B NMOUNT
is used to configure the number of mountable file systems in \*(2B.  Since
each slot in the kernel mount table takes up close to a half Kb of
valuable kernel data space, the distribution kernel comes configured
with
.B NMOUNT
set to 5.
This is almost certainly too small for most sites and should be increased
to the number of file systems you expect to mount.
.PP
.B NMOUNT
is defined in
.IR /sys/h/param.h .
If you change its value, you must recompile the kernel (obviously) and the
following applications:
.IR mount ,
.IR quotaon ,
.IR edquota ,
.IR umount ,
and
.IR df .
.NH 2
GENERIC kernel configuration
.PP
All of the generic kernels support the following devices:
.TS
l n.
Device	Number
-
RK06/07	2
MSCP (RA) Controllers	1
MSCP (RA) Disks	2
RL01/02	2
SMD (XP) Controllers	1
SMD (XP) Disks	2
TE16, TU45, TU77 (HT) Tape drives	1
TM11 (TM) Tape drives	1
TS11 (TS) Tape drives	1
.TE
.PP
The generic kernels are set up with the following disk drive
configurations:
.TS
l l l n l.
Kernel	root/pipe	swap	nswap	comments
-
hkunix	/dev/hk0e	/dev/hk1a	5940	two drive, 0e covers 0a and 0b
raunix	/dev/ra0a	/dev/ra0b	10032
rlunix	/dev/rl0h	/dev/rl1h	10240	two drive, no partitions on RLs
xpunix	/dev/xp0a	/dev/xp0b	8878
.TE
.PP
If these values aren't usable at all, you'll have to patch the kernel
before it first accesses any of the disks.  Don't do this unless you
absolutely have to.  It's far better to use the defaults, rebuild a
kernel for your specific setup, and then rearrange your file systems.
.\"CHECK - XXX
.TS
l l l.
Variable	location	comments
-
rootdev	02336	Root device - a file system
pipedev	02342	Pipe device - a file system
swapdev	02340	Swap device - raw chunk of disk
nswap	02350	Size of swap partition
.TE
.PP
.TS
l l.
Device	major*256
-
HK	02000
RA	02400
RL	03400
XP	05000
.TE
.IR Rootdev ,
.IR pipedev ,
and
.I swapdev
are 256 * device major number + device minor number.  Device minor numbers
are 8 * drive + partition.
.NH 3
GENERIC kernel configuration file
.PP
.ta 8n 16n 24n 32n 40n 48n 56n 72n 80n
.nf
# Machine configuration file for 2.10BSD distributed kernel.
#
# Format:
#	name	value		comments
# An item's value may be either numerical, boolean or a string; if it's
# boolean, use "YES" or "NO" to set it or unset it, respectively.  Use
# the default value and the comments field as indicators of the type of
# field it is.

#########################################
# MACHINE DEPENDENT PARAMETERS		#
#########################################

# Machine type
# 2.10 runs on:
#	11/24/34/44/53/60/70/73/83/84
#	11/23/35/40/45/50/55 with 18 or 22 bit addressing
# 2.10 WILL NOT run on:
#	T11, 11/03/04/05/10/15/20/21
#	11/23/35/40/45/50/55 with 16 bit addressing
# 2.10 networking will run on:
#	11/44/53/70/73/83/84
#	11/45/50/55 with 18 bit addressing
#
# Any QBUS machine using an ABLE Microverter should be treated as
# if it were an 11/70.
#
# You should also make a point of having both floating point hardware and
# at least a megabyte of memory, if possible.  They aren't that expensive,
# and they make a big difference.
#
# Including UNIBUS map support for machines without a UNIBUS will not cause
# a kernel to die.  It simply includes code to support UNIBUS mapping if
# present.

# The define UNIBUS_MAP implements kernel support for UNIBUS mapped
# machines.  However, a kernel compiled with UNIBUS_MAP does *not* have to
# be run on a UNIBUS machine.  The define simply includes support for UNIBUS
# mapping if the kernel finds itself on a machine with UNIBUS mapping.
UNIBUS_MAP	YES			# include support for UNIBUS mapping
					# always: 11/44/70/84
					# sometimes: 11/24
					# never: 11/23/34/35/40/45/50/53/
					#	55/60/73/83

# The define Q22 states that the configured system is a 22-bit Q-BUS machine
# (if UNIBUS mapping isn't found) and no 18-bit DMA disk or tape devices
# exist.  If Q22 is defined and an 18-bit DMA disk or tape does exist, reads
# and writes to the raw devices will cause DMA transfers to and from user
# space which might be above 18-bits (256K) which would cause random
# sections of memory (probably the kernel) to be overwritten (for reads).
# An 18-bit DH isn't a problem since it never does DMA to user space and
# clists (even with UCB_CLIST defined) are never above 18-bits.
#
# Note, the Q22 define is only effective if the kernel finds itself on a
# machine without UNIBUS mapping.  Note also that the presence of UNIBUS
# mapping is only tested for if UNIBUS_MAP is defined.
Q22		NO			# 22-bit Qbus with no 18-bit devices
					# always 11/53/73/83
					# sometimes: 11/23
					# never: 11/24/34/35/40/44/50/
					#	55/60/70/84

# Defining NONFP to NO compiles in support for hardware floating point.
# However, this doesn't require that floating point hardware be present.
# Defining NONFP to YES will save you a few hundred bytes of text.
NONFP		NO			# if no floating point hardware

# Defining FPSIM to YES compiles a floating point simulator into the kernel
# which will catch floating point instruction traps from user space.  Note
# that defining FPSIM to YES will only cost you text space.  If you actually
# have floating point hardware, the simulator just won't be used.  The floating
# point simulator is automatically compiled in if PDP11 (below) is GENERIC.
FPSIM		NO			# floating point simulator

#LINEHZ		50			# clock frequency European
LINEHZ		60			# clock frequency USA

# To enable profiling, the :splfix script must be changed to use spl6 instead
# of spl7 (see conf/:splfix.profile), also, you have to have a machine with a
# supervisor PAR/PDR pair, i.e. an 11/44/45/50/53/55/70/73/83/84, as well
# as both a KW11-L and a KW11-P.
#
# Note that profiling is not currently working.  We don't have any plans on
# fixing it, so this is essentially a non-supported feature.
PROFILE		NO			# system profiling with KW11P clock

# PDP-11 machine type; allowable values are GENERIC, 23, 24, 34, 35, 40,
# 44, 45, 50, 53, 55, 60, 70, 73, 83, 84.  GENERIC should only be used to
# build a distribution kernel.
PDP11		GENERIC			# distribution kernel
#PDP11		44			# PDP-11/44
#PDP11		70			# PDP-11/70
#PDP11		73			# PDP-11/73

#########################################
# GENERAL SYSTEM PARAMETERS		#
#########################################

IDENT		GENERIC			# machine name
MAXUSERS	4			# maxusers on machine

# BOOTDEV is the letter combination denoting the autoboot device,
# or NONE if not using the autoboot feature.
BOOTDEV		NONE			# don't autoboot
#BOOTDEV	dvhp			# DIVA Comp/V boot device
#BOOTDEV	hk6			# rk06 boot device
#BOOTDEV	hk7			# rk07 boot device
#BOOTDEV	ra			# MSCP boot device
#BOOTDEV	rl			# rl01/02 boot device
#BOOTDEV	rm			# rm02/03/05 boot device
#BOOTDEV	br			# Eaton BR1537/BR1711 boot device
#BOOTDEV	sc11			# Emulex SC11/B boot device
#BOOTDEV	sc21			# Emulex SC21 boot device
#BOOTDEV	si			# si 9500 boot device

# Timezone, in minutes west of GMT
#TIMEZONE	300			# EST
#TIMEZONE	360			# CST
#TIMEZONE	420			# WST
TIMEZONE	480			# PST
DST		1			# Daylight Savings Time (1 or 0)

# Filesystem configuration
# Rootdev, swapdev etc. should be in terms of makedev.  For example,
# if you have an SMD drive using the xp driver, rootdev would be xp0a,
# or "makedev(10,0)".  Swapdev would be the b partition, xp0b, or
# "makedev(10,1)".  The 10 is the major number of the device (the offset
# in the bdevsw table in conf.c) and the 0 and 1 are the minor numbers
# which correspond to the partitions as described in the section 4 manual
# pages.  You can also get the major numbers from the MAKEDEV script in
# /dev.
PIPEDEV		makedev(10,0)		# makedev(10,0) xp0a
ROOTDEV		makedev(10,0)		# makedev(10,0) xp0a
SWAPDEV		makedev(10,1)		# makedev(10,1) xp0b
SWAPLO		0			# swap start address, normally 0

# DUMPROUTINE indicates which dump routine should be used.  DUMPDEV
# should be in terms of makedev.  If DUMPDEV is NODEV no automatic
# dumps will be taken, and DUMPROUTINE needs to be set to "nulldev" to
# resolve the reference.  See param.h and ioconf.c for more information.
# DUMPLO should leave room for the kernel to start swapping without
# overwriting the dump.
DUMPLO		512			# dump start address
DUMPDEV		NODEV			# makedev(10,1) xp0b
DUMPROUTINE	nulldev			# no dump routine.
#DUMPROUTINE	hkdump			# hk driver dump routine
#DUMPROUTINE	hpdump			# hp driver dump routine
#DUMPROUTINE	radump			# ra driver dump routine
#DUMPROUTINE	rldump			# rl driver dump routine
#DUMPROUTINE	rmdump			# rm driver dump routine
#DUMPROUTINE	brdump			# br driver dump routine
#DUMPROUTINE	sidump			# si driver dump routine
#DUMPROUTINE	xpdump			# xp driver dump routine

# NSWAP should be set to the 512-byte block length of the swap device, e.g.
# 9120 for an RM05 B partition.
#NSWAP		9405			# dvhp?b or xp?b, DIVA COMP V
#NSWAP		5940			# hk?a, RK611, RK06/07
NSWAP		2376			# hk?b, RK611, RK06/07
#NSWAP		12122			# br?b, Eaton BR1538 or BR1711
#NSWAP		8779			# hp?b or xp?b, RP04/05/06
#NSWAP		4800			# rm?b or xp?b, RM02/03
#NSWAP		9120			# xp?b, RM05
#NSWAP		17300			# rd?b, RD51/52/53
#NSWAP		3100			# rd?c, RD51/52/53
#NSWAP		10032			# ra?b, RC25
#NSWAP		33440			# ra?b, RA60/80
#NSWAP		10240			# rl?, RL01
#NSWAP		20480			# rl?, RL02

#########################################
# KERNEL CONFIGURATION			#
#########################################

BADSECT		NO			# bad-sector forwarding
CGL_RTP		NO			# allow one real time process
EXTERNALITIMES	NO			# map out inode time values
SMALL		NO			# smaller inode, buf, sched queues
UCB_CLIST	NO			# clists moved from kernel data space
UCB_FRCSWAP	NO			# force swap on expand/fork
NOKA5		NO			# KA5 not used except for buffers
					# and clists (_end < 0120000);
QUOTA		NO			# dynamic file system quotas
					# NOTE -- *very* expensive

# UCB_METER is fairly expensive.  Unless you really look at the statistics
# that it produces, don't bother running with it on.  Suggested usage is
# when you want to tune your system or you're curious about how effective
# some algorithm, for example, the text coremap cacheing, is.  UCB_RUSAGE
# isn't nearly as bad, and should probably be included, although it's not
# necessary for anything.
UCB_METER	NO			# vmstat performance metering
UCB_RUSAGE	YES			# enable more rusage fields

# If your system is *seriously* short of memory, and you're doing a lot of
# thrashing, 2.10's implementation of vfork can hurt you.  Otherwise, run
# with it on.  You do not have to recompile any applications when you change
# it.  You should also get rid of any local code that uses VIRUS_VFORK to
# decide whether to call fork or vfork, they should just call vfork.
VIRUS_VFORK	YES			# include vfork system call

# NBUF is the size of the buffer cache, and is directly related to the UNIBUS
# mapping registers.  There are 32 total mapping registers, of which 30 are
# available.  The 0'th is used for CLISTS, and the 31st is used for the I/O
# page on some PDP's.  It's suggested that you allow 7 mapping registers
# per UNIBUS character device so that you can move 56K of data on each device
# simultaneously.  The rest should be assigned to the block buffer pool.  So,
# if you have a DR-11 and a TM-11, you would leave 14 unassigned for them and
# allocate 16 to the buffer pool.  Since each mapping register addresses 8
# buffers for a 1K file system, NBUF would be 128.  A possible exception would
# be to reduce the buffers to save on data space, as they were 24 bytes per
# header, last time I looked.
# should be 20 for GENERIC, so room for kernel + large program to run.
NBUF		20			# buffer cache, *must* be <= 240

# MAXMEM is the maximum core per process is allowed.  First number
# is Kb.
MAXMEM		(300*16)		# 300K max per process ...

# DIAGNOSTIC does various run-time checks, some of which are pretty
# expensive and at a high priority.  Suggested use is when the kernel
# is crashing and you don't know why, otherwise run with it off.
DIAGNOSTIC	NO			# misc. diagnostic loops and checks

# The following entries used to be part of the kernel configuration, and,
# for various reasons, are no longer modifiable.  They are included here
# for historical information ONLY.  If "YES", they are always included in
# the kernel, if "NO", they are never included.

# ACCT			YES		# process accounting
# DISPLAY		NO		# PDP-11/45/70 display routine
# INSECURE		NO		# don't clear setuid/gid bits on write
# INTRLVE		NO		# file system/disk interleaving
# MENLO_JCL		YES		# job control
# MENLO_KOV		YES		# kernel uses overlaid call sequence
# MENLO_OVLY		YES		# support user process text overlays
# MPX_FILS		NO		# hooks for multiplexed files
# OLDTTY		YES		# old line discipline
# RAND_XO		YES		# rand file exclusive open (flock(2))
# TEXAS_AUTOBAUD	YES		# tty image mode to support autobauding
# UCB_AUTOBOOT		YES		# system is able to reboot itself
# UCB_BHASH		YES		# hashed buffer accessing
# UCB_DBUF		YES		# use one buffer per disk
# UCB_DEVERR		YES		# print device errors in mnemonics
# UCB_ECC		YES		# disk drivers should do ECC
# UCB_FSFIX		YES		# crash resistant filesystems
# UCB_GRPMAST		NO		# group master accounts
# UCB_IHASH		YES		# hashed inode table
# UCB_LOAD		YES		# load average and uptime
# UCB_LOGIN		NO		# login sys call
# UCB_NTTY		YES		# new tty driver
# UCB_PGRP		NO		# count process limit by process group
# UCB_RENICE		YES		# renice system call (setpriority(2))
# UCB_SCRIPT		YES		# shell scripts can specify interpreter
# UCB_SUBM		YES		# "submit" processing (stty(1))
# UCB_SYMLINKS		YES		# symbolic links
# UCB_UPRINTF		YES		# send error messages to user
# UCB_VHANGUP		YES		# revoke control tty access
# UNFAST		NO		# don't use inline.h macro speedups

# DISKMON has been absorbed into UCB_METER.
# DISKMON		UCB_METER	# iostat disk monitoring

# BSLOP was a #define in param.c for the TIU/Spider.  It seems that
# particular printer would DMA a few extra characters for you for free.
# V7 came with a BSLOP of 2, but it was normally set to 0.  Add the
# following #defines to param.c, then change all occurrences of BSIZE
# to bsize.
#
#			/* BSLOP can be 0 unless you have a TIU/Spider*/
#	#define BSLOP	0
#	int	bsize = BSIZE + BSLOP;		/* size of buffers */

# The UCB_NKB flag requires changes to UNIX boot pgms as well as changes to
# dump, restore, icheck, dcheck, ncheck, mkfs.  It includes the options
# previously known as UCB_SMINO (smaller inodes, NADDR = 7) and UCB_MOUNT
# (multiple superblocks per internal buffer).  It's measured in KB byte
# system buffers, it's not just a boolean.  If you're not hearing what
# I'm saying, don't even *think* of changing it.
# UCB_NKB		1		# "n" KB byte system buffers

#########################################
# PERIPHERALS: DISK DRIVES		#
#########################################

NBR		0		# EATON BR1537/BR1711, BR1538A, B, C, D

NHK		2		# RK611, RK06/07

NRAC		1		# NRAD controllers
NRAD		2		# RX50, RC25, RD51/52/53, RA60/80/81

NRK		0		# RK05

NRL		2		# RL01/02

NRX		0		# RX02

NSI		0		# SI 9500 driver for CDC 9766 disks

# Because the disk drive type registers conflict with other DEC
# controllers, you cannot use XP_PROBE for the Ampex 9300 and
# Diva drives.  Read through /sys/pdpuba/hpreg.h and /sys/pdpuba/xp.c
# for information on how to initialize for these drives.
NXPC		1		# NXPD controllers (RH70/RH11 style)
NXPD		2		# RM02/03/05, RP04/05/06, CDC 9766,
				# Ampex 9300, Diva, Fuji 160, SI Eagle.
XP_PROBE	YES		# check drive types at boot

NRAM		0		# RAM disk size (512-byte blocks)

#########################################
# PERIPHERALS: TAPE DRIVES		#
#########################################

NHT		1		# TE16, TU45, TU77

# Setting AVIVTM configures the TM driver for the AVIV 800/1600/6250
# controller (the standard DEC TM only supports 800BPI).  For more details,
# see /sys/pdpuba/tm.c.
NTM		1		# TM11
AVIVTM		YES		# AVIV 800/1600/6250 controller

NTS		1		# TS11

#########################################
# PERIPHERALS: TERMINALS		#
#########################################

# NKL includes both KL11's and DL11's.
# It should always be at least 1, for the console.
NKL		1		# KL11, DL11
NDH		0		# DH11; NDH is in units of boards (16 each)
NDM		0		# DM11; NDM is in units of boards (16 each)
NDHU		0		# DHU11, DHV11
NDZ		0		# DZ11; NDZ is in units of boards (8 each)

#########################################
# PERIPHERALS: OTHER			#
#########################################
NDN		0		# DN11 dialer
NLP		0		# Line Printer
LP_MAXCOL	132		# Maximum number of columns on line printers
NDR		0		# DR11-W

#########################################
# PSEUDO DEVICES, PROTOCOLS, NETWORKING	#
#########################################
# Networking only works with split I/D and SUPERVISOR space, i.e. with the
# 11/44/45/50/53/55/70/73/83/84.  NETHER should be non-zero for networking
# systems using any ethernet.  CHECKSTACK makes sure the networking stack
# pointer and the kernel stack pointer don't collide; it's fairly expensive
# at 4 extra instructions for EVERY function call AND return, always left
# NO unless doing serious debugging.
UCB_NET		NO		# TCP/IP
CHECKSTACK	NO		# Kernel & Supervisor stack pointer checking
NETHER		0		# ether pseudo-device

# Note, PTY's and the select(2) system call do not require the kernel to
# be configured for networking (UCB_NET).  Note that you can allocate PTY's
# in any number (multiples of 8, of 16, even, odd, prime, whatever).  Nothing
# in the kernel cares.  PTY's cost 78 bytes apiece in kernel data space.  You
# should probably have at least 16 since so many applications use them:
# script, jove, window, rlogin, ...
NPTY		16		# pseudo-terminals

# To make the 3Com Ethernet board work correctly, splimp has to be promoted
# to spl6; splfix files that do this are in conf/3Com; the config script
# hopefully does the right thing.
NEC		0		# 3Com Ethernet
NDE		0		# DEUNA
NIL		0		# Interlan Ethernet
NSL		0		# Serial Line IP
NQE		0		# DEQNA

# The following devices are untested in 2.10BSD; some are untested since
# before 2.9BSD.  Some won't even compile.  Most will require modification
# of various Makefiles to include the correct source.  Good luck.
ENABLE34	NO		# if have the ENABLE34 board

NACC		0		# ACC LH/DH ARPAnet IMP interface
PLI		YES		# LH/DH is connected to a PLI

NCSS		0		# DEC/CSS IMP11-A ARPAnet IMP interface
NDMC		0		# DMC11
NEN		0		# Xerox prototype (3 Mb) Ethernet
NHY		0		# Hyperchannel
NIMP		0		# ARPAnet IMP 1822 interface
NPUP		0		# Xerox PUP-I protocol family
NS		0		# Xerox NS (XNS)
NSRI		0		# SRI DR11c ARPAnet IMP
NTB		0		# RS232 interface for Genisco/Hitachi tablets
NVV		0		# V2LNI (Pronet)
.fi

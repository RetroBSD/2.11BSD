.\" Copyright (c) 1980 Regents of the University of California.
.\" All rights reserved.  The Berkeley software License Agreement
.\" specifies the terms and conditions for redistribution.
.\"
.\"	@(#)a.t	2.1 (GTE) 1995/06/16
.\"
.de IR
\fI\\$1\fP\|\\$2
..
.ds LH "Installing/Operating \*(2B
.nr H1 7
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
Because of time constraints the
.B NMOUNT
constant was not moved into the kernel configuration file where it belongs.
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
MSCP (RA) Controllers	2
MSCP (RA) Disks	3
RL01/02 Drives	2
SMD (XP) Controllers	1
SMD (XP) Disks	2
TE16, TU45, TU77 (HT) Tape drives	2
TM11 (TM) Tape drives	2
TS11 (TS) Tape drives	2
TK50 (TMSCP) Tape drives	2
.TE
.PP
The generic kernel adapts automatically to the booted device.  The 'a'
partition on the booted device is automatically made the root filesystem
and the 'b' partition the swap area (except for the RL02 which uses the
second drive).  The size of the swap partition is determined at run
time, the kernel queries the driver for the number of block in the 'b'
partition.  \fBNOTE:\fP If the swap partition is not labeled as being
of type \fIswap\fP the kernel will panic.
.NH 3
GENERIC kernel configuration file
.PP
.ta 8n 16n 24n 32n 40n 48n 56n 72n 80n
.cs R 24
.nf
# Machine configuration file for 2.11BSD distributed kernel.
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
# Split I/D and hardware floating point are required.
#
# Including UNIBUS map support for machines without a UNIBUS will not cause
# a kernel to die.  It simply includes code to support UNIBUS mapping if
# present.
#
# The define UNIBUS_MAP implements kernel support for UNIBUS mapped
# machines.  However, a kernel compiled with UNIBUS_MAP does *not* have to
# be run on a UNIBUS machine.  The define simply includes support for UNIBUS
# mapping if the kernel finds itself on a machine with UNIBUS mapping.
UNIBUS_MAP	YES			# include support for UNIBUS mapping

# The define Q22 has been removed.  The references to it were incorrect
# (i.e. using it to distinguish between an Emulex CS02 and a DH11) or
# inappropriate (the if_il.c driver should have been checking if a Unibus
# Map was present at runtime).

#LINEHZ		50		# clock frequency European
LINEHZ		60			# clock frequency USA

# PDP-11 machine type; allowable values are GENERIC, 44, 70, 73.  GENERIC 
# should only be used to build a distribution kernel.  The only use of this
# option is to select the proper in-line PS instructions (references to the
# PSW use 'spl', 'mfps/mtps' or 'movb' instructions depending on the cpu type).
PDP11		GENERIC		# distribution kernel
#PDP11		44			# PDP-11/44
#PDP11		70			# PDP-11/70,45,50,55
#PDP11		73			# PDP-11/73,53,83,93,84,94

#########################################
# GENERAL SYSTEM PARAMETERS		#
#########################################

IDENT		GENERIC			# machine name
MAXUSERS	4			# maxusers on machine

# BOOTDEV is the letter combination denoting the autoboot device,
# or NONE if not using the autoboot feature.
BOOTDEV		NONE		# don't autoboot
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
#DUMPROUTINE	tmsdump			# tms driver dump routine

#########################################
# KERNEL CONFIGURATION			#
#########################################

BADSECT		NO		# bad-sector forwarding
EXTERNALITIMES	YES		# map out inode time values
UCB_CLIST	NO			# clists moved from kernel data space
NOKA5		NO			# KA5 not used except for buffers
					# and clists (_end < 0120000);
QUOTA		NO			# dynamic file system quotas
					# NOTE -- *very* expensive

# UCB_METER is fairly expensive, but various programs (iostat, vmstat, etc)
# use it.
UCB_METER	NO			# vmstat performance metering

# NBUF is the size of the buffer cache, and is directly related to the UNIBUS
# mapping registers.  There are 32 total mapping registers, of which 30 are
# available.  The 0'th is used for CLISTS, and the 31st is used for the I/O
# page on some PDP's.  It's suggested that you allow 7 mapping registers
# per UNIBUS character device so that you can move 56K of data on each device
# simultaneously.  The rest should be assigned to the block buffer pool.  So,
# if you have a DR-11 and a TM-11, you would leave 14 unassigned for them and
# allocate 16 to the buffer pool.  Since each mapping register addresses 8
# buffers for a 1K file system, NBUF would be 128.  A possible exception would
# be to reduce the buffers to save on data space, as they were 24 bytes each
# Should be 'small' for GENERIC, so room for kernel + large program to run.
NBUF		32			# buffer cache, *must* be <= 240

# DIAGNOSTIC does various run-time checks, some of which are pretty
# expensive and at a high priority.  Suggested use is when the kernel
# is crashing and you don't know why, otherwise run with it off.
DIAGNOSTIC	NO			# misc. diagnostic loops and checks

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

NHT		2		# TE16, TU45, TU77

# Setting AVIVTM configures the TM driver for the AVIV 800/1600/6250
# controller (the standard DEC TM only supports 800BPI).  For more details,
# see /sys/pdpuba/tm.c.
NTM		2		# TM11
AVIVTM		YES		# AVIV 800/1600/6250 controller

NTS		2		# TS11

NTMSCP		2		# TMSCP controllers
NTMS		2		# TMSCP drives
TMSCP_DEBUG	NO		# debugging code in TMSCP drive (EXPENSIVE)

#########################################
# PERIPHERALS: TERMINALS		#
#########################################

# NKL includes both KL11's and DL11's.
# It should always be at least 1, for the console.
NKL		1		# KL11, DL11
NDH		0		# DH11; NDH is in units of boards (16 each)
CS02		NO		# DH units above are really Emulex CS02 
				# boards on a 22bit Qbus.
NDM		0		# DM11; NDM is in units of boards (16 each)
NDHU		0		# DHU11
NDHV		0		# DHV11
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
INET		NO		# TCP/IP
CHECKSTACK	NO		# Kernel & Supervisor stack pointer checking
NETHER		0		# ether pseudo-device

# Note, PTY's and the select(2) system call do not require the kernel to
# be configured for networking (INET).  Note that you can allocate PTY's
# in any number (multiples of 8, of 16, even, odd, prime, whatever).  Nothing
# in the kernel cares.  PTY's cost 78 bytes apiece in kernel data space.  You
# should probably have at least 8-10 since several applications use them:
# script, jove, window, rlogin, ...
NPTY		0		# pseudo-terminals - GENERIC sys needs NONE

# To make the 3Com Ethernet board work correctly, splimp has to be promoted
# to spl6; splfix files that do this are in conf/3Com; the config script
# does the right thing.
NEC		0		# 3Com Ethernet
NDE		0		# DEUNA/DELUA
NIL		0		# Interlan Ethernet
NSL		0		# Serial Line IP
NQE		0		# DEQNA
NQT		0		# DEQTA (DELQA-YM, DELQA-PLUS)
NVV		0		# V2LNI (Pronet)
NACC		0		# ACC LH/DH ARPAnet IMP interface
PLI		NO		# LH/DH is connected to a PLI
NIMP		0		# ARPAnet IMP 1822 interface

# The following are untested in 2.11BSD; some are untested since before 2.9BSD
# Some won't even compile.  Most will require modification.  Good luck.
ENABLE34	NO		# if have the ENABLE34 board

NCSS		0		# DEC/CSS IMP11-A ARPAnet IMP interface
NDMC		0		# DMC11
NEN		0		# Xerox prototype (3 Mb) Ethernet
NHY		0		# Hyperchannel
NS		0		# Xerox NS (XNS)
NSRI		0		# SRI DR11c ARPAnet IMP
NTB		0		# RS232 interface for Genisco/Hitachi tablets

# Defining FPSIM to YES compiles a floating point simulator into the kernel
# which will catch floating point instruction traps from user space.  This
# doesn't work at present.
FPSIM		NO		# floating point simulator

# To enable profiling, the :splfix script must be changed to use spl6 instead
# of spl7 (see conf/:splfix.profile), also, you have to have a machine with a
# supervisor PAR/PDR pair, i.e. an 11/44/45/50/53/55/70/73/83/84, as well
# as both a KW11-L and a KW11-P.
#
# Note that profiling is not currently working.  We don't have any plans on
# fixing it, so this is essentially a non-supported feature.
PROFILE		NO		# system profiling with KW11P clock

INGRES		NO		# include the Ingres lock driver
.cs R

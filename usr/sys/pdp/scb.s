/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)scb.s	1.3 (2.11BSD GTE) 1/1/93
 */

#include "DEFS.h"
#include "../machine/trap.h"
#include "../machine/mch_iopage.h"
#include "../machine/koverlay.h"	/* for OVLY_TABLE_BASE */

#include "acc.h"
#include "css.h"
#include "de.h"
#include "dh.h"
#include "dhu.h"
#include "dhv.h"
#include "dn.h"
#include "dr.h"
#include "dz.h"
#include "ec.h"
#include "hk.h"
#include "ht.h"
#include "il.h"
#include "lp.h"
#include "ra.h"
#include "rk.h"
#include "rl.h"
#include "br.h"
#include "rx.h"
#include "si.h"
#include "sri.h"
#include "tm.h"
#include "ts.h"
#include "tms.h"
#include "xp.h"
#include "vv.h"

/*
 * Reference the global symbol "_end" so that ld(1) will define it for us.
 */
.globl _end

sup = 40000				/* current supervisor previous kernel */
br4 = 200				/* PS interrupt priority masks */
br5 = 240
br6 = 300
br7 = 340


#ifdef KERN_NONSEP
	.text				/* vectors at text zero */
#else
	.data				/* vectors at data zero */
#endif

ZERO:
#define	SETDOT(n)			. = ZERO+n
#define	TRAP(handler, pri)		.globl	handler; handler; pri;
#define	DEVTRAP(vec, handler, pri)	SETDOT(vec); handler; pri;


SETDOT(0)
#ifdef KERN_NONSEP
	/*
	 * If vectors at 110 and 444 are unused, autoconfig will set these
	 * to something more reasonable.  On jump, this will branch to 112,
	 * which branches to 50.  On trap, will vector to 444, where a
	 * T_ZEROTRAP will be simulated.
	 */
	42			/* illegal instruction if jump */
	777			/* trace trap at high priority if trap */
#else
	TRAP(trap,	br7+T_ZEROTRAP)	/* trap-to-zero trap */
#endif

SETDOT(4)				/* trap vectors */
	TRAP(trap,	br7+T_BUSFLT)	/* bus error */
	TRAP(trap,	br7+T_INSTRAP)	/* illegal instruction */
	TRAP(trap,	br7+T_BPTTRAP)	/* bpt-trace trap */
#ifdef INET
	TRAP(iothndlr,	br7+T_IOTTRAP)	/* network uses iot */
#else
	TRAP(trap,	br7+T_IOTTRAP)	/* iot trap */
#endif
	TRAP(powrdown,	br7+T_POWRFAIL)	/* power fail */
	TRAP(emt,	br7+T_EMTTRAP)	/* emulator trap */
	TRAP(start,	br7+T_SYSCALL)	/* system (overlaid by 'syscall') */

#ifdef KERN_NONSEP
SETDOT(40)				/* manual reboot entry */
	jmp	do_panic
#endif

SETDOT(44)				/* manual dump entry */
	jmp	dump

#ifdef KERN_NONSEP
SETDOT(50)				/* handler for jump-to-zero panic. */
	mov	$zjmp, -(sp)
	jsr	pc, _panic
#endif

	DEVTRAP(60,	cnrint,	br4)	/* KL11 console */
	DEVTRAP(64,	cnxint,	br4)

	DEVTRAP(100,	hardclock, br6)	/* KW11-L clock */

#ifdef PROFILE
	DEVTRAP(104,	_sprof,	br7)	/* KW11-P clock */
#else
	DEVTRAP(104,	hardclock, br6)
#endif


SETDOT(114)
	TRAP(trap,	br7+T_PARITYFLT) /* 11/70 parity fault */


#if NDE > 0				/* DEUNA */
	DEVTRAP(120,	deintr,	sup|br5)
#endif

#if NCSS > 0				/* IMP-11A */
	DEVTRAP(124,	cssrint,sup|br5)
	/* note that the transmit interrupt vector is up at 274 ... */
#endif

#if NDR > 0				/* DR-11W */
	DEVTRAP(124,	drintr,	br5)
#endif

#if NRAC > 0				/* RQDX? (RX50,RD51/52/53) */
					/* UDA50 (RA60/80/81), KLESI (RA25) */
	DEVTRAP(154,	raintr,	br5)
#endif

#if NRL > 0				/* RL01/02 */
	DEVTRAP(160,	rlintr,	br5)
#endif

#if NSI > 0				/* SI 9500 -- CDC 9766 disks */
	DEVTRAP(170,	siintr,	br5)
#endif

#if NHK > 0				/* RK611, RK06/07 */
	DEVTRAP(210,	hkintr,	br5)
#endif

#if NRK > 0				/* RK05 */
	DEVTRAP(220,	rkintr,	br5)
#endif


SETDOT(240)
	TRAP(trap,	br7+T_PIRQ)		/* program interrupt request */
	TRAP(trap,	br7+T_ARITHTRAP)	/* floating point */
	TRAP(trap,	br7+T_SEGFLT)		/* segmentation violation */

#if NBR > 0
	DEVTRAP(254,	brintr, br5)	/* EATON BR1537 or EATON BR1711 */
#endif

#if NXPD > 0				/* RM02/03/05, RP04/05/06 */
					/* DIVA, SI Eagle */
	DEVTRAP(254,	xpintr,	br5)
#endif

#if NRX > 0				/* RX01 */
	DEVTRAP(264,	rxintr,	br5)
#endif

#if NACC > 0				/* ACC LH/DH-11 */
	DEVTRAP(270,	accrint, sup|br5)
	DEVTRAP(274,	accxint, sup|br5)
#endif

#if NCSS > 0				/* IMP-11A */
	/* note that the receive interrupt vector is down at 124 ... */
	DEVTRAP(274,	cssxint,sup|br5)
#endif

#if NIL > 0				/* Interlan Ethernet */
	DEVTRAP(340,	ilrint,	sup|br5)
	DEVTRAP(344,	ilcint,	sup|br5)
#endif

#if NVV > 0				/* V2LNI */
	DEVTRAP(350,	vvrint,	sup|br5)
	DEVTRAP(354,	vvxint,	sup|br5)
#endif

#if NEC > 0				/* 3Com ethernet */
	/*
	 * These are almost certainly wrong for any given site since the
	 * 3Com seems to be somewhat randomly configured.  Pay particular
	 * attention to the interrupt priority levels: if they're too low
	 * you'll get recursive interrupts; if they're too high you'll lock
	 * out important interrupts (like the clock).
	 */
	DEVTRAP(400,	ecrint,	sup|br6)
	DEVTRAP(404,	eccollide,sup|br4)
	DEVTRAP(410,	ecxint,	sup|br6)
#endif

#if NSRI > 0				/* SRI DR11-C ARPAnet IMP */
	DEVTRAP(500,	srixint, sup|br5)
	DEVTRAP(504,	srirint, sup|br5)
#endif


/*
 * End of floating vectors.  Endvec should be past vector space if NONSEP,
 * should be at least 450.
 *
*/
SETDOT(1000)
CONST(GLOBAL, endvec, .)

/*
 * The overlay tables are initialized by boot.  Ovhndlr, cret and call use
 * them to perform kernel overlay switches.  The tables contain an arrays of
 * segment addresses and descriptors.  Note: don't use SPACE macro as it
 * allocates it's space in bss, not data space ...
 */
. = ZERO+OVLY_TABLE_BASE
.globl	ova, ovd
ova:	.=.+40				/* overlay addresses */
ovd:	.=.+40				/* overlay descriptors */

/*
 * FLASH!  for overlaid programs /boot kindly lets us know where our
 * load image stops by depositing a value at the end of the overlay tables.
 * Needless to say this had been clobbering something all along, but the
 * effect was rather nasty (crash) when the 'last interrupt vector' location
 * was overwritten!  Not sure whether to fix /boot or leave room here, so
 * for now just add a "pad" word.
*/
INT(LOCAL, physend, 0)

/*
 * _lastiv is used for assigning vectors to devices which have programmable
 * vectors.  Usage is to decrement _lastiv by 4 before use.  The routine 
 * _nextiv (in mch_xxx.s) will do this, returning the assigned vector in r0.
 */
INT(GLOBAL, _lastiv, endvec)

	.text
TEXTZERO:				/ base of system program text

#ifndef KERN_NONSEP
	/*
	 * this is text location 0 for separate I/D kernels.
	 */
	mov	$zjmp,-(sp)
	jsr	pc,_panic
	/*NOTREACHED*/
#endif

STRING(LOCAL, zjmp, <jump to 0\0>)

#ifndef KERN_NONSEP
/*
 * Unmap is called from _doboot to turn off memory management.  The "return"
 * is arranged by putting a jmp at unmap+2 (data space).
 */
ASENTRY(unmap)
	reset
	/*
	 * The next instruction executed is from unmap+2 in physical memory,
	 * which is unmap+2 in data space.
	 */
#endif

/*
 * Halt cpu in its tracks ...
 */
ENTRY(halt)
	halt
	/* NOTREACHED */

/*
 * CGOOD and CBAD are used by autoconfig.  All unused vectors are set to CBAD
 * before probing the devices.
 */
ASENTRY(CGOOD)
	mov	$1,_conf_int
	rtt
ASENTRY(CBAD)
	mov	$-1,_conf_int
	rtt

/*
 * Routine to call panic, take dump and reboot; entered by manually loading
 * the PC with 040 and continuing.  Note that putting this here is calculated
 * to waste as little text space as possible for separate I&D kernels.
 * Currently the zero jmp, unmap, halt, CGOOD, and CBAD routines leave the
 * separate I&D assembly counter at 034, so the ". = 40" below only wastes
 * 4 bytes.  This is safe since any attempt to set the assembly counter back
 * will cause an error indicating that the assembly counter is higher than
 * 040 meaning things need to be rearranged ...
 */
#ifndef KERN_NONSEP
. = TEXTZERO+040
#endif

do_panic:
	mov	$1f,-(sp)
	STRING(LOCAL, 1, <forced from console\0>)
	jsr	pc, _panic
	/* NOTREACHED */


/*
 * Start of locore interrupt entry thunks.
 */
#define	HANDLER(handler)	.globl _/**/handler; \
				handler: jsr r0,call; jmp _/**/handler

	HANDLER(cnrint)			/* KL-11, DL-11 */
	HANDLER(cnxint)

	HANDLER(hardclock)

#if NDR > 0				/* DR-11W */
	HANDLER(drintr)
#endif

#if NRAC > 0				/* RQDX? (RX50,RD51/52/53) */
	HANDLER(raintr)			/* UDA50 (RA60/80/81), KLESI (RA25) */
#endif

#if NRL > 0				/* RL01/02 */
	HANDLER(rlintr)
#endif

#if NSI > 0				/* SI 9500 -- CDC 9766 disks */
	HANDLER(siintr)
#endif

#if NLP > 0				/* Line Printer */
	HANDLER(lpintr)
#endif

#if NHK > 0				/* RK611, RK06/07 */
	HANDLER(hkintr)
#endif

#if NRK > 0				/* RK05 */
	HANDLER(rkintr)
#endif

#if NBR > 0
	HANDLER(brintr)			/* EATON BR1537/BR1711 */
#endif

#if NXPD > 0				/* RM02/03/05, RP04/05/06 */
	HANDLER(xpintr)			/* DIVA, SI Eagle */
#endif

#if NRX > 0				/* RX01/02 */
	HANDLER(rxintr)
#endif

#if NHT > 0				/* TJU77, TWU77, TJE16, TWE16 */
	HANDLER(htintr)
#endif

#if NTM > 0				/* TM-11 */
	HANDLER(tmintr)
#endif

#if NTS > 0				/* TS-11 */
	HANDLER(tsintr)
#endif

#if NTMSCP > 0
	HANDLER(tmsintr)		/* TMSCP (TU81/TK50) */
#endif

#if NDH > 0				/* DH-11 */
	HANDLER(dhrint)
	HANDLER(dhxint)
#endif

#if NDM > 0				/* DM-11 */
	HANDLER(dmintr)
#endif

#if NDHU > 0				/* DHU */
	HANDLER(dhurint)
	HANDLER(dhuxint)
#endif

#if NDHV > 0				/* DHV */
	HANDLER(dhvrint)
	HANDLER(dhvxint)
#endif

#if NDN > 0				/* DN11 */
	HANDLER(dnint)
#endif

#if NDZ > 0				/* DZ */
	HANDLER(dzrint)
#endif

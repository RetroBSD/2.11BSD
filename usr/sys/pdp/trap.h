/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)trap.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * Trap type values
 */
#define	T_BUSFLT	000		/* bus error */
#define	T_INSTRAP	001		/* illegal instruction */
#define	T_BPTTRAP	002		/* bpt/trace trap */
#define	T_IOTTRAP	003		/* iot trap */
#define	T_POWRFAIL	004		/* power failure */
#define	T_EMTTRAP	005		/* emt trap */
#define	T_SYSCALL	006		/* system call */
#define	T_PIRQ		007		/* program interrupt request */
#define	T_ARITHTRAP	010		/* floating point trap */
#define	T_SEGFLT	011		/* segmentation fault */
#define	T_PARITYFLT	012		/* parity fault */
#define	T_SWITCHTRAP	014		/* process switch */
/*
 * T_RANDOMTRAP is used by autoconfig/do_config.c when it substitutes
 * the trap routine for the standard device interrupt routines when
 * probing a device in case the device probe routine causes an interrupt.
 * Ignored in trap.c
 */
#define	T_RANDOMTRAP	016		/* random trap */
#define	T_ZEROTRAP	017		/* trap to zero */

/*
 * User mode flag added to trap code passed to trap routines
 * if trap is from user space.
 */
#define	USER		020
#define	SUPV		040

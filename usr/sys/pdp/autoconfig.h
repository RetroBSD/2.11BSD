/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)autoconfig.h	1.1 (2.10BSD Berkeley) 12/1/86
 */

/*
 * Possible error codes of the auto configuration program
 */
#define	AC_OK		0	/* Everything A-OK for system go */
#define	AC_SETUP	1	/* Error in initializing autoconf program */
#define	AC_SINGLE	2	/* Non serious error, come to single user */

#define	ACI_BADINTR	-1	/* device interrupted through wrong vector */
#define	ACP_EXISTS	1	/* device exists */
#define	ACI_GOODINTR	1	/* interrupt OK */
#define	ACP_IFINTR	0	/* device exists if interrupts OK */
#define	ACI_NOINTR	0	/* device didn't interrupt */
#define	ACP_NXDEV	-1	/* no such device */

#define	YES		1	/* general yes */
#define	NO		0	/* general no */

/*
 * Magic number to verify that autoconfig runs only once.
 */
#define	CONF_MAGIC	0x1960

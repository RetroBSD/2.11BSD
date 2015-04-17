/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)cpu.h	1.5 (2.11BSD GTE) 1998/4/3
 */

/*
 * Define others as needed.  The old practice of defining _everything_
 * for _all_ models and then attempting to 'ifdef' the mess on a particular
 * cputype was simply too cumbersome (and didn't work when moving kernels
 * between cpu types).
*/
#define	PDP1170_LEAR	((physadr) 0177740)

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_TMSCP		2	/* tmscp debugging */
#define	CPU_MSCP		3	/* mscp debugging/logging */
#define	CPU_MAXID		4	/* number of valid machdep ids */

#ifndef	KERNEL
#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ "tmscp", CTLTYPE_NODE }, \
	{ "mscp", CTLTYPE_NODE }, \
}
#endif

#define	TMSCP_CACHE	1		/* enable/disable drive cache */
#define	TMSCP_PRINTF	2		/* get/set print flag */
#define	TMSCP_MAXID	3		/* number of valid TMSCP ids */

#define	MSCP_PRINTF	1		/* get/set print/logging flag */
#define	MSCP_MAXID	2		/* number of valid MSCP ids */

#ifndef	KERNEL
#define	TMSCP_NAMES { \
	{ 0, 0 }, \
	{ "cache", CTLTYPE_INT }, \
	{ "printf", CTLTYPE_INT }, \
}

#define	MSCP_NAMES { \
	{ 0, 0 }, \
	{ "printf", CTLTYPE_INT }, \
}
#endif

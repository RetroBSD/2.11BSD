/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)param.h	1.6 (2.11BSD) 1999/9/5
 */

#define	BSD	211		/* 2.11 * 10, as cpp doesn't do floats */

#include <sys/localopts.h>
#include <sys/stddef.h>		/* for 'offsetof' */

/*
 * Machine type dependent parameters.
 */
#include <machine/machparam.h>

/*
 * Machine-independent constants
 */
#define	NMOUNT	6		/* number of mountable file systems */
#define	MAXUPRC	20		/* max processes per user */
#define	NOFILE	30		/* max open files per process */
#define	CANBSIZ	256		/* max size of typewriter line */
#define	NCARGS	5120		/* # characters in exec arglist */
#define	NGROUPS	16		/* max number groups */

#define	NOGROUP	65535		/* marker for empty group set member */

/*
 * Priorities
 */
#define	PSWP	0
#define	PINOD	10
#define	PRIBIO	20
#define	PRIUBA	24
#define	PZERO	25
#define	PPIPE	26
#define	PSOCK	26
#define	PWAIT	30
#define	PLOCK	35
#define	PPAUSE	40
#define	PUSER	50

#define	NZERO	0

#define	PRIMASK	0xff
#define	PCATCH	0x100

/*
 * Signals
 */
#include <signal.h>

#define	NBPW	sizeof(int)	/* number of bytes in an integer */

#ifndef	NULL
#define	NULL	0
#endif
#define	CMASK	026		/* default mask for file creation */
#define	NODEV	(dev_t)(-1)

#define	CLBYTES		(CLSIZE*NBPG)
#define	CLOFSET		(CLBYTES-1)
#define	claligned(x)	((((int)(x))&CLOFSET)==0)
#define	CLOFF		CLOFSET
#define	CLSHIFT		(PGSHIFT + CLSIZELOG2)

/* round a number of clicks up to a whole cluster */
#define	clrnd(i)	(((i) + (CLSIZE-1)) &~ ((long)(CLSIZE-1)))

/* CBLOCK is the size of a clist block, must be power of 2 */
#define	CBLOCK	32
#define	CBSIZE	(CBLOCK - sizeof(struct cblock *))	/* data chars/clist */
#define	CROUND	(CBLOCK - 1)				/* clist rounding */

#include	<sys/types.h>

/*
 * File system parameters and macros.
 *
 * The file system is made out of blocks of most MAXBSIZE units.
 */
#define	MAXBSIZE	1024

/*
 * MAXPATHLEN defines the longest permissable path length
 * after expanding symbolic links. It is used to allocate
 * a temporary buffer from the buffer pool in which to do the
 * name expansion, hence should be a power of two, and must
 * be less than or equal to MAXBSIZE.
 * MAXSYMLINKS defines the maximum number of symbolic links
 * that may be expanded in a path name. It should be set high
 * enough to allow all legitimate uses, but halt infinite loops
 * reasonably quickly.
 */
#define MAXPATHLEN	256
#define MAXSYMLINKS	8

/*
 * Macros for fast min/max.
 */
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))

/*
 * Macros for counting and rounding.
 */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

/*
 * Maximum size of hostname recognized and stored in the kernel.
 */
#define MAXHOSTNAMELEN	64

#if defined(KERNEL) && defined(INET)
#include "../machine/net_mac.h"
#endif

/*
 * MAXMEM is the maximum core per process is allowed.  First number is Kb.
*/
#define	MAXMEM		(300*16)

/*
 * MAXLOGNAME should be >= UT_NAMESIZE (see <utmp.h>)
 */
#define	MAXLOGNAME	16		/* max login name length */

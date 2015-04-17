/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)machparam.h	1.4 (2.11BSD GTE) 1998/9/15
 */

/*
 * Machine dependent constants for PDP.
 */
#ifndef ENDIAN
/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define	LITTLE	1234		/* least-significant byte first (vax) */
#define	BIG	4321		/* most-significant byte first */
#define	PDP	3412		/* LSB first in word, MSW first in long (pdp) */
#define	ENDIAN	PDP		/* byte order on pdp */

/*
 * Macros for network/external number representation conversion.
 */
#if ENDIAN == BIG && !defined(lint)
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)
#else
#include <sys/types.h>
u_short	ntohs(), htons();
u_long	ntohl(), htonl();
#endif

#define	CHAR_BIT	NBBY
#define	CHAR_MAX	0x7f
#define	CHAR_MIN	0x80
#define	CLK_TCK		60			/* for times() */
#define	INT_MAX		0x7fff
#define	INT_MIN		0x8000
#define	LONG_MAX	0x7fffffff
#define	LONG_MIN	0x80000000
#define	SCHAR_MAX	0x7f
#define	SCHAR_MIN	0x80
#define	SHRT_MAX	0x7fff
#define	SHRT_MIN	0x8000
#define	UCHAR_MAX	0xff
#define	UINT_MAX	((unsigned int)0xffff)
#define	ULONG_MAX	0x7fffffff
#define	USHRT_MAX	((unsigned short)0xffff)

#define	NBPG		512		/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	PGSHIFT		9		/* LOG2(NBPG) */

#define	DEV_BSIZE	1024
#define	DEV_BSHIFT	10		/* log2(DEV_BSIZE) */
#define	DEV_BMASK	0x3ffL		/* DEV_BSIZE - 1 */

#define	CLSIZE		2
#define	CLSIZELOG2	1

#define	SSIZE	20			/* initial stack size (*64 bytes) */
#define	SINCR	20			/* increment of stack (*64 bytes) */

/* machine dependent stuff for core files */
#define	TXTRNDSIZ	8192L
#define	stacktop(siz)	(0x10000L)
#define	stackbas(siz)	(0x10000L-(siz))

/*
 * User area: a user structure, followed by a stack for the networking code
 * (running in supervisor space on the PDP-11), followed by the kernel
 * stack.  The numbers for NET_SSIZE and KERN_SSIZE are determined
 * empirically.
 *
 * Note that the SBASE and STOP constants are only used by the assembly code,
 * but are defined here to localize information about the user area's
 * layout (see pdp/genassym.c).  Note also that a networking stack is always
 * allocated even for non-networking systems.  This prevents problems with
 * applications having to be recompiled for networking versus non-networking
 * systems.
 */
#define	NET_SSIZE	16	/* size of the network stack (*64) */
#define	KERN_SSIZE	32	/* size of the kernel stack (*64) */
#define	USIZE		(btoc(sizeof(struct user)) + NET_SSIZE + KERN_SSIZE)

#define	NET_SBASE	ctob(btoc(sizeof(struct user)))
#define	NET_STOP	(NET_SBASE + ctob(NET_SSIZE))
#define	KERN_SBASE	NET_STOP
#define	KERN_STOP	(KERN_SBASE + ctob(KERN_SSIZE))

/*
 * Some macros for units conversion
 */
/* Core clicks (64 bytes) to segments and vice versa */
#define	ctos(x)	(((x)+127)/128)
#define	stoc(x)	((x)*128)

/* Core clicks (64 bytes) to disk blocks */
#define	ctod(x)	(((x)+7)>>3)

/* clicks to bytes */
#define	ctob(x)	((x)<<6)

/* bytes to clicks */
#define	btoc(x)	((((unsigned)(x)+63)>>6))

/* these should be fixed to use unsigned longs, if we ever get them. */
#define	btodb(bytes)		/* calculates (bytes / DEV_BSIZE) */ \
	((long)(bytes) >> DEV_BSHIFT)
#define	dbtob(db)		/* calculates (db * DEV_BSIZE) */ \
	((long)(db) << DEV_BSHIFT)

/* clicks to KB */
#define	ctok(x)	(((x)>>4)&07777)

/*
 * Macros to decode processor status word.
 */
#define	SUPVMODE(ps)	(((ps) & PSL_CURMOD) == PSL_CURSUP)
#define	USERMODE(ps)	(((ps) & PSL_USERSET) == PSL_USERSET)
#define	BASEPRI(ps)	(((ps) & PSL_IPL) == 0)

#define	DELAY(n)	{ long N = ((long)(n))<<1; while (--N > 0); }

/*
 * Treat ps as byte, to allow restoring value from mfps/movb.
 * (see splfix.*.sed)
 */
#define	PS_LOBYTE	((char *)0177776)
#define	splx(ops)	(*PS_LOBYTE = ((char)(ops)))

/*
 * high int of a long
 * low int of a long
 */
#define	hiint(long)	(((int *)&(long))[0])
#define	loint(long)	(((int *)&(long))[1])

/*
 * SUPERADD is used to distinguish a supervisor-mode address from a
 * kernel mode address to insure uniqueness over both address spaces.
 */
#define	SUPERADD(add)	((int)(add)|01)
#define	KERNELADD(add)	((int)(add)&~01)
#define	ISSUPERADD(add)	((int)(add)&01)

#endif ENDIAN

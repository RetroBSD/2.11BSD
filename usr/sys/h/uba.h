/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)uba.h	1.2 (2.11BSD GTE) 1/3/93
 */

/*
 *	Structure to access UNIBUS map registers.
 */

struct	ubmap	{
	short	ub_lo;
	short	ub_hi;
};

#ifdef UCB_METER
/*
 *	Structure for metering mapalloc performance.
 */
struct	ubmeter {
	long	ub_calls;	/* total # of calls to mapalloc */
	long	ub_remaps;	/* total # of buffer remappings */
	long	ub_fails;	/* total # of allocation failures */
	long	ub_pages;	/* total # of pages allocated */
};
#endif

#define	UBMAP	((struct ubmap *) 0170200)
#define	UBPAGE	020000			/* size of UNIBUS map segment */

/*
 *	BUF_UBADDR is the UNIBUS address of buffers
 *	if we have a UNIBUS map, as distinguished from bpaddr,
 *	which is the physical address in clicks.
 */
#define	BUF_UBADDR	020000

/*
 *	Bytes to UNIBUS pages.
 */
#define	btoub(b)	((((long)(b)) + ((long)(UBPAGE - 1))) / ((long)UBPAGE))

/*
 *	Number of UNIBUS registers required by n objects of size s.
 */
#define	nubreg(n,s)	(((long) (n) * (long) (s) +		\
			((long) (UBPAGE - 1))) / (long) UBPAGE)

/*
 *	Set UNIBUS register r to point at physical address p (in bytes).
 */
#define	setubregno(r,p)	{					\
				UBMAP[r].ub_lo	= loint(p);	\
				UBMAP[r].ub_hi	= hiint(p);	\
			}

/*
 *	Point the appropriate UNIBUS register at a kernel
 *	virtual data address (in clicks).  V must be less
 *	than btoc(248K).
 */
#define	pointubreg(v,sep)	{	ubadr_t	x; \
					short	regno; \
					regno	= ((v) >> 7) & 037; \
					x	= (ubadr_t) (v) & ~01; \
					UBMAP[regno].ub_lo	= loint (x); \
					UBMAP[regno].ub_hi	= hiint (x); \
				}

#ifdef KERNEL
extern	short	ubmap;			/* Do we have UNIBUS registers? */
extern	memaddr	bpaddr;			/* physical click-address of buffers */
extern	struct map	ub_map[];
extern	int	ub_wantmr;
#endif

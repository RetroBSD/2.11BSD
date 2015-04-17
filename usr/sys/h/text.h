/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)text.h	1.2 (2.11BSD GTE) 1/19/95
 */

#ifndef	_SYS_TEXT_H_
#define	_SYS_TEXT_H_

/*
 * Text structure.			XXX REF COUNT should be short
 * One allocated per pure
 * procedure on swap device.
 * Manipulated by text.c
 */
struct text
{
	struct	text *x_forw;	/* forward link in free list */
	struct	text **x_back;	/* backward link in free list */
	short	x_daddr;	/* segment's disk address */
	short	x_caddr;	/* core address, if loaded */
	size_t	x_size;		/* size (clicks) */
	struct	inode *x_iptr;	/* inode of prototype */
	u_char	x_count;	/* reference count */
	u_char	x_ccount;	/* number of loaded references */
	u_char	x_flag;		/* traced, written flags */
	char	dummy;		/* room for one more */
};

#if defined(KERNEL) && !defined(SUPERVISOR)
extern struct	text text[], *textNTEXT;
int	ntext;
#endif

#define	XTRC	0x01		/* Text may be written, exclusive use */
#define	XWRIT	0x02		/* Text written into, must swap out */
#define	XLOAD	0x04		/* Currently being read from file */
#define	XLOCK	0x08		/* Being swapped in or out */
#define	XWANT	0x10		/* Wanted for swapping */
#define	XPAGI	0x20		/* Page in on demand from inode */
#define	XUNUSED	0x40		/* unused since swapped out for cache */

/* arguments to xswap: */
#define	X_OLDSIZE	(-1)	/* the old size is the same as current */
#define	X_DONTFREE	0	/* save core image (for parent in newproc) */
#define	X_FREECORE	1	/* free core space after swap */

/*
 * Text table statistics
 */
struct xstats {
	u_long	alloc;			/* calls to xalloc */
	u_long	alloc_inuse;		/*	found in use/sticky */
	u_long	alloc_cachehit;		/*	found in cache */
	u_long	alloc_cacheflush;	/*	flushed cached text */
	u_long	alloc_unused;		/*	flushed unused cached text */
	u_long	free;			/* calls to xfree */
	u_long	free_inuse;		/*	still in use/sticky */
	u_long	free_cache;		/*	placed in cache */
	u_long	free_cacheswap;		/*	swapped out to place in cache */
};
#endif /* _SYS_TEXT_H_ */

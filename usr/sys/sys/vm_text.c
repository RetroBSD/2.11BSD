/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_text.c	1.2 (2.11BSD GTE) 11/26/94
 */

#include "param.h"
#include "../machine/seg.h"

#include "map.h"
#include "user.h"
#include "proc.h"
#include "text.h"
#include "inode.h"
#include "buf.h"
#include "uio.h"
#include "systm.h"

#define X_LOCK(xp) { \
	while ((xp)->x_flag & XLOCK) { \
		(xp)->x_flag |= XWANT; \
		sleep((caddr_t)(xp), PSWP); \
	} \
	(xp)->x_flag |= XLOCK; \
}
#define	XUNLOCK(xp) { \
	if ((xp)->x_flag & XWANT) \
		wakeup((caddr_t)(xp)); \
	(xp)->x_flag &= ~(XLOCK|XWANT); \
}
#define FREE_AT_HEAD(xp) { \
	(xp)->x_forw = xhead; \
	xhead = (xp); \
	(xp)->x_back = &xhead; \
	if (xtail == &xhead) \
		xtail = &(xp)->x_forw; \
	else \
		(xp)->x_forw->x_back = &(xp)->x_forw; \
}
#define FREE_AT_TAIL(xp) { \
	(xp)->x_back = xtail; \
	*xtail = (xp); \
	xtail = &(xp)->x_forw; \
	/* x_forw is NULL */ \
}
#define	ALLOC(xp) { \
	*((xp)->x_back) = (xp)->x_forw; \
	if ((xp)->x_forw) \
		(xp)->x_forw->x_back = (xp)->x_back; \
	else \
		xtail = (xp)->x_back; \
	(xp)->x_forw = NULL; \
	(xp)->x_back = NULL; \
}

/*
 * We place free text table entries on a free list.
 * All text images are treated as "sticky,"
 * and are placed on the free list (as an LRU cache) when unused.
 * They may be reclaimed from the free list until reused.
 * Files marked sticky are locked into the table, and are never freed.
 */
struct	text *xhead, **xtail;		/* text table free list */
#ifdef UCB_METER
struct	xstats xstats;			/* cache statistics */
#endif

/*
 * initialize text table
 */
xinit()
{
	register struct text *xp;

	xtail = &xhead;
	for (xp = text; xp < textNTEXT; xp++)
		FREE_AT_TAIL(xp);
}

/*
 * Decrement loaded reference count of text object.  If not sticky and
 * count of zero, attach to LRU cache, at the head if traced or the
 * inode has a hard link count of zero, otherwise at the tail.
 */
xfree()
{
	register struct text *xp;

	if ((xp = u.u_procp->p_textp) == NULL)
		return;
#ifdef UCB_METER
	xstats.free++;
#endif
	X_LOCK(xp);
	/*
	 * Don't add the following test to the "if" below:
	 *
	 *	(xp->x_iptr->i_mode & ISVTX) == 0
	 *
	 * all text under 2.10 is sticky in an LRU cache.  Putting the
	 * above test in makes sticky text objects ``gluey'' and nearly
	 * impossible to flush from memory.
	 */
	if (--xp->x_count == 0) {
		if (xp->x_flag & XTRC || xp->x_iptr->i_nlink == 0) {
			xp->x_flag &= ~XLOCK;
			xuntext(xp);
			FREE_AT_HEAD(xp);
		} else {
#ifdef UCB_METER
			if (xp->x_flag & XWRIT) {
				xstats.free_cacheswap++;
				xp->x_flag |= XUNUSED;
			}
			xstats.free_cache++;
#endif
			--xp->x_ccount;
			FREE_AT_TAIL(xp);
		}
	} else {
		--xp->x_ccount;
#ifdef UCB_METER
		xstats.free_inuse++;
#endif
	}
	XUNLOCK(xp);
	u.u_procp->p_textp = NULL;
}

/*
 * Attach to a shared text segment.  If there is no shared text, just
 * return.  If there is, hook up to it.  If it is not available from
 * core or swap, it has to be read in from the inode (ip); the written
 * bit is set to force it to be written out as appropriate.  If it is
 * not available from core, a swap has to be done to get it back.
 */
xalloc(ip, ep)
	struct exec *ep;
	register struct inode *ip;
{
	register struct text *xp;
	register u_int	count;
	off_t	offset;
	size_t ts;

	if (ep->a_text == 0)
		return;
#ifdef UCB_METER
	xstats.alloc++;
#endif
	while ((xp = ip->i_text) != NULL) {
		if (xp->x_flag&XLOCK) {
			/*
			 * Wait for text to be unlocked,
			 * then start over (may have changed state).
			 */
			xwait(xp);
			continue;
		}
		X_LOCK(xp);
		if (xp->x_back) {
			ALLOC(xp);
#ifdef UCB_METER
			xstats.alloc_cachehit++;
			xp->x_flag &= ~XUNUSED;
#endif
		}
#ifdef UCB_METER
		else
			xstats.alloc_inuse++;
#endif
		xp->x_count++;
		u.u_procp->p_textp = xp;
		if (!xp->x_caddr && !xp->x_ccount)
			xexpand(xp);
		else
			++xp->x_ccount;
		XUNLOCK(xp);
		return;
	}
	xp = xhead;
	if (xp == NULL) {
		tablefull("text");
		psignal(u.u_procp, SIGKILL);
		return;
	}
	ALLOC(xp);
	if (xp->x_iptr) {
#ifdef UCB_METER
		xstats.alloc_cacheflush++;
		if (xp->x_flag & XUNUSED)
			xstats.alloc_unused++;
#endif
		xuntext(xp);
	}
	xp->x_flag = XLOAD|XLOCK;
	ts = btoc(ep->a_text);
	if (u.u_ovdata.uo_ovbase)
		xp->x_size = u.u_ovdata.uo_ov_offst[NOVL];
	else
		xp->x_size = ts;
	if ((xp->x_daddr = malloc(swapmap, (size_t)ctod(xp->x_size))) == NULL) {
		swkill(u.u_procp, "xalloc");
		return;
	}
	xp->x_count = 1;
	xp->x_ccount = 0;
	xp->x_iptr = ip;
	ip->i_flag |= ITEXT;
	ip->i_text = xp;
	ip->i_count++;
	u.u_procp->p_textp = xp;
	xexpand(xp);
	estabur(ts, (u_int)0, (u_int)0, 0, RW);
	offset = sizeof(struct exec);
	if (u.u_ovdata.uo_ovbase)
		offset += (NOVL + 1) * sizeof(u_int);
	u.u_procp->p_flag |= SLOCK;
	u.u_error = rdwri(UIO_READ, ip, (caddr_t)0, ep->a_text & ~1,
			offset, UIO_USERISPACE, IO_UNIT, (int *)0);

	if (u.u_ovdata.uo_ovbase) {	/* read in overlays if necessary */
		register int i;

		offset += (off_t)(ep->a_text & ~1);
		for (i = 1; i <= NOVL; i++) {
			u.u_ovdata.uo_curov = i;
			count = ctob(u.u_ovdata.uo_ov_offst[i] - u.u_ovdata.uo_ov_offst[i-1]);
			if (count) {
				choverlay(RW);
				u.u_error = rdwri(UIO_READ, ip,
				    (caddr_t)(ctob(stoc(u.u_ovdata.uo_ovbase))),
					count, offset, UIO_USERISPACE,
					IO_UNIT, (int *)0);
				offset += (off_t) count;
			}
		}
	}
	u.u_ovdata.uo_curov = 0;
	u.u_procp->p_flag &= ~SLOCK;
	xp->x_flag |= XWRIT;
	xp->x_flag &= ~XLOAD;
}

/*
 * Assure core for text segment.  If there isn't enough room to get process
 * in core, swap self out.  x_ccount must be 0.  Text must be locked to keep
 * someone else from freeing it in the meantime.   Don't change the locking,
 * it's correct.
 */
xexpand(xp)
	register struct text *xp;
{
	if ((xp->x_caddr = malloc(coremap, xp->x_size)) != NULL) {
		if ((xp->x_flag & XLOAD) == 0)
			swap(xp->x_daddr, xp->x_caddr, xp->x_size, B_READ);
		xp->x_ccount++;
		XUNLOCK(xp);
		return;
	}
	if (setjmp(&u.u_ssave)) {
		sureg();
		return;
	}
	swapout(u.u_procp, X_FREECORE, X_OLDSIZE, X_OLDSIZE);
	XUNLOCK(xp);
	u.u_procp->p_flag |= SSWAP;
	swtch();
	/* NOTREACHED */
}

/*
 * Lock and unlock a text segment from swapping
 */
xlock(xp)
	register struct text *xp;
{

	X_LOCK(xp);
}

/*
 * Wait for xp to be unlocked if it is currently locked.
 */
xwait(xp)
register struct text *xp;
{

	X_LOCK(xp);
	XUNLOCK(xp);
}

xunlock(xp)
register struct text *xp;
{

	XUNLOCK(xp);
}

/*
 * Decrement the in-core usage count of a shared text segment.
 * When it drops to zero, free the core space.  Write the swap
 * copy of the text if as yet unwritten.
 */
xccdec(xp)
	register struct text *xp;
{
	if (!xp->x_ccount)
		return;
	X_LOCK(xp);
	if (--xp->x_ccount == 0) {
		if (xp->x_flag & XWRIT) {
			swap(xp->x_daddr, xp->x_caddr, xp->x_size, B_WRITE);
			xp->x_flag &= ~XWRIT;
		}
		mfree(coremap, xp->x_size, xp->x_caddr);
		xp->x_caddr = NULL;
	}
	XUNLOCK(xp);
}

/*
 * Free the swap image of all unused saved-text text segments which are from
 * device dev (used by umount system call).  If dev is NODEV, do all devices
 * (used when rebooting or malloc of swapmap failed).
 */
xumount(dev)
	register dev_t dev;
{
	register struct text *xp;

	for (xp = text; xp < textNTEXT; xp++) 
		if (xp->x_iptr != NULL &&
		    (dev == xp->x_iptr->i_dev || dev == NODEV))
			xuntext(xp);
}

/*
 * Remove text image from the text table.
 * the use count must be zero.
 */
xuntext(xp)
	register struct text *xp;
{
	register struct inode *ip;

	X_LOCK(xp);
	if (xp->x_count == 0) {
		ip = xp->x_iptr;
		xp->x_iptr = NULL;
		mfree(swapmap, ctod(xp->x_size), xp->x_daddr);
		if (xp->x_caddr)
			mfree(coremap, xp->x_size, xp->x_caddr);
		ip->i_flag &= ~ITEXT;
		ip->i_text = NULL;
		irele(ip);
	}
	XUNLOCK(xp);
}

/*
 * Free up "size" core; if swap copy of text has not yet been written,
 * do so.
 */
xuncore(size)
	register size_t size;
{
	register struct text *xp;

	for (xp = xhead; xp; xp = xp->x_forw) {
		if (!xp->x_iptr)
			continue;
		X_LOCK(xp);
		if (!xp->x_ccount && xp->x_caddr) {
			if (xp->x_flag & XWRIT) {
				swap(xp->x_daddr, xp->x_caddr, xp->x_size, B_WRITE);
				xp->x_flag &= ~XWRIT;
			}
			mfree(coremap, xp->x_size, xp->x_caddr);
			xp->x_caddr = NULL;
			if (xp->x_size >= size) {
				XUNLOCK(xp);
				return;
			}
		}
		XUNLOCK(xp);
	}
}

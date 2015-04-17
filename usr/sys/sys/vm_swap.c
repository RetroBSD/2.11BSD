/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_swap.c	1.3 (2.11BSD GTE) 3/10/93
 */

#include "param.h"
#include "../machine/seg.h"

#include "user.h"
#include "proc.h"
#include "text.h"
#include "map.h"
#include "buf.h"
#include "systm.h"
#include "vm.h"

/*
 * Swap a process in.
 * Allocate data and possible text separately.  It would be better
 * to do largest first.  Text, data, stack and u. are allocated in
 * that order, as that is likely to be in order of size.
 */
swapin(p)
	register struct proc *p;
{
	register struct text *xp;
	register memaddr x = NULL;
	memaddr a[3];

	/* Malloc the text segment first, as it tends to be largest. */
	xp = p->p_textp;
	if (xp) {
		xlock(xp);
		if (!xp->x_caddr && !xp->x_ccount) {
			x = malloc(coremap, xp->x_size);
			if (!x) {
				xunlock(xp);
				return(0);
			}
		}
	}
	if (malloc3(coremap, p->p_dsize, p->p_ssize, USIZE, a) == NULL) {
		if (x)
			mfree(coremap, xp->x_size, x);
		if (xp)
			xunlock(xp);
		return(0);
	}
	if (xp) {
		if (x) {
			xp->x_caddr = x;
			if ((xp->x_flag & XLOAD) == 0)
				swap(xp->x_daddr, x, xp->x_size, B_READ);
		}
		xp->x_ccount++;
		xunlock(xp);
	}
	if (p->p_dsize) {
		swap(p->p_daddr, a[0], p->p_dsize, B_READ);
		mfree(swapmap, ctod(p->p_dsize), p->p_daddr);
	}
	if (p->p_ssize) {
		swap(p->p_saddr, a[1], p->p_ssize, B_READ);
		mfree(swapmap, ctod(p->p_ssize), p->p_saddr);
	}
	swap(p->p_addr, a[2], USIZE, B_READ);
	mfree(swapmap, ctod(USIZE), p->p_addr);
	p->p_daddr = a[0];
	p->p_saddr = a[1];
	p->p_addr = a[2];
	if (p->p_stat == SRUN)
		setrq(p);
	p->p_flag |= SLOAD;
	p->p_time = 0;
#ifdef UCB_METER
	cnt.v_swpin++;
#endif
	return(1);
}

/*
 * Swap out process p.
 * odata and ostack are the old data size and the stack size
 * of the process, and are supplied during core expansion swaps.
 * The freecore flag causes its core to be freed -- it may be
 * off when called to create an image for a child process
 * in newproc.
 *
 * panic: out of swap space
 */
swapout(p, freecore, odata, ostack)
	register struct proc *p;
	int freecore;
	register u_int odata, ostack;
{
	memaddr a[3];

	if (odata == (u_int)X_OLDSIZE)
		odata = p->p_dsize;
	if (ostack == (u_int)X_OLDSIZE)
		ostack = p->p_ssize;
	if (malloc3(swapmap, ctod(p->p_dsize), ctod(p->p_ssize),
	    ctod(USIZE), a) == NULL)
		panic("out of swap space");
	p->p_flag |= SLOCK;
	if (p->p_textp)
		xccdec(p->p_textp);
	if (odata) {
		swap(a[0], p->p_daddr, odata, B_WRITE);
		if (freecore == X_FREECORE)
			mfree(coremap, odata, p->p_daddr);
	}
	if (ostack) {
		swap(a[1], p->p_saddr, ostack, B_WRITE);
		if (freecore == X_FREECORE)
			mfree(coremap, ostack, p->p_saddr);
	}
	/*
	 * Increment u_ru.ru_nswap for process being tossed out of core.
	 * We can be called to swap out a process other than the current
	 * process, so we have to map in the victim's u structure briefly.
	 * Note, savekdsa6 *must* be a static, because we remove the stack
	 * in the next instruction.  The splclock is to prevent the clock
	 * from coming in and doing accounting for the wrong process, plus
	 * we don't want to come through here twice.  Why are we doing
	 * this, anyway?
	 */
	{
		static u_short savekdsa6;
		int s;

		s = splclock();
		savekdsa6 = *KDSA6;
		*KDSA6 = p->p_addr;
		u.u_ru.ru_nswap++;
		*KDSA6 = savekdsa6;
		splx(s);
	}
	swap(a[2], p->p_addr, USIZE, B_WRITE);
	if (freecore == X_FREECORE)
		mfree(coremap, USIZE, p->p_addr);
	p->p_daddr = a[0];
	p->p_saddr = a[1];
	p->p_addr = a[2];
	p->p_flag &= ~(SLOAD|SLOCK);
	p->p_time = 0;

#ifdef UCB_METER
	cnt.v_swpout++;
#endif

	if (runout) {
		runout = 0;
		wakeup((caddr_t)&runout);
	}
}

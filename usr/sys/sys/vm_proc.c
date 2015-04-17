/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_proc.c	1.2 (2.11BSD GTE) 12/24/92
 */

#include "param.h"
#include "user.h"
#include "proc.h"
#include "text.h"
#include "map.h"
#include "kernel.h"

/*
 * Change the size of the data+stack regions of the process.
 * If the size is shrinking, it's easy -- just release the extra core.
 * If it's growing, and there is core, just allocate it and copy the
 * image, taking care to reset registers to account for the fact that
 * the system's stack has moved.  If there is no core, arrange for the
 * process to be swapped out after adjusting the size requirement -- when
 * it comes in, enough core will be allocated.  After the expansion, the
 * caller will take care of copying the user's stack towards or away from
 * the data area.  The data and stack segments are separated from each
 * other.  The second argument to expand specifies which to change.  The
 * stack segment will not have to be copied again after expansion.
 */
expand(newsize,segment)
	int newsize, segment;
{
	register struct proc *p;
	register int i, n;
	int a1, a2;

	p = u.u_procp;
	if (segment == S_DATA) {
		n = p->p_dsize;
		p->p_dsize = newsize;
		a1 = p->p_daddr;
		if(n >= newsize) {
			n -= newsize;
			mfree(coremap, n, a1+newsize);
			return;
		}
	} else {
		n = p->p_ssize;
		p->p_ssize = newsize;
		a1 = p->p_saddr;
		if(n >= newsize) {
			n -= newsize;
			p->p_saddr += n;
			mfree(coremap, n, a1);
			/*
			 *  Since the base of stack is different,
			 *  segmentation registers must be repointed.
			 */
			sureg();
			return;
		}
	}
	if (setjmp(&u.u_ssave)) {
		/*
		 * If we had to swap, the stack needs moving up.
		 */
		if (segment == S_STACK) {
			a1 = p->p_saddr;
			i = newsize - n;
			a2 = a1 + i;
			/*
			 * i is the amount of growth.  Copy i clicks
			 * at a time, from the top; do the remainder
			 * (n % i) separately.
			 */
			while (n >= i) {
				n -= i;
				copy(a1+n, a2+n, i);
			}
			copy(a1, a2, n);
		}
		sureg();
		return;
	}
	if (u.u_fpsaved == 0) {
		savfp(&u.u_fps);
		u.u_fpsaved = 1;
	}
	a2 = malloc(coremap, newsize);
	if (a2 == NULL) {
		if (segment == S_DATA)
			swapout(p, X_FREECORE, n, X_OLDSIZE);
		else
			swapout(p, X_FREECORE, X_OLDSIZE, n);
		p->p_flag |= SSWAP;
		swtch();
		/* NOTREACHED */
	}
	if (segment == S_STACK) {
		p->p_saddr = a2;
		/*
		 * Make the copy put the stack at the top of the new area.
		 */
		a2 += newsize - n;
	} else
		p->p_daddr = a2;
	copy(a1, a2, n);
	mfree(coremap, n, a1);
	sureg();
}

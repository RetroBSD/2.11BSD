/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)machdep.c	2.4 (2.11BSD) 1999/9/13
 */

#include "param.h"
#include "../machine/psl.h"
#include "../machine/reg.h"

#include "signalvar.h"
#include "user.h"
#include "proc.h"
#include "buf.h"
#include "map.h"
#include "uba.h"
#include "syslog.h"

#ifdef CURRENTLY_EXPANDED_INLINE
/*
 * Clear registers on exec
 */
setregs(entry)
	u_int entry;
{
	u.u_ar0[PC] = entry & ~01;
	u.u_fps.u_fpsr = 0;
}
#endif

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow trampoline code stored at u.u_pcb.pcb_sigc (as
 * specified by the user process) to call the user's real signal catch
 * routine, followed by sys sigreturn to the sigreturn routine below (see
 * /usr/src/lib/libc/pdp/sys/sigvec.s).  After sigreturn resets the signal
 * mask, the stack, and the frame pointer, it returns to the user specified
 * pc, ps.
 */
sendsig(p, sig, mask)
	int (*p)(), sig;
	long mask;
{
	struct sigframe {
		int	sf_signum;
		int	sf_code;
		struct	sigcontext *sf_scp;
		struct	sigcontext sf_sc;
	};
	struct sigframe sf;
	register struct sigframe *sfp = &sf;
	register struct sigcontext *scp = &sf.sf_sc;
	register int *regs;
	int oonstack;
	caddr_t n;

#ifdef	DIAGNOSTIC
	printf("sendsig %d to %d mask=%O action=%o\n", sig, u.u_procp->p_pid,
		mask, p);
#endif
	regs = u.u_ar0;
	oonstack = u.u_sigstk.ss_flags & SA_ONSTACK;
	/*
	 * Allocate and validate space for the signal frame.
	 */
	if	((u.u_psflags & SAS_ALTSTACK) &&
		 !(u.u_sigstk.ss_flags & SA_ONSTACK) &&
		 (u.u_sigonstack & sigmask(sig)))
		{
		n = u.u_sigstk.ss_base + u.u_sigstk.ss_size - sizeof (sf);
		u.u_sigstk.ss_flags |= SA_ONSTACK;
		}
	else
		n = (caddr_t)regs[R6] - sizeof (sf);
	if	(!(u.u_sigstk.ss_flags & SA_ONSTACK) && 
		 n < (caddr_t)-ctob(u.u_ssize) && 
		 !grow(n))
		{
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction violation to halt it in its tracks.
		 */
		fatalsig(SIGILL);
		return;
		}
	/* 
	 * Build the argument list for the signal handler.
	 */
	sfp->sf_signum = sig;
	if (sig == SIGILL || sig == SIGFPE) {
		sfp->sf_code = u.u_code;
		u.u_code = 0;
	} else
		sfp->sf_code = 0;
	sfp->sf_scp = (struct sigcontext *)
			(n + (u_int)&((struct sigframe *)0)->sf_sc);
	/*
	 * Build the signal context to be used by sigreturn.
	 */
	scp->sc_onstack = oonstack;
	scp->sc_mask = mask;
	scp->sc_sp = regs[R6];
	scp->sc_fp = regs[R5];
	scp->sc_r1 = regs[R1];
	scp->sc_r0 = regs[R0];
	scp->sc_pc = regs[R7];
	scp->sc_ps = regs[RPS];
	scp->sc_ovno = u.u_ovdata.uo_ovbase ? u.u_ovdata.uo_curov : 0;
	copyout((caddr_t)sfp, n, sizeof(*sfp));
	regs[R0] = (int)p;
	regs[R6] = (int)n;
	regs[R7] = (int)u.u_pcb.pcb_sigc;
	regs[RPS] &= ~PSL_T;
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and ps as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * ps to gain improper priviledges or to cause
 * a machine fault.
 */
sigreturn()
{
	struct a {
		struct sigcontext *scp;
	};
	struct sigcontext sc;
	register struct sigcontext *scp = &sc;
	register int *regs = u.u_ar0;

	u.u_error = copyin((caddr_t)((struct a *)u.u_ap)->scp, (caddr_t *)scp, sizeof(*scp));
	if (u.u_error)
		return;
	if ((scp->sc_ps & PSL_USERCLR) != 0 || !USERMODE(scp->sc_ps)) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = EJUSTRETURN;
	if	(scp->sc_onstack & SA_ONSTACK)
		u.u_sigstk.ss_flags |= SA_ONSTACK;
	else
		u.u_sigstk.ss_flags &= ~SA_ONSTACK;
	u.u_procp->p_sigmask = scp->sc_mask & ~sigcantmask;
	regs[R6] = scp->sc_sp;
	regs[R5] = scp->sc_fp;
	regs[R1] = scp->sc_r1;
	regs[R0] = scp->sc_r0;
	regs[R7] = scp->sc_pc;
	regs[RPS] = scp->sc_ps;
}

#define	UMAPSIZ	10

struct	mapent _ubmap[UMAPSIZ];
struct	map ub_map[1] = {
	&_ubmap[0], &_ubmap[UMAPSIZ], "ub_map"
};
int	ub_wantmr;

#ifdef UCB_METER
struct ubmeter	ub_meter;
#endif

/*
 * Routine to allocate the UNIBUS map and initialize for a UNIBUS device.
 * For buffers already mapped by the UNIBUS map, perform the physical to
 * UNIBUS-virtual address translation.
 */
mapalloc(bp)
	register struct buf *bp;
{
	register struct ubmap *ubp;
	register int ub_nregs;
	ubadr_t ubaddr;
	long paddr;
	int s, ub_first;

	if (!ubmap)
		return;
#ifdef UCB_METER
	++ub_meter.ub_calls;
#endif
	paddr = ((long)((u_int)bp->b_xmem)) << 16
		| ((long)((u_int)bp->b_un.b_addr));
	if (!(bp->b_flags & B_PHYS)) {
		/*
		 * Transfer in the buffer cache.
		 * Change the buffer's physical address
		 * into a UNIBUS address for the driver.
		 */
#ifdef UCB_METER
		++ub_meter.ub_remaps;
#endif
		ubaddr = paddr - (((ubadr_t)bpaddr) << 6) + BUF_UBADDR;
		bp->b_un.b_addr = (caddr_t)loint(ubaddr);
		bp->b_xmem = hiint(ubaddr);
		bp->b_flags |= B_UBAREMAP;
	}
	else {
		/*
		 * Physical I/O.
		 * Allocate a section of the UNIBUS map.
		 */
		ub_nregs = (int)btoub(bp->b_bcount);
#ifdef UCB_METER
		ub_meter.ub_pages += ub_nregs;
#endif
		s = splclock();
		while (!(ub_first = malloc(ub_map,ub_nregs))) {
#ifdef UCB_METER
			++ub_meter.ub_fails;
#endif
			ub_wantmr = 1;
			sleep(ub_map, PSWP + 1);
		}
		splx(s);

		ubp = &UBMAP[ub_first];
		bp->b_xmem = ub_first >> 3;
		bp->b_un.b_addr = (caddr_t)((ub_first & 07) << 13);
		bp->b_flags |= B_MAP;

		while (ub_nregs--) {
			ubp->ub_lo = loint(paddr);
			ubp->ub_hi = hiint(paddr);
			ubp++;
			paddr += (ubadr_t)UBPAGE;
		}
	}
}

mapfree(bp)
	register struct buf *bp;
{
	register int s;
	ubadr_t ubaddr;
	long paddr;

	if (bp->b_flags & B_MAP) {
		/*
		 * Free the UNIBUS map registers
		 * allocated to this buffer.
		 */
		s = splclock();
		mfree(ub_map, (size_t)btoub(bp->b_bcount), (bp->b_xmem << 3) |
			(((u_int)bp->b_un.b_addr >> 13) & 07));
		splx(s);
		bp->b_flags &= ~B_MAP;
		if (ub_wantmr)
			wakeup((caddr_t) ub_map);
		ub_wantmr = 0;
	}
	else if (bp->b_flags & B_UBAREMAP) {
		/*
		 * Translate the UNIBUS virtual address of this buffer
		 * back to a physical memory address.
		 */
		ubaddr = ((long)((u_int) bp->b_xmem)) << 16
			| ((long)((u_int)bp->b_un.b_addr));
		paddr = ubaddr - (long)BUF_UBADDR + (((long)bpaddr) << 6);
		bp->b_un.b_addr = (caddr_t)loint(paddr);
		bp->b_xmem = hiint(paddr);
		bp->b_flags &= ~B_UBAREMAP;
	}
}

#define	SOFUB_DEBUG

#ifdef	SOFUB_MAP
/*
 * Implement soft unibus map for 18 bit controllers present in a
 * 22-bit q-bus systems.  Initially only used by the tape driver 
 * (for tar and dump) but when this was integrated into 2.11BSD changes
 * were made to support the RX02 driver as well.
 *
 * Adapted to 2.11BSD and rather heavily revised by:
 *
 *		steven schultz (sms)
 *		GTE Government Systems
 *
 *	from the original by:
 *
 *		matt jacob
 *		University of California at Davis
 *		22-Nov-84
*/

#define	QMEG	((long)(256L*1024L))
#define	exad(x,y)	((long)((long)x<<16|(unsigned)y))
#define	B_UBUF		exad(bp->b_xmem,(u_int)bp->b_un.b_addr)
#define	E_UBUF		(B_UBUF + (long) bp->b_bcount)

	size_t	sofub_addr, sofub_off;
	memaddr	sofub_base;
	unsigned sofub_size;

static	int	bpxm, bpadd;
static	int	sofub_flags;
#define	BUSY	1
#define	WANTED	2

/*
 * sofub_alloc - allocate usage of soft unibus map
 *
 *	called from strategy routine of device drivers.
 *
 *	returns 0 if not okay, else returns 1.
 *	expects a buffer pointer as an argument
 *	expects all addresses in bp already validated (by physio)
 *	sets appropriate bits and calls iodone if error
*/

int
sofub_alloc(bp)
register struct buf *bp;
	{
	register int s;
	register int count;
	size_t uaddr;
	memaddr sbase;

	if	(E_UBUF < QMEG)
		return(1);	/* okay, < 256kb	*/
	else if	(bp->b_bcount > sofub_size)
		{
		log(LOG_ERR, "I/O >10kb on %d,%d\n",
			major(bp->b_dev),minor(bp->b_dev));
		bp->b_flags |= B_ERROR;
		bp->b_error = EFAULT;
		iodone(bp);
		return(0);
		}

	s = splbio();

	while	(sofub_flags & BUSY)
		{
#ifdef SOFUB_DEBUG
		printf("sofub_alloc sleep\n");
#endif
		sofub_flags |= WANTED;
		sleep((caddr_t) &sofub_flags,PSWP+2);
		}

	sofub_flags |= BUSY;
	splx(s);

	/*
	 * now calculate virtual address of user buffer...
	 */

	sofub_off = (size_t)((u_int)bp->b_un.b_addr & 077);
	sofub_addr = (size_t)(((u_int)bp->b_un.b_addr>>6)&01777) | (((int) bp->b_xmem) << 10);

	bpadd = (u_int)bp->b_un.b_addr;
	bpxm = bp->b_xmem;

	/*
	 * if this is a write, we have to fetch data from user buffer first
	 */

	if	((bp->b_flags & B_READ) == 0)
		{
		count = bp->b_bcount;
		uaddr = sofub_addr;
		sbase = sofub_base;

		/* first, copy all 8kb-1click segments.. */

		s = spl5();

		while	(count > (8192-64))
			{
			copyv(uaddr,sofub_off,sbase,0,(8192-64));
			count -= (8192-64);
			uaddr += 0177;	/* add 8192-64 bytes */
			sbase += 0177;
			}
		/* copy last residual segment */

		copyv(uaddr,sofub_off,sbase,0,count);
		splx(s);
		}
	/* put in new 18 bit address */

	bp->b_un.b_addr = (caddr_t)ctob((long)sofub_base);

	/*
	 * don't turn sofub_base to clicks here
	 * because half the work is done by
	 * having it in click form here, i.e.,
	 * bp->b_xmem would equal
	 * ctob(x)>>16 (hi six bits of 18)
	 */

	bp->b_xmem = (sofub_base >> 10)&3;
	return(1);
	}

/*
 * sofub_relse	- release sofub_map
 *
 *	passed a buffer pointer and a transfer byte count...
 *	(for use if was a read)
 *
 *	note, we are called from the driver's interrupt service routine.
*/

sofub_relse(bp,count)
register struct buf *bp;
register unsigned count;
	{
	register int s;
	size_t uaddr;
	memaddr sbase;
	long	ebuf = E_UBUF;
	caddr_t	newaddr;
	u_int	newxmem;

/*
 * If the transfer was entirely below or above the 'bounce buffer' then 
 * this I/O request was done without the soft map being allocated.
*/
	if	(ebuf < ctob((long)sofub_base) || 
		 ebuf > ctob((long)sofub_base) + sofub_size)
		return;
#ifdef	SOFUB_DEBUG
	if	(ebuf > QMEG)		/* Can't happen, transfer over 256kb */
		panic("sofub_relse");	
#endif

/*
 * Next make sure that the I/O request is the one which currently has the
 * map allocated to it.
*/
	newaddr = (caddr_t)((sofub_addr << 6) | sofub_off);
	newxmem = ((u_int)sofub_addr >> 10) & 077;

	if	(newaddr != bpadd ||  newxmem != bpxm)
		{
#ifdef	SOFUB_DEBUG
		printf("sofub_relse: new %o:%o, old %o:%o\n",
			newxmem, newaddr, bpxm, bpadd);
#endif
		bp->b_flags |= B_ERROR;
		bp->b_error = EFAULT;
		return;
		}

	if	(bp->b_flags & B_READ)
		{
		uaddr = sofub_addr;
		sbase = sofub_base;

		/*
		 * first, copy all 8kb-1click segments..
		 */
		s = spl5();
		while	(count > (8192-64))
			{
			copyv(sbase,0,uaddr,sofub_off,(8192-64));
			count -= (8192-64);
			uaddr += 0177;	/* add 8192-64 bytes */
			sbase += 0177;
			}
		/*
		 * copy last residual segment
		 */
		copyv(sbase,0,uaddr,sofub_off,count);
		splx(s);
		}

	bp->b_un.b_addr = newaddr;
	bp->b_xmem = newxmem;

	sofub_flags &= ~BUSY;

	if	(sofub_flags & WANTED)
		{
#ifdef	SOFUB_DEBUG
		printf("sofub_relse: wakeup\n");
#endif
		sofub_flags &= ~WANTED;
		wakeup((caddr_t) &sofub_flags);
		}
	}
#endif	/* SOFUB_MAP */

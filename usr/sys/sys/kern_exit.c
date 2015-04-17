/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_exit.c	2.6 (2.11BSD) 2000/2/20
 */

#include "param.h"
#include "../machine/psl.h"
#include "../machine/reg.h"

#include "systm.h"
#include "map.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "vm.h"
#include "file.h"
#include "wait.h"
#include "kernel.h"
#ifdef QUOTA
#include "quota.h"
#endif
#include "ingres.h"

extern	int	Acctopen;	/* kern_acct.c */

/*
 * exit system call: pass back caller's arg
 */
rexit()
{
	register struct a {
		int	rval;
	} *uap = (struct a *)u.u_ap;

	exit(W_EXITCODE(uap->rval, 0));
	/* NOTREACHED */
}

/*
 * Exit: deallocate address space and other resources,
 * change proc state to zombie, and unlink proc from allproc
 * list.  Save exit status and rusage for wait4().
 * Check for child processes and orphan them.
 */
exit(rv)
{
	register int i;
	register struct proc *p;
	struct	proc **pp;

	p = u.u_procp;
	p->p_flag &= ~(P_TRACED|SULOCK);
	p->p_sigignore = ~0;
	p->p_sig = 0;
	/*
	 * 2.11 doesn't need to do this and it gets overwritten anyway.
	 * p->p_realtimer.it_value = 0;
	 */
	for (i = 0; i <= u.u_lastfile; i++) {
		register struct file *f;

		f = u.u_ofile[i];
		u.u_ofile[i] = NULL;
		u.u_pofile[i] = 0;
		(void) closef(f);
	}
	ilock(u.u_cdir);
	iput(u.u_cdir);
	if (u.u_rdir) {
		ilock(u.u_rdir);
		iput(u.u_rdir);
	}
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	if	(Acctopen)
		(void) acct();
#ifdef QUOTA
	QUOTAMAP();
	qclean();
	QUOTAUNMAP();
#endif
	/*
	 * Freeing the user structure and kernel stack
	 * for the current process: have to run a bit longer
	 * using the slots which are about to be freed...
	 */
	if (p->p_flag & SVFORK)
		endvfork();
	else {
		xfree();
		mfree(coremap, p->p_dsize, p->p_daddr);
		mfree(coremap, p->p_ssize, p->p_saddr);
	}
	mfree(coremap, USIZE, p->p_addr);

	if (p->p_pid == 1)
		panic("init died");
	if (*p->p_prev = p->p_nxt)		/* off allproc queue */
		p->p_nxt->p_prev = p->p_prev;
	if (p->p_nxt = zombproc)		/* onto zombproc */
		p->p_nxt->p_prev = &p->p_nxt;
	p->p_prev = &zombproc;
	zombproc = p;
	p->p_stat = SZOMB;

#if	NINGRES > 0
	ingres_rma(p->p_pid);		/* Remove any ingres locks */
#endif

	noproc = 1;
	for (pp = &pidhash[PIDHASH(p->p_pid)]; *pp; pp = &(*pp)->p_hash)
		if (*pp == p) {
			*pp = p->p_hash;
			goto done;
		}
	panic("exit");
done:
	/*
	 * Overwrite p_alive substructure of proc - better not be anything
	 * important left!
	 */
	p->p_xstat = rv;
	p->p_ru = u.u_ru;
	ruadd(&p->p_ru, &u.u_cru);
	{
		register struct proc *q;
		int doingzomb = 0;

		q = allproc;
again:
		for(; q; q = q->p_nxt)
			if (q->p_pptr == p) {
				q->p_pptr = &proc[1];
				q->p_ppid = 1;
				wakeup((caddr_t)&proc[1]);
				if (q->p_flag& P_TRACED) {
					q->p_flag &= ~P_TRACED;
					psignal(q, SIGKILL);
				} else if (q->p_stat == SSTOP) {
					psignal(q, SIGHUP);
					psignal(q, SIGCONT);
				}
			}
		if (!doingzomb) {
			doingzomb = 1;
			q = zombproc;
			goto again;
		}
	}
	psignal(p->p_pptr, SIGCHLD);
	wakeup((caddr_t)p->p_pptr);
	swtch();
	/* NOTREACHED */
}

	struct	args
		{
		int pid;
		int *status;
		int options;
		struct rusage *rusage;
		int compat;
		};

wait4()
{
	int retval[2];
	register struct	args *uap = (struct args *)u.u_ap;

	uap->compat = 0;
	u.u_error = wait1(u.u_procp, uap, retval);
	if (!u.u_error)
		u.u_r.r_val1 = retval[0];
}

/*
 * Wait: check child processes to see if any have exited,
 * stopped under trace or (optionally) stopped by a signal.
 * Pass back status and make available for reuse the exited
 * child's proc structure.
 */
wait1(q, uap, retval)
	struct proc *q;
	register struct args *uap;
	int retval[];
{
	int nfound, status;
	struct rusage ru;			/* used for local conversion */
	register struct proc *p;
	register int error;

	if (uap->pid == WAIT_MYPGRP)		/* == 0 */
		uap->pid = -q->p_pgrp;
loop:
	nfound = 0;
	/*
	 * 4.X has child links in the proc structure, so they consolidate
	 * these two tests into one loop.  We only have the zombie chain
	 * and the allproc chain, so we check for ZOMBIES first, then for
	 * children that have changed state.  We check for ZOMBIES first
	 * because they are more common, and, as the list is typically small,
	 * a faster check.
	 */
	for (p = zombproc; p;p = p->p_nxt) {
		if (p->p_pptr != q)	/* are we the parent of this process? */
			continue;
		if (uap->pid != WAIT_ANY &&
		    p->p_pid != uap->pid && p->p_pgrp != -uap->pid)
			continue;
		retval[0] = p->p_pid;
		retval[1] = p->p_xstat;
		if (uap->status && (error = copyout(&p->p_xstat, uap->status,
						sizeof (uap->status))))
			return(error);
		if (uap->rusage) {
			rucvt(&ru, &p->p_ru);
			if (error = copyout(&ru, uap->rusage, sizeof (ru)))
				return(error);
		}
		ruadd(&u.u_cru, &p->p_ru);
		p->p_xstat = 0;
		p->p_stat = NULL;
		p->p_pid = 0;
		p->p_ppid = 0;
		if (*p->p_prev = p->p_nxt)	/* off zombproc */
			p->p_nxt->p_prev = p->p_prev;
		p->p_nxt = freeproc;		/* onto freeproc */
		freeproc = p;
		p->p_pptr = 0;
		p->p_sig = 0;
		p->p_sigcatch = 0;
		p->p_sigignore = 0;
		p->p_sigmask = 0;
		p->p_pgrp = 0;
		p->p_flag = 0;
		p->p_wchan = 0;
		return (0);
	}
	for (p = allproc; p;p = p->p_nxt) {
		if (p->p_pptr != q)
			continue;
		if (uap->pid != WAIT_ANY &&
		    p->p_pid != uap->pid && p->p_pgrp != -uap->pid)
			continue;
		++nfound;
		if (p->p_stat == SSTOP && (p->p_flag& P_WAITED)==0 &&
		    (p->p_flag&P_TRACED || uap->options&WUNTRACED)) {
			p->p_flag |= P_WAITED;
			retval[0] = p->p_pid;
			error = 0;
			if (uap->compat)
				retval[1] = W_STOPCODE(p->p_ptracesig);
			else if (uap->status) {
				status = W_STOPCODE(p->p_ptracesig);
				error = copyout(&status, uap->status,
						sizeof (status));
			}
			return (error);
		}
	}
	if (nfound == 0)
		return (ECHILD);
	if (uap->options&WNOHANG) {
		retval[0] = 0;
		return (0);
	}
	error = tsleep(q, PWAIT|PCATCH, 0);
	if	(error == 0)
		goto loop;
	return(error);
}

/*
 * Notify parent that vfork child is finished with parent's data.  Called
 * during exit/exec(getxfile); must be called before xfree().  The child
 * must be locked in core so it will be in core when the parent runs.
 */
endvfork()
{
	register struct proc *rip, *rpp;

	rpp = u.u_procp;
	rip = rpp->p_pptr;
	rpp->p_flag &= ~SVFORK;
	rpp->p_flag |= SLOCK;
	wakeup((caddr_t)rpp);
	while(!(rpp->p_flag&SVFDONE))
		sleep((caddr_t)rip,PZERO-1);
	/*
	 * The parent has taken back our data+stack, set our sizes to 0.
	 */
	u.u_dsize = rpp->p_dsize = 0;
	u.u_ssize = rpp->p_ssize = 0;
	rpp->p_flag &= ~(SVFDONE | SLOCK);
}

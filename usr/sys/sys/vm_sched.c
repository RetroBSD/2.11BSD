/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_sched.c	1.2 (2.11BSD) 1999/9/10
 */

#include "param.h"
#include "user.h"
#include "proc.h"
#include "text.h"
#include "vm.h"
#include "kernel.h"
#include "systm.h"

#define	MINFINITY	-32767		/* minus infinity */

int	maxslp = MAXSLP;

/*
 * The main loop of the scheduling (swapping) process.
 * The basic idea is:
 *	see if anyone wants to be swapped in
 *	swap out processes until there is room
 *	swap him in
 *	repeat
 * The runout flag is set whenever someone is swapped out.  Sched sleeps on
 * it awaiting work.  Sched sleeps on runin whenever it cannot find enough
 * core (by swapping out or otherwise) to fit the selected swapped process.
 * It is awakened when the core situation changes and in any case once per
 * second.
 */
sched()
{
	register struct proc *rp;
	struct proc *p;
	int inage, maxsize, outpri;

    /* Find user to swap in; of users ready, select one out longest. */
    for (;;) {
	(void)_splhigh();
	{
	register int l_outpri, rppri;

	l_outpri = -20000;
	for (rp = allproc; rp; rp = rp->p_nxt) {
		if (rp->p_stat != SRUN || rp->p_flag & SLOAD)
			continue;
		rppri = rp->p_time - rp->p_nice * 8;

		/*
		 * Always bring in parents ending a vfork,
		 * to avoid deadlock
		 */
		if (rppri > l_outpri || rp->p_flag & SVFPRNT) {
			p = rp;
			l_outpri = rppri;
			if (rp->p_flag & SVFPRNT)
				break;
		}
	}

	/* If there is no one there, wait. */
	if (l_outpri == -20000) {
		++runout;
		sleep((caddr_t)&runout, PSWP);
		continue;
	}
	outpri = l_outpri;
	}

	(void)_spl0();

	/* See if there is core for that process, if so, swap it in. */
	if (swapin(p))
		continue;

	/*
	 * None found.  Look around for core: 1) kick out dead wood
	 * (processes asleep longer than maxslp+10); or 2) select out
	 * of the processes sleeping interruptibly the process with
	 * maximum f(size, slptime); or 3) if none, select the oldest.
	 * If we can find someone to swap out we try to swap someone
	 * else (hopefully) in, possibly causing someone else to get
	 * swapped out.
	 *
	 * f(size, slptime) = size + (slptime - maxslp) * 16
	 *
	 * This weighting is designed to penalize linearly for size
	 * and excessive slptime, but to reward processes which are
	 * executing or have executed recently.
	 */
	{
	register int l_maxsize;

	(void)_splhigh();
	l_maxsize = MINFINITY;
	inage = -1;
	for (rp = allproc; rp != NULL; rp = rp->p_nxt) {
		if (rp->p_stat == SZOMB ||
		    (rp->p_flag & (SSYS|SLOCK|SULOCK|SLOAD)) != SLOAD)
			continue;
		if (rp->p_textp && rp->p_textp->x_flag & XLOCK)
			continue;
		if (rp->p_stat == SSLEEP &&
		    (rp->p_flag & P_SINTR) || rp->p_stat == SSTOP) {
			register int size;

			if (rp->p_slptime > maxslp+10) {
				p = rp;
				l_maxsize = 1;
				break;
			}
			size = rp->p_dsize + rp->p_ssize
			    + (rp->p_slptime - maxslp << 4);
			if (l_maxsize < size) {
				p = rp;
				l_maxsize = size;
			}
		}
		else if (l_maxsize == MINFINITY &&
		    (rp->p_stat == SRUN || rp->p_stat == SSLEEP)) {
			register int rppri;

			rppri = rp->p_time + rp->p_nice;
			if (rppri > inage) {
				p = rp;
				inage = rppri;
			}
		}
	}
	maxsize = l_maxsize;
	}

	(void)_spl0();
	noop();
	(void)_splhigh();
	/*
	 * Swap found user out if sleeping interruptibly, or if he has spent at
	 * least 1 second in core and the swapped-out process has spent at
	 * least 2 seconds out.  Otherwise wait a bit and try again.
	 */
	if (maxsize != MINFINITY || outpri >= 2 && inage >= 1) {
		p->p_flag &= ~SLOAD;
		if (p->p_stat == SRUN)
			remrq(p);
		(void)_spl0();
		swapout(p, X_FREECORE, X_OLDSIZE, X_OLDSIZE);
	}
	else {
		++runin;
		sleep((caddr_t)&runin, PSWP);
	}
    }
}

/*
 * Count up various things once a second
 */
vmmeter()
{
#ifdef UCB_METER
	register u_short *cp, *rp;
	register long *sp;

	ave(avefree, freemem, 5);
	ave(avefree30, freemem, 30);
	cp = &cnt.v_first; rp = &rate.v_first; sp = &sum.v_first;
	while (cp <= &cnt.v_last) {
		ave(*rp, *cp, 5);
		*sp += *cp;
		*cp = 0;
		rp++, cp++, sp++;
	}
#endif

	if (time.tv_sec % 5 == 0) {
		vmtotal();
#ifdef UCB_METER
		rate.v_swpin = cnt.v_swpin;
		sum.v_swpin += cnt.v_swpin;
		cnt.v_swpin = 0;
		rate.v_swpout = cnt.v_swpout;
		sum.v_swpout += cnt.v_swpout;
		cnt.v_swpout = 0;
#endif
	}
}

vmtotal()
{
	register struct proc *p;
	register nrun = 0;
#ifdef UCB_METER
	char textcounted[100];

	total.t_vmtxt = 0;
	total.t_avmtxt = 0;
	total.t_rmtxt = 0;
	total.t_armtxt = 0;
{
	register struct text *xp;

	for (xp = text; xp < textNTEXT; xp++)
		if (xp->x_iptr) {
			total.t_vmtxt += xp->x_size;
			if (xp->x_ccount)
				total.t_rmtxt += xp->x_size;
		}
}
	total.t_vm = 0;
	total.t_avm = 0;
	total.t_rm = 0;
	total.t_arm = 0;
	total.t_rq = 0;
	total.t_dw = 0;
	total.t_sl = 0;
	total.t_sw = 0;
	bzero(textcounted, ntext * sizeof(char));
#endif
	for (p = allproc; p != NULL; p = p->p_nxt) {
		if (p->p_flag & SSYS)
			continue;
		if (p->p_stat) {
#ifdef UCB_METER
			if (p->p_stat != SZOMB) {
				total.t_vm += p->p_dsize + p->p_ssize + USIZE;
				if (p->p_flag & SLOAD)
					total.t_rm += p->p_dsize + p->p_ssize
					    + USIZE;
			}
#endif
			switch (p->p_stat) {

			case SSLEEP:
			case SSTOP:
				if (!(p->p_flag & P_SINTR) && p->p_stat == SSLEEP)
					nrun++;
#ifdef UCB_METER
				if (p->p_flag & SLOAD) {
					if	(!(p->p_flag & P_SINTR))
						total.t_dw++;
					else if (p->p_slptime < maxslp)
						total.t_sl++;
				} else if (p->p_slptime < maxslp)
					total.t_sw++;
				if (p->p_slptime < maxslp)
					goto active;
#endif
				break;

			case SRUN:
			case SIDL:
				nrun++;
#ifdef UCB_METER
				if (p->p_flag & SLOAD)
					total.t_rq++;
				else
					total.t_sw++;
active:
				total.t_avm += p->p_dsize + p->p_ssize + USIZE;
				if (p->p_flag & SLOAD)
					total.t_arm += p->p_dsize + p->p_ssize
					    + USIZE;
				if (p->p_textp) switch (p->p_stat) {

				case SSTOP:
				case SSLEEP:
					if (p->p_slptime >= maxslp)
						break;
					/* fall into... */

				case SRUN:
				case SIDL:
				{	register int nt;

					total.t_avmtxt += p->p_textp->x_size;
					nt = p->p_textp - text;
					if (!textcounted[nt]) {
						textcounted[nt] = 1;
						if (p->p_textp->x_ccount)
							total.t_armtxt +=
							    p->p_textp->x_size;
					}
				}
				}
#endif
				break;
			}
		}
	}
#ifdef UCB_METER
	total.t_vm += total.t_vmtxt;
	total.t_avm += total.t_avmtxt;
	total.t_rm += total.t_rmtxt;
	total.t_arm += total.t_armtxt;
	total.t_free = avefree;
#endif
	loadav(avenrun, nrun);
}

/*
 * Compute Tenex style load average.  This code is adapted from similar code
 * by Bill Joy on the Vax system.  The major change is that we avoid floating
 * point since not all pdp-11's have it.  This makes the code quite hard to
 * read - it was derived with some algebra.
 *
 * "floating point" numbers here are stored in a 16 bit short, with 8 bits on
 * each side of the decimal point.  Some partial products will have 16 bits to
 * the right.
 *
 * The Vax algorithm is:
 *
 * /*
 *  * Constants for averages over 1, 5, and 15 minutes
 *  * when sampling at 5 second intervals.
 *  * /
 * double	cexp[3] = {
 * 	0.9200444146293232,	/* exp(-1/12) * /
 * 	0.9834714538216174,	/* exp(-1/60) * /
 * 	0.9944598480048967,	/* exp(-1/180) * /
 * };
 * 
 * /*
 *  * Compute a tenex style load average of a quantity on
 *  * 1, 5 and 15 minute intervals.
 *  * /
 * loadav(avg, n)
 * 	register double *avg;
 * 	int n;
 * {
 * 	register int i;
 * 
 * 	for (i = 0; i < 3; i++)
 * 		avg[i] = cexp[i] * avg[i] + n * (1.0 - cexp[i]);
 * }
 */

long cexp[3] = {
	0353,	/* 256 * exp(-1/12)  */
	0373,	/* 256 * exp(-1/60)  */
	0376,	/* 256 * exp(-1/180) */
};

loadav(avg, n)
	register short	*avg;
	register int	n;
{
	register int	i;

	for (i = 0; i < 3; i++)
		avg[i] = (cexp[i] * (avg[i]-(n<<8)) + (((long)n)<<16)) >> 8;
}

/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)sys_process.c	1.2 (2.11BSD) 1999/9/5
 */

#include "param.h"
#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/seg.h"

#include "systm.h"
#include "user.h"
#include "proc.h"
#include "inode.h"
#include "text.h"
#include "vm.h"
#include "ptrace.h"

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct
{
	int	ip_lock;
	int	ip_req;
	int	*ip_addr;
	int	ip_data;
} ipc;

/*
 * sys-trace system call.
 */
ptrace()
{
	register struct proc *p;
	register struct a {
		int	req;
		int	pid;
		int	*addr;
		int	data;
	} *uap;

	uap = (struct a *)u.u_ap;
	if (uap->req <= 0) {
		u.u_procp->p_flag |= P_TRACED;
		return;
	}
	p = pfind(uap->pid);
	if (p == 0 || p->p_stat != SSTOP || p->p_ppid != u.u_procp->p_pid ||
	    !(p->p_flag & P_TRACED)) {
		u.u_error = ESRCH;
		return;
	}
	while (ipc.ip_lock)
		sleep((caddr_t)&ipc, PZERO);
	ipc.ip_lock = p->p_pid;
	ipc.ip_data = uap->data;
	ipc.ip_addr = uap->addr;
	ipc.ip_req = uap->req;
	p->p_flag &= ~P_WAITED;
	setrun(p);
	while (ipc.ip_req > 0)
		sleep((caddr_t)&ipc, PZERO);
	u.u_r.r_val1 = (short)ipc.ip_data;
	if (ipc.ip_req < 0)
		u.u_error = EIO;
	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
}

/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
procxmt()
{
	register int i;
	register *p;
	register struct text *xp;

	if (ipc.ip_lock != u.u_procp->p_pid)
		return(0);
	u.u_procp->p_slptime = 0;
	i = ipc.ip_req;
	ipc.ip_req = 0;
	wakeup((caddr_t)&ipc);
	switch (i) {

	/* read user I */
	case PT_READ_I:
#ifndef NONSEPARATE
		if (fuibyte((caddr_t)ipc.ip_addr) == -1)
			goto error;
		ipc.ip_data = fuiword((caddr_t)ipc.ip_addr);
		break;
#endif /* !NONSEPARATE */

	/* read user D */
	case PT_READ_D:
		if (fubyte((caddr_t)ipc.ip_addr) == -1)
			goto error;
		ipc.ip_data = fuword((caddr_t)ipc.ip_addr);
		break;

	/* read u */
	case PT_READ_U:
		i = (int)ipc.ip_addr;
		if (i<0 || i >= ctob(USIZE))
			goto error;
		ipc.ip_data = ((physadr)&u)->r[i/sizeof(int)];
		break;

	/* write user I */
	/* Must set up to allow writing */
	case PT_WRITE_I:
		/*
		 * If text, must assure exclusive use
		 */
		if (xp = u.u_procp->p_textp) {
			if (xp->x_count!=1 || xp->x_iptr->i_mode&ISVTX)
				goto error;
			xp->x_flag |= XTRC;
		}
		estabur(u.u_tsize, u.u_dsize, u.u_ssize, u.u_sep, RW);
		i = suiword((caddr_t)ipc.ip_addr, 0);
		suiword((caddr_t)ipc.ip_addr, ipc.ip_data);
		estabur(u.u_tsize, u.u_dsize, u.u_ssize, u.u_sep, RO);
		if (i<0)
			goto error;
		if (xp)
			xp->x_flag |= XWRIT;
		break;

	/* write user D */
	case PT_WRITE_D:
		if (suword((caddr_t)ipc.ip_addr, 0) < 0)
			goto error;
		suword((caddr_t)ipc.ip_addr, ipc.ip_data);
		break;

	/* write u */
	case PT_WRITE_U:
		i = (int)ipc.ip_addr;
		p = (int *)&((physadr)&u)->r[i/sizeof(int)];
		if (p >= (int *)&u.u_fps && p < (int *)&u.u_fps.u_fpregs[6])
			goto ok;
		for (i=0; i<8; i++)
			if (p == &u.u_ar0[regloc[i]])
				goto ok;
		if (p == &u.u_ar0[RPS]) {
			ipc.ip_data |= PSL_USERSET;	/* user space */
			ipc.ip_data &= ~PSL_USERCLR;	/* priority 0 */
			goto ok;
		}
		if ((p == (int *)&u.u_ovdata.uo_curov) && ((ipc.ip_data >= 0) &&
		    (ipc.ip_data <= NOVL) && u.u_ovdata.uo_ovbase)) {
			u.u_ovdata.uo_curov = ipc.ip_data;
			choverlay(RW);
			break;
		}
		goto error;

	ok:
		*p = ipc.ip_data;
		break;

	/* set signal and continue */
	/*  one version causes a trace-trap */
	case PT_STEP:
		u.u_ar0[RPS] |= PSL_T;
		/* FALL THROUGH TO ... */
	case PT_CONTINUE:
		if ((int)ipc.ip_addr != 1)
			u.u_ar0[PC] = (int)ipc.ip_addr;
		if (ipc.ip_data > NSIG)
			goto error;
		u.u_procp->p_ptracesig = ipc.ip_data;
		return(1);

	/* force exit */
	case PT_KILL:
		exit(u.u_procp->p_ptracesig);
		/*NOTREACHED*/

	default:
	error:
		ipc.ip_req = -1;
	}
	return(0);
}

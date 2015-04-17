/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dr.c	1.5 (2.11BSD GTE) 1997/2/14
 */

/*
 *	DR11-W device driver
 */

#include "dr.h"
#if NDR > 0

#include "param.h"
#include "../machine/seg.h"

#include "user.h"
#include "proc.h"
#include "buf.h"
#include "conf.h"
#include "ioctl.h"
#include "drreg.h"
#include <sys/kernel.h>

struct	dr11w {
	int	i_flags;		/* interface flags */
	int	i_req;			/* request number (for timeout) */
	int	i_unit;			/* unit number of device */
	int	i_prev;			/* previous request number (timeout) */
	short	i_fun;			/* function bits */
	struct	proc *i_proc;		/* process pointer of opening process */
	int	i_sig;			/* signal to send on ATTN */
	int	i_tsig;			/* signal to send on timeout */
	struct	buf i_tab;		/* buffer for device */
	struct	drdevice *i_addr;	/* address of DR11-W interface */
};

struct dr11w dr11[NDR];

drattach(addr, unit)
struct drdevice *addr;
int unit;
{
	if(unit > NDR)
		return(0);
	if((addr != (struct drdevice *)NULL) && (fioword(addr) != -1))
	{
		dr11[unit].i_addr = addr;
		dr11[unit].i_flags = DR_ALIVE;	/* mark as active */
		dr11[unit].i_unit = unit;
		return(1);
	}
	return(0);
}

dropen(dev)
dev_t dev;
{
	register int unit;
	register struct dr11w *drptr;
	extern int drtimeout();

	unit = minor(dev) & 07;		/* get minor device number */
	drptr = &dr11[unit];
	if((unit > NDR) || !(drptr->i_flags & DR_ALIVE))
		return(ENXIO);		/* not attatched or present */
	drptr->i_flags |= DR_OPEN;
	drptr->i_flags &= ~(DR_IGNORE | DR_TIMEOUT);
	drptr->i_proc = u.u_procp;	/* process pointer of opener */
	drptr->i_sig = 0;		/* clear signals (set by ioctl) */
	drptr->i_tsig = 0;
	drptr->i_fun = 0;		/* clear function */
	timeout(drtimeout, (caddr_t)drptr, hz);
	return(0);
}

drclose(dev, flag)
dev_t dev;
{
	register int unit;
	register struct drdevice *addr;

	unit = minor(dev) & 07;
	dr11[unit].i_flags &= ~DR_OPEN;		/* clear opened status */
	addr = dr11[unit].i_addr;		/* get address of DR11-W */
	addr->csr = dr11[unit].i_fun;		/* clear IE and GO bits */
}

drstrategy(bp)
register struct buf *bp;
{
	register struct buf *dp;
	register struct dr11w *drptr;
	int s;

	drptr = &dr11[minor(bp->b_dev) & 07];
	if(!(drptr->i_flags & DR_OPEN))
	{
		bp->b_flags |= B_ERROR;	/* unit not open */
		iodone(bp);
		return;
	}
	dp = &(drptr->i_tab);		/* point to buffer */
	bp->av_forw = NULL;
	s = splbio();			/* lock out interrupts */

	mapalloc(bp);

	if(dp->b_actf == NULL)		/* if nothing in current buffer */
		dp->b_actf = bp;	/* this request is first */
	dp->b_actl = bp;		/* set last request */
	if(dp->b_active == 0)		/* if not active now */
		drstart(drptr);		/* start the DR11-W */
	splx(s);			/* return to normal state */
}

drstart(drptr)
register struct dr11w *drptr;
{
	register struct buf *bp, *dp;
	register struct drdevice *addr;
	short com;

	if(!drptr)
		return;
	dp = &(drptr->i_tab);		/* point dp to device buffer */
	if((bp = dp->b_actf) == NULL)	/* if nothing in queue */
		return;			/* return */

	drptr->i_req++;			/* increment request number */
	if(drptr->i_flags & DR_TIMEOUT)	/* do we need timeout checking */
	{
		drptr->i_flags |= DR_TACTIVE;	/* mark active timeout */
	}
	dp->b_active = 1;		/* set active flag */
	addr = drptr->i_addr;		/* get device pointer */
	/*
	 *	Set up DR11-W for specific operation
	 */
	addr->bar = (short)bp->b_un.b_addr;
	addr->wcr = -(bp->b_bcount >> 1);	/* DR deals in words */
	com = ((bp->b_xmem & 3) << 4) | drptr->i_fun;
	addr->csr = com;			/* set csr fun and address */
	com |= (DR_IE | DR_GO);			/* set IE and GO bits (also) */
	addr->csr = com;
}

drintr(unit)
int unit;
{
	register struct buf *bp;
	register struct drdevice *dr;
	register struct dr11w *drptr;
	mapinfo map;

	drptr = &dr11[unit];		/* point to struct for device */
	dr = drptr->i_addr;		/* get address of device */
	if(drptr->i_tab.b_active == 0)	/* if not active, return */
		return;
	bp = drptr->i_tab.b_actf;	/* point to current buffer */
	if(dr->csr & DR_ERR)		/* if error */
	{
		/*
		 * The error bit can be set in one of two ways:
		 * a 'real' error (timing, non-existant memory) or
		 * by the interface setting ATTN true.  This is
		 * not considered an 'error' but cause to send
		 * a signal to the parent process.  He (hopefully)
		 * will know what to do then.
		 */
		if(dr->csr & DR_ATTN)
		{
			dr->csr = drptr->i_fun;
			savemap(map);
			if(drptr->i_sig)
				psignal(drptr->i_proc, drptr->i_sig);
			restormap(map);
		}
		else
		{
			printf("dr%d: error ", unit);
			printf("csr=%b, ", dr->csr, DRCSR_BITS);
			dr->csr = DR_ERR | drptr->i_fun;
			printf("eir=%b\n", dr->csr, DREIR_BITS);
			/* now reset the DR11-W interface */
			dr->csr = DR_MANT | drptr->i_fun;
			dr->csr = drptr->i_fun;
			bp->b_flags |= B_ERROR;
		}
	}
	drptr->i_flags &= ~DR_TACTIVE;		/* we beat the timeout */
	drptr->i_tab.b_active = 0;		/* clear global stuff */
	drptr->i_tab.b_errcnt = 0;
	drptr->i_tab.b_actf = bp->b_forw;	/* link to next request */
	bp->b_resid = -(dr->wcr) << 1;		/* change words to bytes */
	iodone(bp);				/* tell system we are done */
	if(drptr->i_tab.b_actf)			/* start next request */
		drstart(drptr);
}

drioctl(dev, cmd, data, flag)
	dev_t dev;
	u_int cmd;
	register caddr_t data;
	int flag;
{
	register struct dr11w *drptr;
	register struct drdevice *dr;
	int *sgbuf = (int *)data;

	drptr = &dr11[minor(dev) & 07];
	dr = drptr->i_addr;
	if(drptr->i_tab.b_active)
		return (EINVAL);

	switch(cmd) {
		case DRGTTY:
			sgbuf[0] = drptr->i_flags;
			sgbuf[1] = drptr->i_fun >> 1;
			break;

		case DRSTTY:
			drptr->i_fun = (sgbuf[1] & 07) << 1;
			dr->csr = drptr->i_fun;
			drptr->i_flags &= ~(DR_TIMEOUT | DR_IGNORE);
			drptr->i_flags |= sgbuf[0] & (DR_TIMEOUT | DR_IGNORE);
			break;

		case DRSFUN:
			drptr->i_fun = (sgbuf[0] & 07) << 1;
			dr->csr = drptr->i_fun;
			break;

		case DRSFLAG:
			drptr->i_flags &= ~(DR_TIMEOUT | DR_IGNORE);
			drptr->i_flags |= sgbuf[0] & (DR_TIMEOUT | DR_IGNORE);
			break;

		case DRGCSR:
			sgbuf[0] = dr->csr;
			sgbuf[1] = dr->wcr;
			break;

		case DRSSIG:
			drptr->i_sig = sgbuf[0];
			if((drptr->i_sig < 0) || (drptr->i_sig > 15)) {
				drptr->i_sig = 0;
				return (EINVAL);
			}
			break;

		case DRESET:
			dr->csr = DR_MANT | drptr->i_fun;
			dr->csr = drptr->i_fun;
			break;

		case DRSTIME:
			drptr->i_tsig = sgbuf[0];
			if((drptr->i_tsig < 0) || (drptr->i_tsig > 15)) {
				drptr->i_tsig = 0;
				return (EINVAL);
			}
			drptr->i_flags |= DR_TIMEOUT;
			drptr->i_flags &= ~DR_IGNORE;
			break;

		case DRCTIME:
			drptr->i_flags &= ~(DR_TIMEOUT | DR_IGNORE);
			break;

		case DRITIME:
			drptr->i_flags |= DR_IGNORE;
			break;

		case DROUTPUT:
			dr->dar = sgbuf[0];
			break;

		case DRINPUT:
			sgbuf[0] = dr->dar;
			break;

		default:
			return (EINVAL);
	}
	return (0);
}

drtimeout(ptr)
caddr_t ptr;
{
	register struct dr11w *drptr;
	mapinfo map;

	drptr = (struct dr11w *)ptr;
	if((drptr->i_flags & DR_TACTIVE) && (drptr->i_req == drptr->i_prev))
	{
		printf("dr%d: timeout error\n", drptr->i_unit);
		savemap(map);
		if(drptr->i_tsig)
			psignal(drptr->i_proc, drptr->i_tsig);

		restormap(map);
		drabort(drptr);
		savemap(map);
		if(drptr->i_tab.b_actf)
			drstart(drptr);		/* start next request */
		restormap(map);
	}
	if(drptr->i_flags & (DR_TACTIVE | DR_OPEN))
	{
		drptr->i_prev = drptr->i_req;	/* arm timeout */
		timeout(drtimeout, ptr, hz);
	}
}

drabort(drptr)
register struct dr11w *drptr;
{
	register struct buf *bp;
	register struct drdevice *dr;
	mapinfo map;

	savemap(map);
	dr = drptr->i_addr;			/* point to device */
	bp = drptr->i_tab.b_actf;		/* point to current buffer */
	drptr->i_flags &= ~DR_TACTIVE;		/* turn off timeout active */
	drptr->i_tab.b_active = 0;		/* clean up global stuff */
	drptr->i_tab.b_errcnt = 0;
	drptr->i_tab.b_actf = bp->b_forw;	/* link to next request */
	bp->b_resid = -(dr->wcr) << 1;		/* make it bytes */
	if(!(drptr->i_flags & DR_IGNORE))
		bp->b_flags |= B_ERROR;		/* set an error condition */
	dr->csr = DR_MANT | drptr->i_fun;	/* clear IE bit in csr */
	dr->csr = drptr->i_fun;			/* restore function bits */
	restormap(map);
	iodone(bp);				/* done with that request */
}
#endif NDR

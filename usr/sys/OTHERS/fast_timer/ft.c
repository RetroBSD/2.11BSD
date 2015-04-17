#include "ft.h"
#if NFT > 0
/*
 *	Fast Timer -- a pseudo-device that behaves like an interval timer.
 *
 *	Note:  because of exclusive open it can also be used as a lock file.
 */

#include "param.h"
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/map.h>
#include <sys/conf.h>

/*
 * Per-minor-device state table
 *
 * Invariants:
 *	- device is open iff ftt_procp != 0
 *	- timeout in progress iff ftt_state != AVAILABLE
 */

struct	fttab	{
	struct	proc	*ftt_procp;	/* proctab entry and pid of	*/
	short	ftt_pid;		/* controlling process.		*/
	short	ftt_state;		/* AVAILABLE, CANCELLED, sig #	*/
} fttab[NFT];

#define	AVAILABLE	(0)
#define	CANCELLED	(-1)

#define	MAXTIMEREQ	(60 * hz)	/* 1 minute */

/*
 *	Open 
 *
 *	Exclusive use only
 */
/*ARGSUSED*/
ftopen(d, flag)
{
	register u_short	dminor;
	register struct	fttab	*ftp;
 
	dminor	= minor(d);
	if (dminor >= NFT)	{
		u.u_error	= ENXIO;
		return;
	}
	ftp	= &fttab[dminor];
	if ((ftp->ftt_procp != (struct proc *) NULL)
	  || (ftp->ftt_state != AVAILABLE))	{
		u.u_error	= EBUSY;
		return;
	}
	ftp->ftt_procp	= u.u_procp;
	ftp->ftt_pid	= u.u_procp->p_pid;
}
 
/*
 *	Close
 */
ftclose(d)
{
	register u_short	dminor;
	register struct	fttab	*ftp;

	dminor	= minor(d);
#ifdef	PARANOIA
		if (dminor >= NFT)	{
			printf("ERR: ftclose\n");
			u.u_error	= ENXIO;
			return;
		}
#endif
	ftp	= &fttab[dminor];
	ftp->ftt_procp	= (struct proc *) NULL;
	ftp->ftt_pid	= 0;
}

/*
 *	Ioctl
 *
 *	All real work is done here using
 *
 *	 	FTIOCSET	request a software interrupt after specified
 *				number of clock ticks
 *		FTIOCCANCEL	cancel an outstanding request
 */
/*ARGSUSED*/
ftioctl(dev, cmd, addr, flag)
caddr_t	addr;
dev_t	dev;
{
	void	fttimeout();
	register u_short	dminor;
	register struct	fttab	*ftp;

	dminor	= minor(dev);
	if (dminor >= NFT)	{
		printf("ERR: ftioctl!\n");
		u.u_error	= EINVAL;
	}

	ftp	= &fttab[dminor];

	/*
	 *	Make sure invoking process owns the device.
	 *	This can only fail if call is from a child
	 *	forked off after opening the ft device.
	 */
	if ((u.u_procp != ftp->ftt_procp)
	  || (u.u_procp->p_pid != ftp->ftt_pid))	{
		u.u_error	= EBUSY;
		return;
	}

	/*
	 *	Interpret command
	 */
	switch(cmd)	{

		case FTIOCSET:
			{
			struct	{
				u_short timereq;
				short	sigreq;
			} request;

			/*
			 *	Get request, make sure it's reasonable
			 */
			if (copyin(addr, (caddr_t) &request, sizeof request))	{
				u.u_error	= EFAULT;
				return;
			}
			if ((request.sigreq <= 0) || (request.sigreq > NSIG) 
			  || (request.timereq > MAXTIMEREQ))	{
				u.u_error	= EINVAL;
				return;
			}
			/*
			 *	Schedule the interrupt
			 */
			if (ftp->ftt_state != AVAILABLE)	{
				u.u_error	= EBUSY;
				return;
			}
			ftp->ftt_state	= request.sigreq;
			timeout(fttimeout, (caddr_t) ftp, request.timereq);
			}
			break;

		case FTIOCCANCEL:
			(void) spl5();
			if (ftp->ftt_state != AVAILABLE)
				ftp->ftt_state	= CANCELLED;
			(void) spl0();
			break;

		default:
			u.u_error	= ENOTTY;
			break;
	}
}

/*
 *	Fttimeout
 *
 *	Called by system timeout
 *
 *	Verify that requesting process is still around
 *	Verify that request hasn't been cancelled
 *	Send the signal
 */
void
fttimeout(ftp)
register struct	fttab	*ftp;
{
	register struct	proc	*pp;
	register prevstate;

	prevstate	= ftp->ftt_state;
	ftp->ftt_state	= AVAILABLE;

	if ((prevstate == CANCELLED)
	  || ((pp = ftp->ftt_procp) == ((struct proc *) NULL))
	  || (pp->p_pid != ftp->ftt_pid))
		return;

#ifdef	PARANOIA
	if ((prevstate < 1) || (prevstate >= NSIG))	{
		printf("ERR: fttimeout %d\n",prevstate);
		return;
	}
#endif
	psignal(pp, prevstate);
}
#endif

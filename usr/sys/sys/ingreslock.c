/*
 * Rewritten for 2.11BSD. Feb 20 1995, Steven Schultz (sms@wlv.iipo.gtegsc.com)
 *
 * The lock table is allocated external to the kernel (in pdp/machdep2.c at
 * kernel startup time) because 8 concurrent Ingres sessions would have cost 
 * over 500 bytes of D space.
 */

#include	"ingres.h"
#if	NINGRES > 0

#include	"param.h"
#include	<sys/file.h>
#include 	<sys/user.h>
#include	<sys/proc.h>
#include	<sys/uio.h>
#include	<machine/seg.h>
#include	<sys/ingreslock.h>

/*
 * Wait channels for locktable.  We need something which is even, unique and
 * not mapped out to sleep on when waiting for a lock.
*/
	int	Locksleep[IL_NLOCKS];
	segm	Locktabseg;
	struct	Lockform *Locktab = (struct Lockform *)SEG5;

/*
 * array of number of locks which can be set for each lock.
 * It looks tempting to make this an array of char or u_char.  DON'T.  The
 * entries are used as wait channel addresses and must be 'even'.
*/
	int	Lockset[] =
		{
		IL_NLOCKS,
		IL_PLOCKS,
		IL_RLOCKS,
		IL_DLOCKS
		};

#define	keycomp(a,b)	bcmp(a,b,KEYSIZE)

ingres_open(dev, flag, mode)
	dev_t	dev;
	int	flag;
	int	mode;
	{

	if	((flag & FWRITE) == 0)
		return(EBADF);
	if	(Locktabseg.se_addr == 0)
		return(ENOMEM);
	return(0);
	}

/*	
 *	ingres_write() : write driver
 *		1. copy Lock request info to lockbuf
 *		2. follow action in l_act
 *		3. Error return conditions
 *			-1: lockrequest fails(only on act=1)
 *			-2: attempt to release a lock not set
 *			    by calling program
 *			-3: illegal action requested
 *
 *	Install the line "ingres_rma(p->p_pid)" in the routine
 *	"exit" (in sys/kern_exit.c) after "p->p_stat = SZOMB".
*/

ingres_write(dev, uio, flag)
	dev_t	dev;
	struct	uio	*uio;
	int	flag;
{
	struct Lockreq	lockbuf;
	register struct Lockreq *ll;
	register int	i;
	int	error = 0, blockflag;

	if	(uio->uio_resid != sizeof (struct ulock))
		return(EINVAL);
	error = uiomove(&lockbuf.lr_req, sizeof (struct ulock), uio);
	if	(error)
		return(error);
	lockbuf.lr_pid = u.u_procp->p_pid;
	ll = &lockbuf;
	if	((ll->lr_act < A_RLS1)
		   && ((ll->lr_type < T_CS) || (ll->lr_type > T_DB )
	   	   || (ll->lr_mod < M_EXCL) || (ll->lr_mod > M_SHARE )))
		return(EINVAL);

/*
 * At this point we are in the high kernel and do not need to save seg5
 * before changing it.  Making sure that 'normalseg5' is called before doing
 * a sleep() or return() is sufficient.
 *
 * It is simpler to map the lock table once here rather than in each routine
 * called below.
*/
	ingres_maplock();

	switch(ll->lr_act)
	{
	  case A_RTN:
					/*
					 * attempt to set lock.
					 * error return if failure.
					 */
		blockflag = FALSE;
		for ( i = 0; i <= ll->lr_type; i++)
			if (Lockset[i] == 0)
				blockflag = TRUE;
		if (blockflag || ingres_unique(ll) >= 0)
			error = -1;
		else
			ingres_enter(ll);
		break;

	  case A_SLP:
				/* attempt to set lock.
				 * sleep on blocking address if failure.
				 */
		do
		{
			do
			{
				blockflag = TRUE;
				for ( i = 0; i <= ll->lr_type; i++)
					if (Lockset[i] == 0)
						{
						normalseg5();
						sleep(&Lockset[i],LOCKPRI);
						ingres_maplock();
						blockflag = FALSE;
						}
			}
			while (!blockflag);
			if (( i = ingres_unique(ll)) >= 0 )
				{
				blockflag = FALSE;
				Locktab[i].l_wflag = W_ON;
				normalseg5();
				sleep(&Locksleep[i],LOCKPRI);
				ingres_maplock();
				}
		}
		while (!blockflag);
		ingres_enter(ll);
		break;

	  case A_RLS1:
				/* remove 1 lock */
		if ((i = ingres_find(ll)) >= 0)
			ingres_rm(i,ll->lr_pid);
		else
			error = -2;
		break;

	  case A_RLSA:
				/* remove all locks for this process id*/
		ingres_rma(ll->lr_pid);	/* does a normalseg5() */
		break;

	  case A_ABT:		/* abort all locks */
		ingres_abt();
		break;

	  default :
		error = -3;
		break;
	}
	normalseg5();
	return(error);
}

/*
 *	ingres_unique- check for match on key
 *	
 *	return index of Locktab if match found
 *	else return -1
 */

static
ingres_unique(q)
register struct	Lockreq	*q;
{
	register int	k;
	register struct Lockform	*p = Locktab;

	for	(k = 0; k < IL_NLOCKS; k++, p++)
		{
		if	((p->l_mod != M_EMTY)
			  && (keycomp(p->l_key,q->lr_key) == 0)
			  && (p->l_type == q->lr_type)
			  && ((p->l_mod == M_EXCL) || (q->lr_mod == M_EXCL)))
			return(k);
		}
	return(-1);
}

static
ingres_find(q)
register struct	Lockreq	*q;
	{
	register int	k;
	register struct Lockform	*p = Locktab;

	for	(k = 0; k < IL_NLOCKS; k++, p++)
		{
		if	((p->l_mod != M_EMTY)
			  && (keycomp(p->l_key,q->lr_key) == 0)
			  && (p->l_type == q->lr_type)
			  && (p->l_pid == q->lr_pid))
			return(k);
		}
	return(-1);
	}

/*
 *	remove the lth Lock
 *		if the correct user is requesting the move.
 */

static void
ingres_rm(l,llpid)
	int	l, llpid;
	{
	register struct Lockform *a = &Locktab[l];
	register int	k;

	if	(a->l_pid == llpid && a->l_mod != M_EMTY)
		{
		a->l_mod = M_EMTY;
		a->l_pid = 0;
		if	(a->l_wflag == W_ON)
			{
			a->l_wflag = W_OFF;
			wakeup(&Locksleep[l]);
			}
		for	(k = 0; k <= a->l_type; k++)
			{
			Lockset[k]++;
			if	(Lockset[k] == 1)
				wakeup(&Lockset[k]);
			}
		}
	}

/*
 *	ingres_rma releases all locks for a given process id(pd).  
*/

ingres_rma(pd)
	int	pd;
	{
	register int	i;
	register struct	Lockform *p = Locktab;
/*
 * Have to map the lock table because we can be called from kern_exit.c
 * when a process exits.
*/
	ingres_maplock();

/*
 * Replicate the pid check here to avoid function calls.  If this process
 * has no Ingres locks outstanding then we avoid IL_NLOCKS function calls
 * and returns.
*/
	for	(i = 0; i < IL_NLOCKS; i++, p++)
		{
		if	(p->l_pid == pd && (p->l_mod != M_EMTY))
			ingres_rm(i,pd);
		}
	normalseg5();
	}

/*
 *	enter Lockbuf in locktable
 *	return position in Locktable
 *	error return of -1
 */

static
ingres_enter(ll)
struct Lockreq	*ll;
	{
	register int	k, l;
	register struct Lockform	*p = Locktab;

	for	(k = 0; k < IL_NLOCKS; k++, p++)
		{
		if	(p->l_mod == M_EMTY)
			{
			p->l_pid = ll->lr_pid;
			p->l_type = ll->lr_type;
			p->l_mod = ll->lr_mod;
			bcopy(ll->lr_key, p->l_key, KEYSIZE);
			for	(l = 0; l <= ll->lr_type; l++)
				Lockset[l]--;
			return(k);
			}
		}
	return (-1);
	}

/*
 *	ingres_abt - abort all locks
 */

static void
ingres_abt()
	{
	register int	k;

	for (k = 0; k < IL_NLOCKS; k++)
		wakeup( &Locktab[k] );
	for (k = 0; k < 4; k++)
		wakeup( &Lockset[k]);
	bzero(Locktab, LOCKTABSIZE);
	Lockset[0] = IL_NLOCKS;
	Lockset[1] = IL_PLOCKS;
	Lockset[2] = IL_RLOCKS;
	Lockset[3] = IL_DLOCKS;
	}
#endif /* NINGRES > 0 */

/*
 *	SCCS id	@(#)rp.c	2.2 (2.11BSD GTE) 1/2/93
 */

/*
 *	RP03 disk driver
 */

#ifdef AUTOCONFIG
#include "param.h"
#include "rpreg.h"
#include "autoconfig.h"
rpprobe(addr)
struct rpdevice *addr;
{
	stuff(RP_IDE | RP_IDLE | RP_GO, (&(addr->rpcs.w)));
	WAIT(10);
	stuff(0, (&(addr->rpcs.w)));
	return(ACP_IFINTR);
}
#else !AUTOCONFIG

#include "rp.h"
#if	NRP > 0
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "user.h"
#include "rpreg.h"

struct	rpdevice *RPADDR = (struct rpdevice *)0176710;

#ifdef SHOULD_WORK_BUT_DOESN'T
/*
 * Apparently, some controllers have problems getting to the "spare*
 * 6 tracks on the RP03's.  The second set of tables ignores those tracks.
 */
struct	size rp_sizes[] = {
	10400,	0,		/* cyl   0 -  51 */
	5200,	52,		/* cyl  52 -  77 */
	65600,	78,		/* cyl  78 - 405 */
	0,	0,		/* not defined */
	0,	0,		/* not defined */
	0,	0,		/* not defined */
	0,	0,		/* not defined */
	81200,	0,		/* cyl   0 - 405 */
};
#endif SHOULD_WORK_BUT_DOESN'T

struct	size rp_sizes[] = {
	10400,	0,		/* cyl   0 -  51 */
	5200,	52,		/* cyl  52 -  77 */
	64400,	78,		/* cyl  78 - 399 */
	0,	0,		/* not defined */
	0,	0,		/* not defined */
	0,	0,		/* not defined */
	0,	0,		/* not defined */
	80000,	0,		/* cyl   0 - 399 */
};

struct	buf	rptab;

#define	RP_NSECT	10
#define	RP_NTRAC	20

rpattach(addr, unit)
struct rpdevice *addr;
{
	if (unit != 0)
		return(0);
	RPADDR = addr;
	return(1);
}

rpstrategy(bp)
register struct	buf *bp;
{
	register struct buf *dp;
	register int unit;
	long	sz;

	unit = minor(bp->b_dev);
	sz = bp->b_bcount;
	sz = (sz + 511) >> 9;
	if (RPADDR == (struct rpdevice *) NULL) {
		bp->b_error = ENXIO;
		goto errexit;
	}
	if (unit >= (NRP << 3) || bp->b_blkno + sz > rp_sizes[unit & 07].nblocks) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	mapalloc(bp);
	bp->av_forw = NULL;
	unit >>= 3;
	(void) _spl5();
	dp = &rptab;
	if (dp->b_actf == NULL)
		dp->b_actf = bp;
	else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	if (dp->b_active == NULL)
		rpstart();
	(void) _spl0();
}

rpstart()
{
	register struct rpdevice *rpaddr = RPADDR;
	register struct buf *bp;
	register int unit;
	int	com, cn, tn, sn, dn;
	daddr_t	bn;


	if ((bp = rptab.b_actf) == NULL)
		return;
	rptab.b_active++;
	unit = minor(bp->b_dev);
	dn = unit >> 3;
	bn = bp->b_blkno;
	cn = bn / (RP_NTRAC * RP_NSECT) + rp_sizes[unit & 07].cyloff;
	sn = bn % (RP_NTRAC * RP_NSECT);
	tn = sn / RP_NSECT;
	sn = sn % RP_NSECT;
	rpaddr->rpcs.w = (dn << 8);
	rpaddr->rpda = (tn << 8) | sn;
	rpaddr->rpca = cn;
	rpaddr->rpba = bp->b_un.b_addr;
	rpaddr->rpwc = -(bp->b_bcount >> 1);
	com = ((bp->b_xmem & 3) << 4) | RP_IDE | RP_GO;
	if (bp->b_flags & B_READ)
		com |= RP_RCOM;
	else
		com |= RP_WCOM;
	
	rpaddr->rpcs.w |= com;
#ifdef	RP_DKN
	dk_busy |= 1 << RP_DKN;
	dk_numb[RP_DKN]++;
	dk_wds[RP_DKN] += bp->b_bcount >> 6;
#endif	RP_DKN
}

rpintr()
{
	register struct rpdevice *rpaddr = RPADDR;
	register struct buf *bp;
	register int ctr;

	if (rptab.b_active == NULL)
		return;
#ifdef	RP_DKN
	dk_busy &= ~(1 << RP_DKN);
#endif	RP_DKN
	bp = rptab.b_actf;
	rptab.b_active = NULL;
	if (rpaddr->rpcs.w & RP_ERR) {
		while ((rpaddr->rpcs.w & RP_RDY) == 0)
			;
		if (rpaddr->rper & RPER_WPV)
			/*
			 *	Give up on write locked devices
			 *	immediately.
			 */
			printf("rp%d: write locked\n", minor(bp->b_dev));
		else
			{
#ifdef	UCB_DEVERR
			harderr(bp, "rp");
			printf("er=%b ds=%b\n", rpaddr->rper, RPER_BITS,
				rpaddr->rpds, RPDS_BITS);
#else
			deverror(bp, rpaddr->rper, rpaddr->rpds);
#endif
			if(rpaddr->rpds & (RPDS_SUFU | RPDS_SUSI | RPDS_HNF)) {
				rpaddr->rpcs.c[0] = RP_HSEEK | RP_GO;
				ctr = 0;
				while ((rpaddr->rpds & RPDS_SUSU) && --ctr)
					;
			}
			rpaddr->rpcs.w = RP_IDLE | RP_GO;
			ctr = 0;
			while ((rpaddr->rpcs.w & RP_RDY) == 0 && --ctr)
				;
			if (++rptab.b_errcnt <= 10) {
				rpstart();
				return;
			}
		}
		bp->b_flags |= B_ERROR;
	}
	rptab.b_errcnt = 0;
	rptab.b_actf = bp->av_forw;
	bp->b_resid = -(rpaddr->rpwc << 1);
	iodone(bp);
	rpstart();
}
#endif NRP
#endif AUTOCONFIG

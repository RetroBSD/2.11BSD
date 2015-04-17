/*
 *	SCCS id	@(#)rf.c	2.2 (2.11BSD GTE) 1/2/93
 */

#include "rf.h"
#if	NRF > 0
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "user.h"
#include "rfreg.h"

#define	NRFBLK	1024

struct	rfdevice *RFADDR = (struct rfdevice *)0177460;

struct	buf	rftab;

rfattach(addr, unit)
struct rfdevice *addr;
{
	if (unit != 0)
		return(0);
	RFADDR = addr;
	return(1);
}

rfstrategy(bp)
register struct buf *bp;
{
	if (RFADDR == (struct rfdevice *) NULL) {
		bp->b_error = ENXIO;
		goto errexit;
	}
	if (bp->b_blkno >= NRFBLK * (minor(bp->b_dev) + 1)) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	mapalloc(bp);
	bp->av_forw = (struct buf *) NULL;
	(void) _spl5();
	if (rftab.b_actf == NULL)
		rftab.b_actf = bp;
	else
		rftab.b_actl->av_forw = bp;
	rftab.b_actl = bp;
	if (rftab.b_active == NULL)
		rfstart();
	(void) _spl0();
}

rfstart()
{
	register struct rfdevice *rfaddr = RFADDR;
	register struct buf *bp;
	register com;

	if ((bp = rftab.b_actf) == (struct buf *) NULL)
		return;
	rftab.b_active++;
	rfaddr->rfdar = (short) (bp->b_blkno << 8) & 0177777;
	rfaddr->rfdae = (short) (bp->b_blkno >> 8) & 037;
	rfaddr->rfcma = bp->b_un.b_addr;
	rfaddr->rfwc = - (bp->b_bcount >> 1);
	com = (bp->b_xmem & 3) << 4;
	if (bp->b_flags & B_READ)
		com |= RF_RCOM | RF_IENABLE | RF_GO;
	else
		com |= RF_WCOM | RF_IENABLE | RF_GO;
	rfaddr->rfdcs = com;

#ifdef	RF_DKN
	dk_busy |= 1 << RF_DKN;
	dk_numb[RF_DKN]++;
	dk_wds[RF_DKN] += (bp->b_bcount) >> 6;
#endif
}

rfintr()
{
	register struct rfdevice *rfaddr = RFADDR;
	register struct buf *bp;

	if (rftab.b_active == (struct buf *) NULL)
		return;
#ifdef	RF_DKN
	dk_busy &= ~ (1 << RF_DKN);
#endif
	bp = rftab.b_actf;
	rftab.b_active = (struct buf *) NULL;
	if (rfaddr->rfdcs & RF_ERR) {
		while ((rfaddr->rfdcs & RF_RDY) == 0)
			;
		if (rfaddr->rfdcs & RF_WLO)
			/*
			 *	Give up on write locked devices
			 *	immediately.
			 */
			printf("rf%d: write locked\n", minor(bp->b_dev));
		else
			{
#ifdef	UCB_DEVERR
			harderr(bp, "rf");
			printf("cs=%b dae=%b\n", rfaddr->rfdcs,
				RF_BITS, rfaddr->rfdae, RFDAE_BITS);
#else
			deverror(bp, rfaddr->rfdcs, rfaddr->rfdae);
#endif
			rfaddr->rfdcs = RF_CTLCLR;
			if (++rftab.b_errcnt <= 10) {
				rfstart();
				return;
			}
		}
		bp->b_flags |= B_ERROR;
	}
	rftab.b_errcnt = 0;
	rftab.b_actf = bp->av_forw;
	bp->b_resid = -(rfaddr->rfwc << 1);
	iodone(bp);
	rfstart();
}
#endif	NRF

/*
 * ML11 solid state disk driver.
 *
 * This driver was written by Fred Canter and distributed on the
 * DEC v7m UNIX tape.  It has been modified for 2BSD and has been
 * included with the permission of the DEC UNIX Engineering Group.
 *
 * This driver does not support mixed drive types,
 * it requires that the ML11 be on a separate
 * RH11 or RH70 controller and that there be only
 * ML11 drives on that controller.
 */

/*
 *	SCCS id	@(#)ml.c	2.2 (2.11BSD GTE) 1/2/93
 */

#include "ml.h"
#if	NML > 0
#include "param.h"
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/seg.h>
#include <sys/mlreg.h>

bool_t	ml_alive;
extern	struct	mldevice *MLADDR;

/*
 * Ml_sizes[] contains the size of each ML11
 * unit in blocks.  The size is set dynamicaly
 * by reading the number of arrays from the ML maintenance
 * register.  The array type bit the the ML maintenacne
 * register is used to adjust the number of blocks per array
 * from 512 (16k chips) to 2048 (64k chips).
 */
short	ml_sizes[NML];

struct	buf	mltab;

void
mlprobe()
{
	register unit;

	if (fioword (MLADDR) != -1) {
		ml_alive++;
		for (unit = 0; unit < NML; unit++) {
			MLADDR->mlcs2.c[0] = unit;

			/*
			 * Find the size of the ML11 unit by reading
			 * the number of array modules from the ML
			 * maintenance register.  There are 512
			 * blocks per array unless the array type
			 * bit is set, in which case there are 2048
			 * blocks per array.
			 */
			ml_sizes[unit] = (MLADDR->mlmr >> 2) & 037000;
			if (MLADDR->mlmr & 02000)
				ml_sizes[unit] <<= 2;
		}
		if (fioword (&(MLADDR->mlbae)) != -1)
			mltab.b_flags |= B_RH70;
	}
}


mlstrategy(bp)
register struct buf *bp;
{
	register struct buf *dp;
	register unit;
	int tr;
	long sz, bn;

	unit = minor(bp->b_dev) & 07;
	if (!ml_alive || unit >= NML)
		goto bad;
	if ((bp->b_flags & B_PHYS) && ((mltab.b_flags & B_RH70) == 0))
		mapalloc(bp);

	(void) _spl5();
	if ((MLADDR->mldt & 0377) != ML11)
		goto bad;
	MLADDR->mlcs2.c[0] = unit;

	/*
	 * Check the ML11 transfer rate.
	 * 2mb is too fast for any pdp11.
	 * 1mb allowed on pdp11/70 only.
	 * .5mb & .25mb ok on any processor.
	 */
	tr = MLADDR->mlmr & 01400;
	if ((tr == MLMR_2MB) || ((tr == MLMR_1MB)
	    && ((mltab.b_flags & B_RH70) == 0))) {
		printf("\nML11 xfer rate error\n");
		goto bad;
	}
	sz = bp->b_bcount;
	sz = (sz + 511) >> 9;
	if (bp->b_blkno < 0 || (bp->b_blkno + sz) > ml_sizes[unit]) {
		goto bad;
	}

	bp->av_forw = NULL;
	dp = &mltab;
	if (dp->b_actf == NULL)
		dp->b_actf = bp;
	else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	if (dp->b_active == NULL)
		mlstart();
	return;

bad:
	u.u_error = ENXIO;
	bp->b_flags |= B_ERROR;
	iodone(bp);
}

mlstart()
{
	register struct buf *bp;
	register unit, com;

	if ((bp = mltab.b_actf) == NULL)
		return;
	mltab.b_active++;
	unit = minor(bp->b_dev) & 07;
	(void) _spl5();
	MLADDR->mlcs2.w = unit;
	if ((MLADDR->mlds & MLDS_VV) == 0)
		MLADDR->mlcs1.w = ML_PRESET | ML_GO;
	if ((MLADDR->mlds & (MLDS_DPR | MLDS_MOL)) != (MLDS_DPR | MLDS_MOL)) {
		mltab.b_active = 0;
		mltab.b_errcnt = 0;
		bp->b_actf = bp->av_forw;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		(void) _spl0();
		return;
	}
	MLADDR->mlda = bp->b_blkno;
	MLADDR->mlba = bp->b_un.b_addr;
	if (mltab.b_flags & B_RH70)
		MLADDR->mlbae = bp->b_xmem;
	MLADDR->mlwc = -(bp->b_bcount >> 1);
	com = ((bp->b_xmem & 3) << 8) | ML_IE | ML_GO;
	if (bp->b_flags & B_READ)
		com |= ML_RCOM;
	else
		com |= ML_WCOM;
	MLADDR->mlcs1.w = com;
#ifdef	ML_DKN
	dk_busy |= 1 << (ML_DKN + NML);
	dk_numb[ML_DKN + NML] += 1;
	dk_wds[ML_DKN + NML] += bp->b_bcount >> 6;
#endif	ML_DKN
	(void) _spl0();
}

mlintr()
{
	register struct buf *bp;
	register unit;
	int as;
	int ctr;

	if (mltab.b_active == NULL)
		return;
	as = MLADDR->mlas & 0377;
	bp = mltab.b_actf;
	unit = minor(bp->b_dev) & 7;
#ifdef	ML_DKN
	dk_busy &= ~(1 << (ML_DKN + NML));
#endif
	as &= ~(1<<unit);
	MLADDR->mlas = as;
	MLADDR->mlcs2.c[0] = unit;
	if (MLADDR->mlcs1.w & ML_TRE) {		/* error bit */
		ctr = 0;
		while (((MLADDR->mlds & MLDS_DRY) == 0) && --ctr)
			;
		if (mltab.b_errcnt == 0)
#ifdef	UCB_DEVERR
			harderr (bp, "ml");
			printf ("cs2=%b er=%b\n", MLADDR->mlcs2.w,
				MLCS2_BITS, MLADDR->mler, MLER_BITS);
#else
			deverror(bp, MLADDR->mlcs2.w, MLADDR->mler);
#endif	UCB_DEVERR
		MLADDR->mlcs1.w = ML_DCLR | ML_GO;
		if (++mltab.b_errcnt <= 10) {
			mlstart();
			return;
		}
		bp->b_flags |= B_ERROR;
	}
	mltab.b_errcnt = 0;
	mltab.b_actf = bp->av_forw;
	bp->b_resid = 0;
	iodone(bp);
	mlstart();
}
#endif	NML

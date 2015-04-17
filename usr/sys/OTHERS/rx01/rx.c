/*
 * Rx disk driver
 */

#ifdef	never
static	char	SCCS_ID[]	= "@(#)rx.c	Ver. 1.2 6/83";
#endif

/*	This was taken and adapted from V7 unix
 *	The baroque ifdefs were to identify changes
 *	I made to handle UCB's Unibus Mapping routines
 *	They can be removed as soon as you're
 *	sure it works.
 */

#include "rx.h"
#if	NRX > 0
#include "param.h"
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/rxreg.h>
#include <sys/uba.h>

#define DK_N	0	/* Monitoring device number */

#define HZ	60	/* line frequency */
#define	NRXDR	4	/* Nbr logical drives per controller */
#define	RXSIZE	2002	/* Size of rx disk in sectors */
#define RXBLK	BSIZE/NRXBYTE	/* Size of block (512 bytes) in rx sectors */
#define	RXBLKTR	26	/* Nbr blocks per track */
#define	NRXBYTE	128	/* Nbr bytes in rx block */
#define	MAXERR	10	/* Nbr retries after error */
#define RXHANG	300	/* Maximum hang on a TR or DONE bit */

#define RXBUSY	1	/* Busy flag */
#define RXERROR	2	/* Error flag */
#define RXINIT	4	/* Initialize flag */

#define rxsa rxdb	/* Sector Address Register */
#define rxta rxdb	/* Track Address Register */
#define rxes rxdb	/* Error Status Register */

#define trwait() { register int n; \
    for (n = RXHANG; (rp->rxcs & TR) == 0; n -= 1) \
        if (n == 0) { dp->b_active |= RXERROR; break; } }
#define donewait() { register int n; \
    for (n = RXHANG; (rp->rxcs & DONE) == 0; n -= 1) \
        if (n == 0) { dp->b_active |= RXERROR; break; } }
#define getdb(n) { trwait (); n = rp->rxdb; }
#define putdb(n) { trwait (); rp->rxdb = n; }
#define errtst (((rp->rxcs & (ERROR|DONE)) != DONE) \
    || ((rp->rxdb & (CRC|PARITY|IDONE)) != 0) \
    || ((dp->b_active & RXERROR) != 0))
#define	exadr(x, y)	(((long) x << 16) | (unsigned) (y))

/*
 * Minor device field coding constants
 */
#define RXLDR	 03	/* Logical drive mask */
#define RXILEAVE 04	/* Interleave mask */
#define RXCTLR	 03	/* Controller shift */

extern struct rxdevice *rx_addr[];
/*
 * Rx handler work areas
 */
struct buf rxtab, rxutab[NRX];	/* rxdevice tables */

/*
 * Rx strategy
 */
rxstrategy (bp)
register struct buf *bp;
{
	register struct buf *dp;
	register int ctlr;

	ctlr = minor (bp->b_dev) >> RXCTLR;
	dp = &rxutab[ctlr];

	if(bp->b_flags & B_PHYS)
		mapalloc(bp);
	/* link buffer into i/o queue and start i/o */
	bp->b_resid = bp->b_bcount;
	bp->b_error = 0;
	spl4 ();
	bp->av_forw = NULL;
	if (dp->b_actf == NULL)
		dp->b_actf = bp;
	else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	if (!dp->b_active)
		rxstart (ctlr);
	spl0 ();
}

/*
 * Do something
 */
rxstart (ctlr)
int ctlr;
{
	register struct buf *dp, *bp;
	register struct rxdevice *rp;
	unsigned int bx;
	char *ptr;
	long blk, lptr;
	int trk, sct, wd;
	struct	ubmap *ubp;

	dp = &rxutab[ctlr];
	rp = rx_addr[ctlr];
retry:
	if ((bp = dp->b_actf) == NULL)
		return;

	dp->b_active = RXBUSY;
	if ((bp->b_flags & B_READ) == 0) {
		/* output characters to buffer */
		rp->rxcs = FILL|GO;
		lptr = exadr(bp->b_xmem, bp->b_un.b_addr);
		if(bp->b_flags & (B_MAP|B_UBAREMAP)) {
			/*
			 * Simulate UNIBUS map if UNIBUS transfer
			 * This is untested.
			 */
			ubp = UBMAP + ((lptr >> 13) & 037);
			lptr = exadr(ubp->ub_hi, ubp->ub_lo) + (lptr & 017777);
		}
		lptr += (long) (bp->b_bcount - bp->b_resid);
		for (bx = 0; bx < NRXBYTE; bx += 1) {
			wd = (bp->b_resid > bx) ? getmemc(lptr) : 0;
			putdb(wd);
			lptr++;
		}
		donewait ();
		if (errtst) {
			if (rxerror (ctlr)) goto retry;
			return;
		}
	}
/* calculate drive, block, track and sector number for i/o */
	blk = (bp->b_blkno << 2)
	    + (bp->b_bcount - bp->b_resid) / NRXBYTE;
	if ((blk >= RXSIZE) || (blk < 0)) {
		if ((blk > ((RXSIZE+RXBLK-1)/RXBLK*RXBLK))
		    || (blk < 0)
		    || ((bp->b_flags & B_READ) == 0)) {
			/* errors on writes don't actually do any good;
			*  nobody listens to them */
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
		}
		rxdone (ctlr);
		goto retry;
	}
	trk = blk / RXBLKTR;
	sct = blk % RXBLKTR;

/* if indicated, interleave sectors using DEC's RT-11 algorithm */
	if (minor (bp->b_dev) & RXILEAVE) {
		if ((sct <<= 1) >= RXBLKTR){
			sct++;
		}
		sct = (sct + 6 * trk) % RXBLKTR;
		if (++trk >= (RXSIZE / RXBLKTR)){
			trk = 0;
		}
	}
	sct += 1;

/* perform read/write */
	rp->rxcs = IENABLE
		| GO
		| (bp->b_flags & B_READ ? READ : WRITE)
		| ((minor (bp->b_dev) & RXLDR) << 4);
	putdb (sct);
	putdb (trk);
}

/*
 * Initialize a controller
 */
rxinit (ctlr)
int ctlr;
{
	register struct buf *dp;
	int rxwaker ();


	dp = &rxutab[ctlr];
	if ((dp->b_active & RXINIT) == 0) {
		rx_addr[ctlr]->rxcs = INIT;
		dp->b_active |= RXINIT;
		timeout (rxwaker, ctlr, HZ*3/2);
	}
}

/*
 * Wake up after initialize operation
 */
rxwaker (ctlr)
int ctlr;
{
	register struct buf *dp;
	register struct rxdevice *rp;


	dp = &rxutab[ctlr];
	rp = rx_addr[ctlr];
	dp->b_active &= ~RXINIT;
	if (((rp->rxcs & (DONE|ERROR)) != DONE)
	    || ((rp->rxes & IDONE) == 0)) {
		if (rxerror (ctlr)) rxstart (ctlr);
		return;
	}
	rxstart (ctlr);
}

/*
 * Restart after power fail
 */
rxrstrt (flg)
int flg;
{
	register struct buf *dp;
	register int ctlr;

	if (flg == 0) {
		for (ctlr = 0; ctlr < NRX; ctlr += 1) {
			dp = &rxutab[ctlr];
			if ((dp->b_active & RXBUSY) != 0)
				dp->b_active |= RXERROR;
		}
	}
	else {
		for (ctlr = 0; ctlr < NRX; ctlr += 1) {
			dp = &rxutab[ctlr];
			if ((dp->b_active & RXBUSY) != 0)
				if (rxerror (ctlr)) rxstart (ctlr);
		}
	}
}

/*
 * Interrupt-time stuff
 */
rxintr (ctlr)
int ctlr;
{
	register struct buf *dp, *bp;
	register struct rxdevice *rp;
	unsigned int bx;
	char *ptr, byte;
	long lptr;
	int wd;
	struct	ubmap *ubp;

	dp = &rxutab[ctlr];
	if (!dp->b_active) {
		return;
	}
	rp = rx_addr[ctlr];
	bp = dp->b_actf;
	if (errtst) {
		if (rxerror (ctlr)) rxstart (ctlr);
		return;
	}
	if (bp->b_flags & B_READ) {
		/* input data from buffer */
		rp->rxcs = EMPTY|GO;
		lptr = exadr(bp->b_xmem, bp->b_un.b_addr);
		if(bp->b_flags & (B_MAP|B_UBAREMAP)) {
			/*
			 * Simulate UNIBUS map if UNIBUS transfer
			 * This is untested.
			 */
			ubp = UBMAP + ((lptr >> 13) & 037);
			lptr = exadr(ubp->ub_hi, ubp->ub_lo) + (lptr & 017777);
		}
		lptr += (long) (bp->b_bcount - bp->b_resid);
		for (bx = 0; bx < NRXBYTE; bx += 1) {
			getdb(byte);
			putmemc(lptr, ((bp->b_resid > bx) ? byte : 0));
			lptr++;
		}
		donewait ();
		if (errtst) {
			if (rxerror (ctlr)) rxstart (ctlr);
			return;
		}
	}
	if (bp->b_resid <= NRXBYTE) {
		bp->b_resid = 0;
		rxdone (ctlr);
	}
	else {
		bp->b_resid -= NRXBYTE;
		dp->b_errcnt = 0;
	}
	rxstart (ctlr);
}

rxdone (ctlr)
int ctlr;
{
	register struct buf *dp, *bp;


	dp = &rxutab[ctlr];
	bp = dp->b_actf;
	dp->b_active = 0;
	dp->b_errcnt = 0;
	dp->b_actf = bp->av_forw;
	iodone (bp);
}

rxerror (ctlr)
int ctlr;
{
	register struct buf *dp, *bp;
	register struct rxdevice *rp;


	dp = &rxutab[ctlr];
	bp = dp->b_actf;
	rp = rx_addr[ctlr];
	dp->b_active &= ~RXERROR;
	/* quit if too many errors */
	dp->b_errcnt += 1;
	if (dp->b_errcnt > MAXERR) {
#ifdef UCB_DEVERR
		harderr (bp, "rx");
		printf("active=%d, cs=0%o", dp->b_active, rp->rxcs);
#else
		deverror (bp, dp->b_active, rp->rxcs);
#endif
		bp->b_flags |= B_ERROR;
		rxdone (ctlr);
	}
	else if (!(rp->rxcs & DONE) || !(rp->rxes & (CRC|PARITY))) {
		rxinit (ctlr);
		return (0);
	}
	return (1);
}
#endif	NRX

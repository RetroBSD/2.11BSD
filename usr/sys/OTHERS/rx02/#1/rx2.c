/*
 * RX02 floppy disk device driver
 *
 * This driver was written by Bill Shannon and distributed on the
 * DEC v7m UNIX tape.  It has been modified for 2BSD and has been
 * included with the permission of the DEC UNIX Engineering Group.
 *
 *
 *	Layout of logical devices:
 *
 *	name	min dev		unit	density
 *	----	-------		----	-------
 *	rx0	   0		  0	single
 *	rx1	   1		  1	single
 *	rx2	   2		  0	double
 *	rx3	   3		  1	double
 *
 *
 *	Stty function call may be used to format a disk.
 *	To enable this feature, define RX2_IOCTL.
 */

/*
 *	SCCS id	@(#)rx2.c	2.1 (Berkeley)	8/5/83
 */

#include "rx2.h"
#if	NRX2 > 0
#include "param.h"
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/rx2reg.h>

extern	struct	rx2device *RX2ADDR;

/*
 *	the following defines use some fundamental
 *	constants of the RX02.
 */
#define	NSPB	((minor(bp->b_dev)&2) ? 4 : 8)		/* sectors per block */
#define	NRXBLKS	((minor(bp->b_dev)&2) ? 500 : 250)	/* blocks on device */
#define	NBPS	((minor(bp->b_dev)&2) ? 256 : 128)	/* bytes per sector */
#define	DENSITY	(minor(bp->b_dev)&2)	/* Density: 0 = single, 2 = double */
#define	UNIT	(minor(bp->b_dev)&1)	/* Unit Number: 0 = left, 1 = right */

#define	rx2wait()	while (((RX2ADDR->(rx2cs)) & RX2_XREQ) == 0)
#define	b_seccnt	av_back
#define	seccnt(bp)	((int) ((bp)->b_seccnt))

struct	buf	rx2tab;
#ifdef RX2_IOCTL
struct	buf	crx2buf;	/* buffer header for control functions */
#endif

/*
 *	states of driver, kept in b_state
 */
#define	SREAD	1	/* read started  */
#define	SEMPTY	2	/* empty started */
#define	SFILL	3	/* fill started  */
#define	SWRITE	4	/* write started */
#define	SINIT	5	/* init started  */
#define	SFORMAT	6	/* format started */

/*ARGSUSED*/
rx2open(dev, flag)
dev_t	dev;
{
	if(minor(dev) >= 4)
		u.u_error = ENXIO;
}

rx2strategy(bp)
register struct buf *bp;
{
	if(bp->b_flags & B_PHYS)
		mapalloc(bp);
	if(bp->b_blkno >= NRXBLKS) {
		if(bp->b_flags&B_READ)
			bp->b_resid = bp->b_bcount;
		else {
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
		}
		iodone(bp);
		return;
	}
	bp->av_forw = (struct buf *) NULL;

	/*
	 * seccnt is actually the number of floppy sectors transferred,
	 * incremented by one after each successful transfer of a sector.
	 */
	seccnt(bp) = 0;

	/*
	 * We'll modify b_resid as each piece of the transfer
	 * successfully completes.  It will also tell us when
	 * the transfer is complete.
	 */
	bp->b_resid = bp->b_bcount;
	(void) _spl5();
	if(rx2tab.b_actf == NULL)
		rx2tab.b_actf = bp;
	else
		rx2tab.b_actl->av_forw = bp;
	rx2tab.b_actl = bp;
	if(rx2tab.b_state == NULL)
		rx2start();
	(void) _spl0();
}

rx2start()
{
	register struct buf *bp;
	int sector, track;
	char *addr, *xmem;

	if((bp = rx2tab.b_actf) == NULL) {
		rx2tab.b_state = NULL;
		return;
	}

#ifdef RX2_IOCTL
	if (bp == &crx2buf) {	/* is it a control request ? */
		rx2tab.b_state = SFORMAT;
		RX2ADDR->rx2cs = RX2_SMD | RX2_GO | RX2_IE | (UNIT << 4) | (DENSITY << 7);
		rx2wait();
		RX2ADDR->rx2db = 'I';
	} else
#endif
	if(bp->b_flags & B_READ) {
		rx2tab.b_state = SREAD;
		rx2factr((int)bp->b_blkno * NSPB + seccnt(bp), &sector, &track);
		RX2ADDR->rx2cs = RX2_RSECT | RX2_GO | RX2_IE | (UNIT << 4) | (DENSITY << 7);
		rx2wait();
		RX2ADDR->rx2sa = sector;
		rx2wait();
		RX2ADDR->rx2ta = track;
	} else {
		rx2tab.b_state = SFILL;
		rx2addr(bp, &addr, &xmem);
		RX2ADDR->rx2cs = RX2_FILL | RX2_GO | RX2_IE | (xmem << 12) | (DENSITY << 7);
		rx2wait();
		RX2ADDR->rx2wc = (bp->b_resid >= NBPS ? NBPS : bp->b_resid) >> 1;
		rx2wait();
		RX2ADDR->rx2ba = addr;
	}
}

rx2intr()
{
	register struct buf *bp;
	int sector, track;
	char *addr, *xmem;

	if (rx2tab.b_state == SINIT) {
		rx2start();
		return;
	}

	if((bp = rx2tab.b_actf) == NULL)
		return;

	if(RX2ADDR->rx2cs < 0) {
		if(rx2tab.b_errcnt++ > 10 || rx2tab.b_state == SFORMAT) {
			bp->b_flags |= B_ERROR;
#ifdef	UCB_DEVERR
			harderr(bp, "rx2");
			printf("cs=%b er=%b\n", RX2ADDR->rx2cs, RX2_BITS,
				RX2ADDR->rx2es, RX2ES_BITS);
#else
			deverror(bp, RX2ADDR->rx2cs, RX2ADDR->rx2db);
#endif	UCB_DEVERR
			rx2tab.b_errcnt = 0;
			rx2tab.b_actf = bp->av_forw;
			iodone(bp);
		}
		RX2ADDR->rx2cs = RX2_INIT;
		RX2ADDR->rx2cs = RX2_IE;
		rx2tab.b_state = SINIT;
		return;
	}
	switch (rx2tab.b_state) {

	case SREAD:			/* read done, start empty */
		rx2tab.b_state = SEMPTY;
		rx2addr(bp, &addr, &xmem);
		RX2ADDR->rx2cs = RX2_EMPTY | RX2_GO | RX2_IE | (xmem << 12) | (DENSITY << 7);
		rx2wait();
		RX2ADDR->rx2wc = (bp->b_resid >= NBPS? NBPS : bp->b_resid) >> 1;
		rx2wait();
		RX2ADDR->rx2ba = addr;
		return;

	case SFILL:			/* fill done, start write */
		rx2tab.b_state = SWRITE;
		rx2factr((int)bp->b_blkno * NSPB + seccnt(bp), &sector, &track);
		RX2ADDR->rx2cs = RX2_WSECT | RX2_GO | RX2_IE | (UNIT << 4) | (DENSITY << 7);
		rx2wait();
		RX2ADDR->rx2sa = sector;
		rx2wait();
		RX2ADDR->rx2ta = track;
		return;

	case SWRITE:			/* write done, start next fill */
	case SEMPTY:			/* empty done, start next read */
		/*
		 * increment amount remaining to be transferred.
		 * if it becomes positive, last transfer was a
		 * partial sector and we're done, so set remaining
		 * to zero.
		 */
		if (bp->b_resid <= NBPS) {
done:
			bp->b_resid = 0;
			rx2tab.b_errcnt = 0;
			rx2tab.b_actf = bp->av_forw;
			iodone(bp);
			break;
		}

		bp->b_resid -= NBPS;
		seccnt(bp)++;
		break;

#ifdef RX2_IOCTL
	case SFORMAT:			/* format done (whew!!!) */
		goto done;		/* driver's getting too big... */
#endif
	}

	/* end up here from states SWRITE and SEMPTY */
	rx2start();
}

/*
 *	rx2factr -- calculates the physical sector and physical
 *	track on the disk for a given logical sector.
 *	call:
 *		rx2factr(logical_sector,&p_sector,&p_track);
 *	the logical sector number (0 - 2001) is converted
 *	to a physical sector number (1 - 26) and a physical
 *	track number (0 - 76).
 *	the logical sectors specify physical sectors that
 *	are interleaved with a factor of 2. thus the sectors
 *	are read in the following order for increasing
 *	logical sector numbers (1,3, ... 23,25,2,4, ... 24,26)
 *	There is also a 6 sector slew between tracks.
 *	Logical sectors start at track 1, sector 1; go to
 *	track 76 and then to track 0.  Thus, for example, unix block number
 *	498 starts at track 0, sector 25 and runs thru track 0, sector 2
 *	(or 6 depending on density).
 */
rx2factr(sectr, psectr, ptrck)
register int sectr;
int *psectr, *ptrck;
{
	register int p1, p2;

	p1 = sectr / 26;
	p2 = sectr % 26;
	/* 2 to 1 interleave */
	p2 = (2 * p2 + (p2 >= 13 ?  1 : 0)) % 26;
	/* 6 sector per track slew */
	*psectr = 1 + (p2 + 6 * p1) % 26;
	if (++p1 >= 77)
		p1 = 0;
	*ptrck = p1;
}


/*
 *	rx2addr -- compute core address where next sector
 *	goes to / comes from based on bp->b_un.b_addr, bp->b_xmem,
 *	and seccnt(bp).
 */
rx2addr(bp, addr, xmem)
register struct buf *bp;
register char **addr, **xmem;
{
	*addr = bp->b_un.b_addr + seccnt(bp) * NBPS;
	*xmem = bp->b_xmem;
	if (*addr < bp->b_un.b_addr)		/* overflow, bump xmem */
		(*xmem)++;
}

#ifdef RX2_IOCTL
/*
 *	rx2sgtty -- format RX02 disk, single or double density.
 *	stty with word 0 == 010 does format.  density determined
 *	by device opened.
 */
/*ARGSUSED*/
rx2ioctl(dev, cmd, addr, flag)
dev_t	dev;
{
	register s;
	register struct buf *bp;
	struct rx2iocb {
		int	ioc_cmd;	/* command */
		int	ioc_res1;	/* reserved */
		int	ioc_res2;	/* reserved */
	} iocb;

	if (cmd != TIOCSETP) {
err:
		u.u_error = ENXIO;
		return(0);
	}
	if (copyin(addr, (caddr_t)&iocb, sizeof (iocb))) {
		u.u_error = EFAULT;
		return(1);
	}
	if (iocb.ioc_cmd != RX2_SMD)
		goto err;
	bp = &crx2buf;
	while (bp->b_flags & B_BUSY) {
		s = spl6();
		bp->b_flags |= B_WANTED;
		sleep(bp, PRIBIO);
	}
	splx(s);
	bp->b_flags = B_BUSY;
	bp->b_dev = dev;
	bp->b_error = 0;
	rx2strategy(bp);
	iowait(bp);
	bp->b_flags = 0;
}
#endif	RX2_IOCTL
#endif	NRX2

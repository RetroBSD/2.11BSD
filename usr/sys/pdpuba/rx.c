/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)rx.c	1.4 (2.11BSD GTE) 1995/11/27
 */

/*
 * RX02 floppy disk device driver
 *
 * sms - November 26, 1995.
 * Actually got it working with a 18 bit controller on a 22 bit Qbus.
 *
 * sms - November 21, 1995.  
 * Moved from OTHERS/rx02/#2 into the supported directory: sys/pdpuba.
 * Added conditionalized support for a "software unibus/qbus map" so that
 * 18 bit controllers could be supported in 22 bit Qbus systems.
 *
 * Date: Sun, 8 May 88 18:42:38 CDT
 * uunet!nuchat!steve@rutgers.edu (Steve Nuchia)
 * The rx02 driver as distributed didn't even come close to working
 * on a Q22 machine - probe was wrong and it got worse from there.
 *
 * This driver was written by Bill Shannon and distributed on the
 * DEC v7m UNIX tape.  It has been modified for 2BSD and has been
 * included with the permission of the DEC UNIX Engineering Group.
 *
 * Modified to actually work with 2.9BSD by Gregory Travis, Indiana Univ.
 *
 *	Layout of logical devices:
 *
 *	name	min dev		unit	density
 *	----	-------		----	-------
 *	rx0a	   0		  0	single
 *	rx1a	   1		  1	single
 *	rx0b	   2		  0	double
 *	rx1b	   3		  1	double
 *
 *	ioctl function call may be used to format a disk.
 */

#include "rx.h"
#if NRX > 0
#include "param.h"
#include "buf.h"
#include "conf.h"
#include "ioctl.h"
#include "tty.h"
#include "rxreg.h"
#include "errno.h"
#include "map.h"
#include "uba.h"

struct	rxdevice *RXADDR;

/*
 *	the following defines use some fundamental
 *	constants of the RX02.
 */
#define	NSPB	((minor(bp->b_dev)&2) ? 2 : 4)		/* sectors per block */
#define	NRXBLKS	((minor(bp->b_dev)&2) ? 1001 : 500)	/* blocks on device */
#define	NBPS	((minor(bp->b_dev)&2) ? 256 : 128)	/* bytes per sector */
#define	DENSITY	(minor(bp->b_dev)&2)	/* Density: 0 = single, 2 = double */
#define	UNIT	(minor(bp->b_dev)&1)	/* Unit Number: 0 = left, 1 = right */
#define	RXGID	(RX_GO | RX_IE | (DENSITY << 7))

#define	rxwait()	while (((RXADDR->rxcs) & RX_XREQ) == 0)
#define	seccnt(bp)	((int)((bp)->b_seccnt))

struct	buf	rxtab;
struct	buf	crxbuf;		/* buffer header for control functions */

/*
 * DEC controllers do not do 22 bit DMA but 3rd party (Sigma MXV-22) can.
 * 'rxsoftmap' can be patched (via 'adb') as indicated below to inhibit
 * the probing (checking bit 10 in the CSR) for 22 bit controllers.
*/

static	char	mxv22;		/* MXV22 can do native 22 bit DMA */
static	char	rxsoftmap = -1;	/* -1 = OK to check for soft map
				 *  0 = Never use soft map
				 *  1 = Always use soft map
				*/

/*
 *	states of driver, kept in b_state
 */
#define	SREAD	1	/* read started */
#define	SEMPTY	2	/* empty started */
#define	SFILL	3	/* fill started */
#define	SWRITE	4	/* write started */
#define	SINIT	5	/* init started */
#define	SFORMAT	6	/* format started */

rxattach(addr, unit)
	struct rxdevice *addr;
	u_int unit;
	{

	if	(unit != 0)
		return(0);
	RXADDR = addr;
	if	(addr->rxcs & RX_Q22)	/* 22 bit capable? */
		mxv22 = 1;
/*
 * If it is not a 22 bit controller and there is no Unibus map and
 * it is permitted to switch to a soft map then set the "use soft map" flag.
*/
	if	(!mxv22 && !ubmap && rxsoftmap == -1)
		rxsoftmap = 1;
	return(1);
	}

/*ARGSUSED*/
rxopen(dev, flag)
	dev_t dev;
{
	if (minor(dev) >= 4 || !RXADDR)
		return (ENXIO);
	return (0);
}

rxstrategy(bp)
	register struct buf *bp;
{
	register int s;

	if (minor(bp->b_dev) >= 4 || !RXADDR)
		goto bad;
	if (bp->b_blkno >= NRXBLKS) {
		if (bp->b_flags&B_READ)
			bp->b_resid = bp->b_bcount;
		else {
bad:			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
		}
		iodone(bp);
		return;
	}

#ifdef	SOFUB_MAP
	if	(rxsoftmap == 1)
		{
		if	(sofub_alloc(bp) == 0)
			return;
		}
	else
#endif
		mapalloc(bp);

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
	s = splbio();
	if (rxtab.b_actf == NULL)
		rxtab.b_actf = bp;
	else
		rxtab.b_actl->av_forw = bp;
	rxtab.b_actl = bp;
	if (rxtab.b_state == NULL)
		rxstart();
	splx(s);
}

rxstart()
{
	register struct buf *bp;
	int addr, xmem, cmd;
	int n, sector, track;

	if ((bp = rxtab.b_actf) == NULL) {
		rxtab.b_state = NULL;
		return;
	}

	if (bp == &crxbuf) {		/* is it a control request ? */
		rxtab.b_state = SFORMAT;
		RXADDR->rxcs = RX_SMD | RXGID | (UNIT << 4);
		rxwait();
		RXADDR->rxdb = 0111;
	} else
	if (bp->b_flags & B_READ) {
		rxtab.b_state = SREAD;
		rxfactr((int)bp->b_blkno * NSPB + seccnt(bp), &sector, &track);
		RXADDR->rxcs = RX_RSECT | RXGID | (UNIT << 4);
		rxwait();
		RXADDR->rxsa = sector;
		rxwait();
		RXADDR->rxta = track;
	} else {
		rxtab.b_state = SFILL;
		n = bp->b_resid >= NBPS ? NBPS : bp->b_resid;
		rxaddr ( bp, &addr, &xmem );
		if	(rxsoftmap <= 0)
			cmd = RX_Q22;
		else
			cmd = 0;
		RXADDR->rxcs = RX_FILL | RXGID | ((xmem & 3) << 12) | cmd;
		rxwait();
		RXADDR->rxwc = n >> 1;
		rxwait();
		RXADDR->rxba = addr;
		if	(rxsoftmap <= 0)
			{
			rxwait();
			RXADDR->rxba = xmem;
			}
	}
}

rxintr()
{
	register struct buf *bp;
	int n, sector, track, cmd;
static	rxerr[4];
	char	*decode;
	int addr, xmem;

	if (rxtab.b_state == SINIT) {
		rxstart();
		return;
	}

	if ((bp = rxtab.b_actf) == NULL)
		return;

	if (RXADDR->rxcs & RX_ERR) {
		if (rxtab.b_errcnt++ > 10 || rxtab.b_state == SFORMAT) {
			bp->b_flags |= B_ERROR;
			harderr(bp, "rx");
			printf("cs=%b er=%b\n", RXADDR->rxcs, RX_BITS,
				RXADDR->rxes, RXES_BITS);
			RXADDR->rxcs = RX_RDEC | RX_GO;
			rxwait();
			RXADDR->rxba = (short) rxerr;
			while ( ! (RXADDR->rxcs & RX_DONE) );
			switch ( rxerr[0] )
			{
			case 0040: decode = "bad track"; break;
			case 0050: decode = "found home"; break;
			case 0070: decode = "no sch sctr"; break;
			case 0120: decode = "no preamble"; break;
			case 0150: decode = "ozone headers"; break;
			case 0160: decode = "too many IDAM"; break;
			case 0170: decode = "data AM missing"; break;
			case 0200: decode = "CRC error"; break;
			case 0240: decode = "density error"; break;
			case 0250: decode = "bad fmt key"; break;
			case 0260: decode = "bad data AM"; break;
			case 0270: decode = "POK while write"; break;
			case 0300: decode = "drv not ready"; break;
			case 0310: decode = "write protected"; break;
			default: decode = "unknown error"; break;
			}
			printf("rx: err %o=%s\n", rxerr[0], decode );
			rxtab.b_errcnt = 0;
			rxtab.b_actf = bp->av_forw;
#ifdef	SOFUB_MAP
			if	(rxsoftmap == 1)
				sofub_relse(bp, bp->b_bcount);
#endif
			iodone(bp);
		}
		RXADDR->rxcs = RX_INIT;
		RXADDR->rxcs = RX_IE;
		rxtab.b_state = SINIT;
		return;
	}
	switch (rxtab.b_state) {

	case SREAD:			/* read done, start empty */
		rxtab.b_state = SEMPTY;
		n = bp->b_resid >= NBPS? NBPS : bp->b_resid;
		rxaddr ( bp, &addr, &xmem );
		if	(rxsoftmap <= 0)
			cmd = RX_Q22;
		else
			cmd = 0;
		RXADDR->rxcs = RX_EMPTY | RXGID | ((xmem & 3) << 12) | cmd;
		rxwait();
		RXADDR->rxwc = n >> 1;
		rxwait();
		RXADDR->rxba = addr;
		if	(rxsoftmap <= 0)
			{
			rxwait();
			RXADDR->rxba = xmem;
			}
		return;

	case SFILL:			/* fill done, start write */
		rxtab.b_state = SWRITE;
		rxfactr((int)bp->b_blkno * NSPB + seccnt(bp), &sector, &track);
		RXADDR->rxcs = RX_WSECT | RXGID | (UNIT << 4);
		rxwait();
		RXADDR->rxsa = sector;
		rxwait();
		RXADDR->rxta = track;
		return;

	case SEMPTY:			/* empty done, start next read */
	case SWRITE:			/* write done, start next fill */
		/*
		 * increment amount remaining to be transferred.
		 * if it becomes positive, last transfer was a
		 * partial sector and we're done, so set remaining
		 * to zero.
		 */
		if (bp->b_resid <= NBPS) {
done:
			bp->b_resid = 0;
			rxtab.b_errcnt = 0;
			rxtab.b_actf = bp->av_forw;
#ifdef	SOFUB_MAP
			if	(rxsoftmap == 1)
				sofub_relse(bp, bp->b_bcount);
#endif
			iodone(bp);
			break;
		}

		bp->b_resid -= NBPS;
		seccnt(bp)++;
		break;

	case SFORMAT:			/* format done (whew!!!) */
		goto done;		/* driver's getting too big... */
	}

	/* end up here from states SWRITE and SEMPTY */
	rxstart();
}

/*
 *	rxfactr -- calculates the physical sector and physical
 *	track on the disk for a given logical sector.
 *	call:
 *		rxfactr(logical_sector,&p_sector,&p_track);
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
static
rxfactr(sectr, psectr, ptrck)
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
 *	rxaddr -- compute core address where next sector
 *	goes to / comes from based on bp->b_un.b_addr, bp->b_xmem,
 *	and seccnt(bp).
 */
static
rxaddr(bp, addr, xmem)
	register struct buf *bp;
	register u_int *addr, *xmem;
{
	*addr = (u_int)bp->b_un.b_addr + (seccnt(bp) * NBPS);
	*xmem = bp->b_xmem;
	if (*addr < (u_int)bp->b_un.b_addr)	/* overflow, bump xmem */
		(*xmem)++;
}

/*
 *	rxioctl -- format RX02 disk, single or double density.
 *	density determined by device opened.
 */
/*ARGSUSED*/
rxioctl(dev, cmd, addr, flag)
	dev_t dev;
	u_int cmd;
{
	register int s;
	register struct buf *bp;

	if (cmd != RXIOC_FORMAT)
		return (ENXIO);
	bp = &crxbuf;
	while (bp->b_flags & B_BUSY) {
		s = splbio();
		bp->b_flags |= B_WANTED;
		sleep(bp, PRIBIO);
	}
	splx(s);
	bp->b_flags = B_BUSY;
	bp->b_dev = dev;
	bp->b_error = 0;
	rxstrategy(bp);
	iowait(bp);
	bp->b_flags = 0;
	return (0);
}
#endif

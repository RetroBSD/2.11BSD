/*
 * RX11 floppy disk driver
 * Modified 2/1/78, Brad Blasing, to emulate DMA operations
 *
 * Modified 15 DEC 77 by SSB,DWD,BEB, University of Minnesota 
 * for RX11 floppy drive.
 * Modified 6/16/78, Brad Blasing, to provide better error recovery
 *  (we don't loop at spl5 waiting for INIT to finish any more).
 * Modified 10/19/79, Scott Bertilson, to run with slow floppy interfaces.
 * Modified 4/5/80, Scott Bertilson, added CP/M interleaving to table.
 * Modified 2/1/81, Scott Bertilson, to run on Version 7.
 * Modified at Tek to "sort of" run under 2.9bsd; 5/83; dgc
 */

#include "rx.h"
#if	NRX > 0
#include "param.h"
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/rxreg.h>
extern	struct rxdevice *RXADDR;

/* #define DEBUG	1 */
#define	DMA	1
/* #define RXADDR	((struct rxdevice *)0177170)	/* MAE - 3/29/83 */
#define NRXTYP	4
#define MAXRETRY  5
#define TTIME	60	/* Timeout time in HZ */
#define RRATE	6	/* Recall rate for rxreset */
#define RESETMAX 10	/* Max. num. of reset recalls before timeout */
			/* RESETMAX*RRATE/60 = time in second */

#define RXWAIT	while((rxaddr->rxcs & (TR | DONE)) == 0) ;
#define KISA	0172340
#define KISD	0172300

struct	rxtype {
	int	secsize;		/* size (bytes) one sector */
	int	secpertrk;		/* sectors/track */
	int	secperblk;		/* sectors/unix block */
	int	numsec;			/* total sectors on device */
	int	numblks;		/* number of blocks on device */
	int	secincr;		/* increment to get next sector of block */
	int	intrlv;			/* interleaving factor */
	int	skew;			/* sector skew across tracks */
	int	trkoffset;		/* offset num of 1st sec */
} rxtypes[NRXTYP] {
		128, 26, 4, 77*26, 500, 2, 13, 6, 0,	/* our "standard" format */
		128, 26, 4, 77*26, 500, 1, 26, 0, 0,	/* IBM format */
		128, 26, 4, 76*26, 494, 2, 13, 6, 1,	/* Terak or RT11 format */
		128, 26, 4, 76*26, 494, 6, 13, 0, 2	/* CP/M format */
};

struct	rxstat {
	int	fminor;			/* present request device number */
	struct	rxtype *ftype;		/* present request floppy type */
	int	bytect;			/* remaining bytes (neg) */
	int	sector;			/* absolute sector (0..numsec-1) */
	int	toutact;		/* timeout active */
	int	reqnum;			/* floppy request number for timeout */
	caddr_t	coreaddr;		/* current core address for transfer */
#ifdef DMA
	char	*coreblk;		/* block no. to put in seg. register */
#endif
} rxstat;

struct	buf	rxtab;

rxstrategy(abp)
struct buf *abp;
{
	register struct buf *bp;
	extern int rxtimeout();

#ifdef DEBUG
	if(minor(abp->b_dev) == 127) {
		rxdebug();
		iodone(abp);
		spl0();
		return;
	}
#endif
	bp = abp;
	/*
	 * test for valid request
	 */
	if(rxok(bp) == 0) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	/*
	 * link buffer into device queue
	 */
	bp->av_forw = NULL;
	spl5();
	if(rxtab.b_actf == NULL)
		rxtab.b_actf = bp;
	else
		rxtab.b_actl->av_forw = bp;
	rxtab.b_actl = bp;
	/*
	 * start rxtimeout if inactive
	 */
	if(rxstat.toutact == 0) {
		rxstat.toutact++;
		timeout(rxtimeout, (caddr_t)0, TTIME);
	}
	/*
	 * start device if there is no current request
	 */
	if(rxtab.b_active == NULL)
		rxstart();
	spl0();
}

rxstart()
{
	register struct buf *bp;
	register struct rxdevice *rxaddr;
	register int dminor;

	rxaddr = RXADDR;
	/*
	 * if there is no request in queue...return
	 */
loop:	if((bp = rxtab.b_actf) == NULL)
		return;
	/*
	 * check if drive ready
	 */
	dminor = (minor(bp->b_dev) & 1) << 4;
	rxaddr->rxcs = dminor | RDSTAT | GO;
	RXWAIT
	if((rxaddr->rxdb & DR) == 0) {
		printf("rx%d: Floppy not ready\n", minor(bp->b_dev));
		rxabtbuf();
		goto loop;
	}
	/*
	 * set active request flag
	 */
	rxtab.b_active++;
	rxsetup(bp);
	rxregs(bp);
}

rxintr()
{
	register struct buf *bp;
	register struct rxtype *rxt;
	register struct rxdevice *rxaddr;

	rxaddr = RXADDR;
	/*
	 * if there is no active request, false alarm.
	 */
	if(rxtab.b_active == NULL) 
		return;
	rxtab.b_active = NULL;
	/*
	 * pointer to the buffer
	 */
	bp = rxtab.b_actf;
	/*
	 * pointer to a data structure describing
	 *  the type of device (i.e. interleaving)
	 */
	rxt = rxstat.ftype;
	/*
	 * check error bit
	 */
	if(rxaddr->rxcs & ERROR) {
		/*
		 * send read error register command
		 */
		rxaddr->rxcs = RDERR | GO;
		RXWAIT
#ifndef	UCB_DEVERR
		deverror(bp, rxaddr->rxcs, rxaddr->rxdb);
#else
		harderr(bp, "rx");
		printf("cs=%b, es=%b\n", rxaddr->rxcs, RXCS_BITS,
			rxaddr->rxdb, RXES_BITS);
#endif
		/*
		 * make MAXRETRY retries on an error
		 */
		if(++rxtab.b_errcnt <= MAXRETRY) {
			rxreset(0);
			return;
		}
		/*
		 * return an i/o error
		 */
		bp->b_flags |= B_ERROR;
	} else {
		/*
		 * if we just read a sector, we need to
		 *  empty the device buffer
		 */
		if(bp->b_flags & B_READ) 
			rxempty(bp);
		/*
		 * see if there is more data to read for
		 * this request.
		 */
		rxstat.bytect += rxt->secsize;
		rxstat.sector++;
		if(rxstat.bytect < 0 && rxstat.sector < rxt->numsec) {
			rxtab.b_active++;
			rxregs(bp);
			return;
		}
	}
	rxtab.b_errcnt = 0;
	/*
	 * unlink block from queue
	 */
	rxtab.b_actf = bp->av_forw;
	iodone(bp);
	/*
	 * start i/o on next buffer in queue
	 */
	rxstart();
}

rxreset(flag)
{
	register struct rxdevice *rxaddr;

	rxaddr = RXADDR;
	/*
	 * Check to see if this is a call from rxintr or
	 * a recall from timeout.
	 */
	if(flag) {
		if(rxaddr->rxcs & DONE) {
			rxtab.b_active = 0;
			rxstart();
		} else
			if(flag > RESETMAX) {
				printf("rx%d: Reset timeout\n", minor(rxtab.b_actf->b_dev));
				rxabtbuf();
				rxstart();
			} else {
				timeout(rxreset, (caddr_t)flag+1, RRATE);
				/*
				 * Keep rxtimeout from timing out.
				 */
				rxstat.reqnum++;
			}
	} else {
		rxaddr->rxcs = INIT;
		rxtab.b_active++;
		rxstat.reqnum++;
		timeout(rxreset, (caddr_t)1, 1);
	}
}

rxregs(abp)
struct buf *abp;
{
	register struct buf *bp;
	register struct rxtype *rxt;
	register struct rxdevice *rxaddr;
	int	dminor, cursec, curtrk;

	rxaddr = RXADDR;
	/*
	 * set device bit into proper position for command
	 */
	dminor = rxstat.fminor << 4;
	bp = abp;
	rxt = rxstat.ftype;
	/*
	 * increment interrupt request number
	 */
	rxstat.reqnum++;
	/*
	 * if command is read, initiate the command
	 */
	if(bp->b_flags & B_READ){
		RXWAIT
		rxaddr->rxcs = dminor | IENABLE | GO | READ;
	} else {
		/*
		 * if command is write, fill the device buffer,
		 *   then initiate the write
		 */
		rxfill(bp);
		RXWAIT
		rxaddr->rxcs = dminor | IENABLE | GO | WRITE;
	}
	/*
	 * set track number
	 */
	curtrk = rxstat.sector / rxt->secpertrk;
	/*
	 * set sector number
	 */
	dminor = rxstat.sector % rxt->secpertrk;
	cursec = (dminor % rxt->intrlv) * rxt->secincr +
		(dminor / rxt->intrlv);
	/*
	 * add skew to sector
	 */
	cursec = (cursec + curtrk * rxt->skew)
		% rxt->secpertrk;
	/*
	 * massage registers
	 */
	RXWAIT
	rxaddr->rxdb = cursec + 1;
	RXWAIT
	rxaddr->rxdb = curtrk + rxt->trkoffset;
}

rxok(abp)
struct buf *abp;
{
	register struct buf *bp;
	register int type;
	register int dminor;

	/*
	 * get sub-device number and type from dminor device number
	 */
	dminor = minor((bp = abp)->b_dev);
	type = dminor >> 3;
	/*
	 * check for valid type
	 *
	 * check for block number within range of device
	 */
	if(type >= NRXTYP ||
		bp->b_blkno >= (daddr_t)rxtypes[type].numblks)
		return(0);
#ifndef DMA
	if(bp->b_xmem != 0) {		/* No buffers outside kernel space */
		prdev("Buffer outside kernel space", bp->b_dev);
		return(0);
	}
#endif
	return(1);
}

rxsetup(abp)
struct buf *abp;
{
	register struct buf *bp;
	register int dminor;
	register struct rxtype *rxt;

	/*
	 * get dminor device number from buffer
	 */
	dminor = minor((bp = abp)->b_dev);
	/*
	 * get sub-device number from dminor device number
	 */
	rxt = rxstat.ftype = &rxtypes[dminor >> 3];
	/*
	 * make sure device number is only 0 or 1
	 */
	rxstat.fminor = dminor & 1;
	/*
	 * get byte count to read from buffer (negative number)
	 */
	rxstat.bytect = -bp->b_bcount;
	/*
	 * transform block number into the first
	 * sector to read on the floppy
	 */
	rxstat.sector = (int)bp->b_blkno * rxt->secperblk;
	/*
	 * set the core address to get or put bytes.
	 */
#ifndef DMA
	rxstat.coreaddr = bp->b_un.b_addr;
#endif
#ifdef DMA
	rxstat.coreaddr = (bp->b_un.b_addr & 077) + 0120000;
	rxstat.coreblk = ((bp->b_un.b_addr >> 6) & 01777) |
		((bp->b_xmem & 03) << 10);
#endif
}

#ifndef DMA
rxempty(abp)
struct buf *abp;
{
	register struct rxdevice *rxaddr;
	register int i;
	register char *cp;
	int wc;

	rxaddr = RXADDR;
	/*
	 * start empty buffer command
	 */
	RXWAIT
	rxaddr->rxcs = EMPTY | GO;
	spl1();
	/*
	 * get core address and byte count
	 */
	cp = rxstat.coreaddr;
	rxstat.coreaddr += 128;
	wc = ((rxstat.bytect <= -128)? 128 : -rxstat.bytect);
	/*
	 * move wc bytes from the device buffer
	 *   into the in core buffer
	 */
	for(i=wc; i>0; --i) {
		RXWAIT
		*cp++ = rxaddr->rxdb;
	}
	/*
	 * sluff excess bytes
	 */
	for(i=128-wc; i>0; --i) {
		RXWAIT
		cp = rxaddr->rxdb;
	}
	spl5();
}

rxfill(abp)
struct buf *abp;
{
	register struct rxdevice *rxaddr;
	register int i;
	register char *cp;
	int wc;

	rxaddr = RXADDR;
	/*
	 * initiate the fill buffer command
	 */
	RXWAIT
	rxaddr->rxcs = FILL | GO;
	spl1();
	/*
	 * get core address and byte count
	 */
	cp = rxstat.coreaddr;
	rxstat.coreaddr += 128;
	wc = ((rxstat.bytect <= -128)? 128 : -rxstat.bytect);
	/*
	 * move wc bytes from the in-core buffer to
	 *   the device buffer
	 */
	for(i=wc;  i>0; --i) {
		RXWAIT
		rxaddr->rxdb = *cp++;
	}
	/*
	 * sluff excess bytes
	 */
	for(i=128-wc; i>0; --i) {
		RXWAIT
		rxaddr->rxdb = 0;
	}
	spl5();
}
#endif

#ifdef DMA
	/*
	 * This copy of the fill and empty routines emulate a dma
	 * floppy controller.  It adds the feature of being able
	 * to write anywhere in physical memory, just like an rk
	 * disk.  To do this, we borrow a segmentation register
	 * to do the transfer.  While the segmentation register
	 * is pointing to the proper place, we need to run at spl7.
	 * This is harder on the system, so the non-dma driver should
	 * be used if you only intend to do buffer requests (i.e.
	 * no swapping or raw i/o).
	 */

struct { int r[]; };

rxempty(abp)
struct buf *abp;
{
	register int i;
	register char *cp;
	register int wc;
	int a,d;
	struct rxdevice *rxaddr;

	rxaddr = RXADDR;
	/*
	 * start empty buffer command
	 */
	rxaddr->rxcs = EMPTY | GO;
	/*
	 * get core address and byte count
	 */
	cp = rxstat.coreaddr;
	wc = ((rxstat.bytect <= -128)? 128 : -rxstat.bytect);
	/*
	 * save and set segmentation register.
	 */
	a = KISA->r[5];
	d = KISD->r[5];
	spl7();
	KISA->r[5] = rxstat.coreblk;
	KISD->r[5] = 01006;
	/*
	 * move wc bytes from the device buffer
	 *   into the in core buffer
	 */
	for(i=wc; i>0; --i) {
		RXWAIT
		*cp++ = rxaddr->rxdb;
	}
	/*
	 * sluff excess bytes
	 */
	for(i=128-wc; i>0; --i) {
		RXWAIT
		cp = rxaddr->rxdb;
	}
	KISA->r[5] = a;
	KISD->r[5] = d;
	spl5();
	rxstat.coreblk += 2;
}

rxfill(abp)
struct buf *abp;
{
	register int i;
	register char *cp;
	register int wc;
	int a,d;
	struct rxdevice *rxaddr;

	rxaddr = RXADDR;
	/*
	 * initiate the fill buffer command
	 */
	rxaddr->rxcs = FILL | GO;
	/*
	 * get core address and byte count
	 */
	cp = rxstat.coreaddr;
	wc = ((rxstat.bytect <= -128)? 128 : -rxstat.bytect);
	/*
	 * save and set segmentation register.
	 */
	a = KISA->r[5];
	d = KISD->r[5];
	spl7();
	KISA->r[5] = rxstat.coreblk;
	KISD->r[5] = 01006;
	/*
	 * move wc bytes from the in-core buffer to
	 *   the device buffer
	 */
	for(i=wc;  i>0; --i) {
		RXWAIT
		rxaddr->rxdb = *cp++;
	}
	/*
	 * sluff excess bytes
	 */
	for(i=128-wc; i>0; --i) {
		RXWAIT
		rxaddr->rxdb = 0;
	}
	KISA->r[5] = a;
	KISD->r[5] = d;
	spl5();
	rxstat.coreblk += 2;
}
#endif

rxtimeout(dummy)
{
	static int prevreq;
	register struct buf *bp;

	bp = rxtab.b_actf;
	/*
	 * if the queue isn't empty and the current request number is the
	 * same as last time, abort the buffer and restart i/o.
	 */
	if(bp) {
		if(prevreq == rxstat.reqnum) {
			printf("rx%d: Floppy timeout\n", minor(bp->b_dev));
			rxabtbuf();
			rxstart();
		}
		prevreq = rxstat.reqnum;
		timeout(rxtimeout, (caddr_t)0, TTIME);
	} else {
		/*
		 * if queue is empty, just quit and rxstrategy will
		 * restart us.
		 */
		rxstat.toutact = 0;
	}
}

rxabtbuf()
{
	register struct buf *bp;

	/*
	 * abort the current buffer with an error and unlink it.
	 */
	bp = rxtab.b_actf;
	bp->b_flags |= B_ERROR;
	rxtab.b_actf = bp->av_forw;
	rxtab.b_errcnt = 0;
	rxtab.b_active = NULL;
	iodone(bp);
}

#ifdef DEBUG
rxdebug() {
	register struct buf *bp;

	spl5();
	printf("Debug:  &rxtab=%o, &rxstat=%o\n", &rxtab, &rxstat);
	printf(" rxstat:  fminor=%l, bytect=%l, sec=%l\n",
		rxstat.fminor, -rxstat.bytect, rxstat.sector);
	printf("   reqnum=%l\n", rxstat.reqnum);
	printf(" rxtab:  d_active=%l, buffers:\n", rxtab.b_active);
	for(bp=rxtab.b_actf; bp; bp=bp->av_forw)
		printf(" dev=%l/%l, blkno=%l, bcnt=%l, flags=%o.\n", major(bp->b_dev),
			minor(bp->b_dev), bp->b_blkno, -bp->b_bcount, bp->b_flags);
	putchar('\n');
}
#endif

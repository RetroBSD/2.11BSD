/*
 *	xe.c
 *
 *	version 3
 *
 *	xebec winchester controller
 *
 */

# include	"xe.h"
# if	NXE > 0
# include	<sys/param.h>
# include	<sys/systm.h>
# include	<sys/buf.h>
# include	<sys/conf.h>
# include	<sys/dir.h>
# include	<sys/user.h>
# include	<sys/seg.h>
# include	<sys/xereg.h>
# include	<sys/inline.h>
# include	<sys/uba.h>

/*
 *	kernal data PAR base
 */
#ifdef	KERN_NONSEP
# define	KDSA0	((u_short *) 0172340)
#else
# define	KDSA0	((u_short *) 0172360)
#endif

bool_t	xe_alive = 1;
struct xedevice *XEADDR = (struct xedevice *) 0177460;

struct buf	xetab;
struct xecommand xec;
u_short	xecaddr[2];
struct xeinit	xei[NXE];
u_short xeiaddr[NXE][2];
int	xeicur = -1;

/*
 *	local struct buf for partial read requests
 */
struct buf	parbuf;
/*
 *	actual data buffer for partial reads
 */
char	pardat[512 + 64];
/*
 *	physical address of pardat
 *	computed when needed
 */
u_short	paraddr[2];
u_short	parclick;

/*
 *	device parameters
 */

/*	17,	153,	6,	128,	153,	2,	0,	/* t603s */

/*	nsect	ntrac	nhead	rwcur	wpc	seek	isset */
struct xep	xeparam [NXE] = {
	17,	306,	4,	306,	128,	7,	0,	/* hh612 */
	17,	829,	3,	829,	829,	7,	0,	/* micropolis */
};

/*
 *	size of logical devices
 */
/*	size	start */
struct xesize	xe_sizes[NXE][8] = {
{	6144,	0,		/* 0 */
	2048,	6144,		/* 1 */
	12616,	8192,		/* 2 */
	0,	0,		/* 3 */
	0,	0,		/* 4 */
	0,	0,		/* 5 */
	0,	0,		/* 6 */
	0,	0,		/* 7 */
},
#ifdef	T603S
{
	2048,	0,		/* 1 */
	4096,	2048,		/* 2 */
	9462,	6144,		/* 3 */
	0,	0,		/* 4 */
	0,	0,		/* 5 */
	0,	0,		/* 6 */
	0,	0,		/* 7 */
	0,	0,		/* 8 */
},
#else
{
	4096,	0,		/* 1 */
	8192,	4092,		/* 2 */
	29821,	12288,		/* 3 reserved 10 tracks for bad tracks */
	0,	0,		/* 4 */
	0,	0,		/* 5 */
	0,	0,		/* 6 */
	0,	0,		/* 7 */
	0,	0,		/* 8 */
},
};

xestrategy (bp)
register struct buf	*bp;
{
	register struct buf	*dp;
	register int	 	unit;
	int			device;
	daddr_t			bn;
	int			s;

	unit = minor (bp->b_dev) & 077;
	device = unit >> 3;

	unit = unit & 07;
	if (device >= NXE || !xe_alive || bp->b_blkno < 0) {
		printf ("error on xe device: %d unit: %d bp->b_blkno: %D\n",
			device, unit, bp->b_blkno);
		bp->b_flags |= B_ERROR;
		iodone (bp);
		return;
	}
	bn = bp->b_blkno;
	if (!xecaddr[0]) {
		/*
		 *	map the xec address to physical bus addr
		 */
		map(&xec, xecaddr);
		/*
		 *	map the partial data block address
		 */
		map((pardat + 64) & ~(077), paraddr);
		parclick = ((paraddr[0] >> 6) & 01777) | (paraddr[1] << 10);
	}
	if (bn + (((bp->b_bcount + 511) >> 9) & 0177) >
			xe_sizes[device][unit].nblocks) {
		printf ("error reading past end of disk, device: %d unit: %d block: %D\n",
			device, unit, bn);
		bp->b_flags |= B_ERROR;
		iodone (bp);
		return;
	}
	/*
	 *	detect reads requiring partial block transfers and
	 *	use the local buffer to read the last block first.
	 */
	if ((bp->b_bcount & 511) && (bp->b_flags & B_READ)) {
		xepartial (bp);
		if (bp->b_flags & B_ERROR)
			return;
		/*
		 *	if the block was only a partial transfer,
		 *	we are done.
		 */
		if (!(bp->b_bcount & ~511)) {
			bp->b_resid = 0;
			iodone (bp);
			return;
		}
	}
	bp->b_cylin = (bn + xe_sizes[device][unit].blkoff) /
			(xeparam[device].xe_nsect * xeparam[device].xe_nhead);
	dp = &xetab;
	s = spl5();
	disksort (dp, bp);
	if (dp->b_active == 0)
		xestart ();
	splx(s);
}

xepartial (bp)
register struct buf	*bp;
{
	int	s;
	unsigned	toc, count;

	/*
	 *	make sure the transfer will work
	 */
	if ((bp->b_bcount & 077) || (bp->b_un.b_addr & 077)) {
		printf ("bad count or addr on partial read: 0%o, 0%o\n",
			bp->b_bcount, bp->b_un.b_addr);
		bp->b_flags |= B_ERROR;
		iodone (bp);
		return;
	}
	/*
	 *	get parbuf
	 */
	s = spl6();
	while (parbuf.b_flags&B_BUSY) {
		parbuf.b_flags |= B_WANTED;
		sleep((caddr_t)&parbuf, PSWP+1);
	}
	parbuf.b_flags = B_READ | B_BUSY;
	splx(s);
	/*
	 *	fill in the parbuf fields
	 */
	parbuf.b_un.b_addr = paraddr[0];
	parbuf.b_xmem = paraddr[1];
	parbuf.b_blkno = bp->b_blkno + ((bp->b_bcount >> 9) & 0177);
	parbuf.b_bcount = 512;
	parbuf.b_dev = bp->b_dev;
	parbuf.b_error = 0;
	/*
	 *	read the partial portion
	 */
	xestrategy (&parbuf);
	/*
	 *	wait for it to finish
	 */
	s = spl6 ();
	while ((parbuf.b_flags & B_DONE) == 0)
		sleep ((caddr_t) &parbuf, PSWP);
	splx (s);
	if (parbuf.b_flags & B_ERROR) {
		bp->b_flags |= B_ERROR;
		iodone (bp);
		return;
	}
	/*
	 *	copy the partial block
	 */
	toc = ((bp->b_un.b_addr >> 6) & 01777) +
		 (((bp->b_bcount & ~511) >> 6) & 01777);
	toc += bp->b_xmem << 10;
	count = (bp->b_bcount & 511) >> 6;
	copy (parclick, toc, count);
	/*
	 *	release parbuf
	 */
	parbuf.b_flags &= ~B_BUSY;
	if (parbuf.b_flags & B_WANTED) {
		parbuf.b_flags &= B_WANTED;
		wakeup ((caddr_t) &parbuf);
	}
}

/*
 *	set up real[0] and real[1] to be the
 *	physical address of virtual
 */
map (virtual, real)
u_short	virtual;
u_short	real[2];
{
	u_short	nb, ts;

	nb = (virtual >> 6) & 01777;
	ts = KDSA0[nb >> 7] + (nb & 0177);
	real[0] = ((ts << 6) + (virtual & 077));
	real[1] = (ts >> 10) & 077;
}

xeselect (device)
int	device;
{
	register struct xedevice *xe = XEADDR;
	int	s;

	/*
	 *	initialize this unit
	 */
	if (!xeparam[device].xe_isset) {
		xei[device].xeihtrac = (xeparam[device].xe_ntrac >> 8) & 0377;
		xei[device].xeiltrac = xeparam[device].xe_ntrac & 0377;
		xei[device].xeinhead = xeparam[device].xe_nhead;
		xei[device].xeihrwcur = (xeparam[device].xe_rwcur >> 8) & 0377;
		xei[device].xeilrwcur = xeparam[device].xe_rwcur & 0377;
		xei[device].xeihwpc = (xeparam[device].xe_wpc >> 8) & 0377;
		xei[device].xeilwpc = xeparam[device].xe_wpc & 0377;
		xei[device].xeiecc = 11;
		xe->xecsr = CSRRESET;
		map(&xei[device], xeiaddr[device]);
	}
	xec.xeop = XEINIT;
	xec.xeunit = device << 5;
	xec.xehblk = 0;
	xec.xelblk = 0;
	xec.xecount = 0;
	xec.xecntl = xeparam[device].xe_seek;
	xe->xecar = xecaddr[0];
	xe->xedar = xeiaddr[device][0];
	xe->xexcar = xecaddr[1] >> 4;
	xe->xexdar = xeiaddr[device][1] >> 4;
	xe->xecsr = CSRGO | ((xecaddr[1] & 017) << 8) |
			((xeiaddr[device][1] & 017) << 2);
	/*
 	 *	wait a reasonable amount of time
	 *	for completion.
	 */
	s = 30000;
	while (!(xe->xecsr & CSRDONE)) {
		s--;
		if (s == 0)
			goto initerr;
	}
	if ((s = xe->xecsr) & CSRERROR) {
initerr:	;
		printf ("error initing drive");
		printf ("csr=%b ccsr=%b\n", s, XECSR_BITS,
			xe->xeccsr, XECCSR_BITS);
		return 0;
	}
	if (!xeparam[device].xe_isset) {
		xec.xeop = XERDY;
		xec.xeunit = device << 5;
		xec.xehblk = 0;
		xec.xelblk = 0;
		xec.xecount = 0;
		xec.xecntl = xeparam[device].xe_seek;
		xe->xecar = xecaddr[0];
		xe->xedar = xeiaddr[device][0];
		xe->xexcar = xecaddr[1] >> 4;
		xe->xexdar = xeiaddr[device][1] >> 4;
		xe->xecsr = CSRGO | ((xecaddr[1] & 017) << 8) |
				((xeiaddr[device][1] & 017) << 2);
		while (!(xe->xecsr & CSRDONE))
			;
		if ((s = xe->xecsr) & CSRERROR) {
			printf ("error initing drive");
			printf ("csr=%b ccsr=%b\n", s, XECSR_BITS,
				xe->xeccsr, XECCSR_BITS);
			return 0;
		}
		++xeparam[device].xe_isset;
	}
	xeicur = device;
	return 1;
}

/*
 *	start up a transfer onto a drive
 */
xestart ()
{
	register struct xedevice *xeaddr = XEADDR;
	register struct buf	*bp;
	register unit;
	struct buf	*dp;
	daddr_t	bn;
	int	device, i;

loop:
	/*
	 *	pull a request off the controller queue.
	 */
	dp = &xetab;
	if ((bp = dp->b_actf) == NULL) {
		return;
	}
	/*
	 *	mark controller busy and
	 *	determine destination of this request.
	 */
	dp->b_active++;
	unit = minor (bp->b_dev) & 077;
	device = unit >> 3;
	/*
 	 *	if the xebec is not currently
	 *	setup for this device, set it
	 *	up.
	 */
	if (device != xeicur) {
		if (!xeselect (device)) {
			printf ("oops, the controller could not select %d\n",
				device);
			bp->b_flags |= B_ERROR;
			bp->b_resid = bp->b_bcount;
			iodone (bp);
			dp->b_actf = bp->av_forw;
			dp->b_active = 0;
			goto loop;
		}
	}
	unit &= 7;
	bn = bp->b_blkno + xe_sizes[device][unit].blkoff;
	/*
	 *	select disk block and device
	 */
	xec.xelblk = bn & 0377;
	i = bn >> 8;
	xec.xehblk = i & 0377;
	i >>= 8;
	xec.xeunit = (device << 5) | (i & 0037);
	/*
 	 *	select seek rate
	 */
	xec.xecntl = xeparam[device].xe_seek;
	if (bp->b_flags & B_READ) {
		xec.xecount = (bp->b_bcount >> 9) & 0177;
		xec.xeop = XEREAD;
	} else {
		if (bp->b_bcount & 511)
			xec.xecount = ((bp->b_bcount + 511) >> 9) & 0177;
		else
			xec.xecount = (bp->b_bcount >> 9) & 0177;
		xec.xeop = XEWRITE;
	}
	/*
	 *	if we aren't doing a whole block,
	 *	then ignore the request and fetch
	 *	another from the queue
	 */
	if (xec.xecount == 0) {
		dp->b_actf = bp->av_forw;
		bp->b_resid = 0;
		iodone (bp);
		dp->b_active = 0;
		goto loop;
	}
	xeaddr->xexcar = (xecaddr[1] >> 4) & 03;
	xeaddr->xexdar = (bp->b_xmem >> 4) & 03;
	xeaddr->xecar = xecaddr[0];
	xeaddr->xedar = bp->b_un.b_addr;
	/*
	 *	warning: unit is being used as temporary
	 */
	unit = ((xecaddr[1] & 017) << 8) | CSRIE | ((bp->b_xmem & 017) << 2) | CSRGO;
	/*
	 *	run it
	 */
	xeaddr->xecsr = unit;
}

/*
 *	handle a disk interrupt
 */
xeintr()
{
	register struct xedevice *xeaddr = XEADDR;
	register struct buf *dp;
	register struct buf *bp;
	int i, j;

	dp = &xetab;
	if (dp->b_active) {
		bp = dp->b_actf;
		j = xeaddr->xeccsr;
		i = xeaddr->xecsr;
		/*
		 *	check for and process errors on
		 *	either the drive or the controller
		 */
		dp->b_active = 0;
		if (i & CSRERROR) {
#ifdef	UCB_DEVERR
			harderr (bp, "xe");
			printf ("csr=%b ccsr=%b\n", i, XECSR_BITS, j, XECCSR_BITS);
#else
			deverror (bp, i, j)
#endif
			if (xeselect ((minor (bp->b_dev) >> 3) & 07)) {
				/*
				 *	if we haven't gotten 10 errors yet,
				 *	retry the transfer
				 */
				if (++dp->b_errcnt <= 10) {
					xestart ();
					return;
				}
			}
			bp->b_flags |= B_ERROR;
		}
		dp->b_errcnt = 0;
		bp->b_resid = 0;
		dp->b_actf = bp->av_forw;
		iodone (bp);
	}
	xestart ();
}
#endif NXE

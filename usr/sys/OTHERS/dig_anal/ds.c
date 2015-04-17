/* %M%	%I%	(CARL)	%G%	%U% */
static char RCSid[] = "$Header: ds.c,v 1.8 84/10/03 04:44:51 lepreau Exp $";

#include "ds.h"
#if NDS > 0

/*
 * DSC System 200 driver
 * via DSC dma11.
 * vax 4.2bsd version
 */

/*
 * *=*=*=* BUGS *=*=*=*
 *
 * 1. if you #define ZEROBUF it has unpleasant
 *    side effects: it crashes the vax.
 */

/*
 * !!! WARNING !!! WARNING !!! WARNING !!!
 *
 * since the disk driver strategy routine
 * is called from the converters' interrupt
 * level the processor level must not be set
 * to 0 in the disk driver. also where the
 * processor level is raised before fiddling
 * with the buffer list it must not be "raised"
 * to lower than the dma11's interrupt level.
 * change the code in the strategy routine to
 * look something like this:
 *
 * (* the following #define causes data lates and was taken out *)
 * (* # define spl5 spl6	(* converters run at level 6 *)
 *
 *	opl = spl5();		(* CHANGED *)
 *	dp = &xxutab[ui->ui_unit];
 *	disksort(dp, bp);
 *	if (dp->b_active == 0) {
 *		xxustart(ui);
 *		bp = &ui->ui_mi->um_tab;
 *		if ((bp->b_actf != NULL) && (bp->b_active == 0))
 *			xxstart(ui->ui_mi);
 *	}
 *	(void) splx(opl);	(* CHANGED *)
 *
 * i suspect that you should also bracket the entire disk
 * interrupt routine with "opl = spl6() ... splx(opl)" but
 * we get data lates when this is done. the next paragraph
 * should be read with this in mind.
 *
 * the other alternative is to fix your dma11 so that it
 * interrupts at level 5 instead of level 6. dsc can give
 * you the information on how to do this. in either event
 * the strategy routine must be fixed as shown above.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../machine/pte.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../vaxuba/ubavar.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/file.h"

#include "../vaxuba/dsc.h"

#include "../vaxuba/dsreg.h"

/*
 * THESE ARE SITE-SPECIFIC
 *
 * bulk storage devices
 *
 * these can be tape, disk, bubble
 * or whatever.
 */

#include "up.h"
#include "hp.h"
#include "tu.h"
#include "te.h"
#include "ra.h"

#if NSC > 0
extern int upstrategy();
#endif NSC

#if NRA > 0
extern int udstrategy();
#endif NRA

#if NHT > 0
extern int htstrategy();
#endif NHT

#if NHP > 0
extern int hpstrategy();
#endif NHP

#if NTE > 0
extern int tmstrategy();
#endif NTE

struct bs {
	dev_t	d_cdev;		/* chrdev; major+minor */
	dev_t	d_bdev;		/* blkdev; major+minor */
	int	d_flags;	/* buffer flags */
	int	(*d_io)();	/* strategy routine */
} bs[] = {

#if NHP > 0
	/* rhp2b */		/* hp2b */
	{ makedev(4, 17),	makedev(0, 17),	0,	hpstrategy },
#endif NHP

#if NTE > 0
	/* rmt8 */		/* mt8 */
	{ makedev(14,8),	makedev(5,8),	B_TAPE,	tmstrategy }
#endif NTE

#ifdef NOTDEFED
#if NUP > 0
	/* rup0c */		/* up0c */
	{ makedev(13, 2),	makedev(2, 2),	0,	upstrategy },
	/* rup1f */		/* up1f */
	{ makedev(13, 13),	makedev(2, 13),	0,	upstrategy },
#endif NUP

#if NRA > 0
	/* rra0f */		/* ra0f */
	{ makedev(9, 5),	makedev(9, 5),	0,	udstrategy },
	/* rra1f */		/* ra1f */
	{ makedev(9, 13),	makedev(9, 13),	0,	udstrategy },
	/* rra2f */		/* ra1f */
	{ makedev(9, 21),	makedev(9, 21),	0,	udstrategy },
#endif NRA

#if NHT > 0
	/* rmt8 */		/* mt8 */
	{ makedev(14, 8),	makedev(5, 8),	B_TAPE,	htstrategy },
#endif NHT
#endif NOTDEFED

};

# define NBS		(sizeof(bs) / sizeof(bs[0]))

/*
 * THESE ARE SITE-SPECIFIC
 *
 * starting base for the
 * d/a and a/d converters,
 * for setting up sequence ram.
 */
# define ADBASE		040
# define DABASE		050

/*
 * reset device
 */
# define RESETDEV	bit(4)

/*
 * used to be a macro
 *
 * # define dsblock(sf) \
 * ((((sf)->f_todo - (sf)->f_dcnt) / 512) + (sf)->f_bno)
 */
extern daddr_t	dsblock();

# define size(sf, cnt) \
((cnt > (sf)->f_bsize) ? (sf)->f_bsize : cnt)
# define dsseq(ui, reg, conv, dir) \
((struct dsdevice *) (ui)->ui_addr)->ascseq[reg] = conv | ((dir & 01) << 6)

/*
 * ds flags
 */
# define DS_CLOSED	0
# define DS_OPEN	bit(0)
# define DS_BSY		bit(1)
# define DS_NDSK	bit(2)
# define DS_MON		bit(3)
# define DS_BRD		bit(4)

/*
 * params to driver
 */
# define A_TODO		bit(0)
# define A_BNO		bit(1)
# define A_CNT		bit(2)
# define A_SEQ		bit(3)
# define A_DEV		bit(4)
# define A_NBLKS	bit(5)
# define A_BLIST	bit(6)

# define MINARG 	(A_BNO | A_CNT | A_SEQ | A_DEV)
# define DSPRI		(PZERO-1)

/* size of the buffer holding the block list */
# define LISTSIZE	MAXBSIZE

/* number of disk addresses per buffer */
# define NDADDRS	(LISTSIZE / sizeof(daddr_t))

/*
 * relevant information
 * about the dma and asc
 */
struct ds_softc {
	int		c_dmacsr;	/* copy of dma csr on error */
	int		c_asccsr;	/* copy of asc csr on error */
	int		c_flags;	/* internal flags */
	int		c_errs;		/* errors, returned via ioctl */
	int		c_bufno;	/* dsubinfo/buffer */
	int		c_uid;		/* user id */
	int		c_args;		/* args received from user */
	int		c_wticks;	/* watch dog */
	int		c_ubinfo[NDSB];	/* uba info */
	int		c_bubinfo;	/* uba info, 1st buffer, once only */
	int		c_nblist;	/* length of block list */
	int		c_blkno;	/* current block in blist */
	int		c_mode;		/* mode opened for */
	struct buf	c_dsb[NDSB];	/* bs buffers */
	struct buf	c_blist;	/* block list */
	struct buf	*c_curb;	/* current buffer in block list */
} ds_softc[NDS];

/*
 * relevant information
 * about the disking and
 * other truck
 */
struct ds_softf {
	daddr_t	f_bno;			/* starting block */
	off_t	f_todo;			/* amnt. of file to convert */
	off_t	f_ccnt;			/* amnt. of data not converted */
	off_t	f_dcnt;			/* amnt. of file not diskio'd */
	off_t	f_icnt;			/* amnt. of file not seen by dsintr */
	int	f_bsize;		/* size of each buffer */
	int	f_boff;			/* offset into buffer of first i/o */
	int	f_dev;			/* bs device to use */
} ds_softf[NDS];

int dsprobe(), dsattach(), dsintr();

struct uba_device *dsdinfo[NDS];

u_short dsstd[] = {
	0165400, 0
};

struct uba_driver dsdriver = {
	dsprobe, 0, dsattach, 0, dsstd, "ds", dsdinfo
};

/* ARGSUSED */
dsopen(dev, mode)
	dev_t dev;
{
	register struct dsdevice *dsaddr;
	register struct uba_device *ui;
	register struct ds_softc *sc;
	register struct ds_softf *sf;
	register int unit;

	if ((unit = (minor(dev) & ~RESETDEV)) >= NDS)
		goto bad;

	if ((ui = dsdinfo[unit]) == NULL)
		goto bad;

	if (ui->ui_alive == 0)
		goto bad;

	sc = &ds_softc[ui->ui_unit];
	sf = &ds_softf[ui->ui_unit];

	/*
	 * if this is the reset device
	 * then just do a reset and return.
	 */
	if (minor(dev) & RESETDEV) {
		/*
		 * if the converters are in use then
		 * only the current user or root can
		 * do a reset.
		 */
		if (sc->c_flags & DS_OPEN) {
			if ((sc->c_uid != u.u_ruid) && (u.u_uid != 0)) {
				return(ENXIO);
			}
		}

		if (dsinit(unit)) {
			uprintf("ds%d: asc offline\n", ui->ui_unit);
			return(EIO);
		}

		return(0);
	}

	/*
	 * only one person can use it
	 * at a time
	 */
	if (sc->c_flags & DS_OPEN)
bad:		return(ENXIO);

	sc->c_mode = mode;

	/*
	 * initialize
	 */
	if (dsinit(unit)) {
		uprintf("ds%d: asc offline\n", ui->ui_unit);
		return(EIO);
	}

	sc->c_uid = u.u_ruid;

	sc->c_flags = DS_OPEN;

	return(0);
}

/* ARGSUSED */
dsclose(dev, flag) {
	register int unit;

	unit = minor(dev) & ~RESETDEV;
	ds_softc[unit].c_flags = DS_CLOSED;
	(void) dsinit(unit);

	return(0);
}

dsinit(unit) {
	register struct dsdevice *dsaddr;
	register struct uba_device *ui;
	register struct ds_softc *sc;
	register struct ds_softf *sf;
	register short dummy;
	register int offl;
	int flts;
	int odd;

	if (unit >= NDS)
		return(1);

	if ((ui = dsdinfo[unit]) == NULL)
		return(1);

	dsaddr = (struct dsdevice *) ui->ui_addr;

	if (dsaddr->dmacsr & DMA_OFL)
		offl = 1;
	else {
		flts = dsaddr->asccsr & ASC_HZMSK;	/* save filters */
		dsaddr->asccsr = flts;
		dsaddr->ascrst = 0;
		offl = 0;
	}

	sc = &ds_softc[ui->ui_unit];
	sf = &ds_softf[ui->ui_unit];

	odd = 0;

	/*
	 * flush out last remaining buffer
	 */
	if ((sc->c_mode & FREAD)	&&
	   (sf->f_dcnt > 0)		&&
	   (sc->c_flags & DS_BSY)	&&
	   (! (sc->c_flags & DS_NDSK)))	{
		register struct buf *bp;

		bp = &sc->c_dsb[sc->c_bufno % NDSB];

		/* BEGIN DEBUG */
		if (sf->f_bsize == 0) {
			uprintf("dsinit: zero bsize\n");
			sc->c_errs |= EDS_CERR;
			goto out;
		}
		/* END DEBUG */

		/* sometimes dmawc is odd? */
		bp->b_bcount = ~ dsaddr->dmawc;
		if ((bp->b_bcount % sizeof(short)) != 0)
			odd = bp->b_bcount;
		bp->b_bcount -= bp->b_bcount % sizeof(short);

		bp->b_blkno = dsblock(sf, sc);
		bp->b_flags &= ~ (B_DONE | B_ERROR);
		(*bs[sf->f_dev].d_io)(bp);

		sf->f_dcnt -= sf->f_bsize;

		out:;
	}

	dsfreeall(&sc->c_blist);

	dsaddr->dmablr = 0;
	dsaddr->dmasax = 0;
	dsaddr->dmasar = 0;
	dsaddr->dmacsr = 0;
	dsaddr->dmawc = 0;
	dsaddr->dmaacx = 0;
	dsaddr->dmaac = 0;
	dsaddr->dmadr = 0;
	dsaddr->dmaiva = 0;
	dsaddr->dmaclr = 0;
	dsaddr->dmacls = 0;
	dummy = dsaddr->dmasar;			/* clears sar flag */

#ifdef notdef
	/*
	 * put converters in monitor mode
	 */
	if (offl == 0) {
		dsseq(ui, 0, ADBASE+0, AD);
		dsseq(ui, 1, ADBASE+1, AD);
		dsaddr->ascseq[1] |= bit(7);	/* last sequence reg */
		flts = dsaddr->asccsr & ASC_HZMSK;	/* save filters */
		dsaddr->asccsr = flts | ASC_RUN | ASC_MON;
	}
#endif notdef

	/* reset ds_softc */
	sc->c_dmacsr = 0;
	sc->c_asccsr = 0;
	sc->c_flags = 0;
	sc->c_errs = 0;
	sc->c_bufno = 0;
	sc->c_args = 0;
	sc->c_blkno = 0;
	sc->c_mode = 0;

	/* reset ds_softf */
	sf = &ds_softf[ui->ui_unit];
	sf->f_bno = 0;
	sf->f_todo = 0;
	sf->f_ccnt = 0;
	sf->f_dcnt = 0;
	sf->f_icnt = 0;
	sf->f_bsize = 0;
	sf->f_boff = 0;
	sf->f_dev = 0;

	/*
	 * terminate current run
	 */
	sc->c_flags &= ~ DS_BSY;
	wakeup((caddr_t) &ds_softc[ui->ui_unit]);
	if (sc->c_flags & DS_BSY)
		sc->c_errs |= EDS_RST;

	if (odd)
		printf("ds%d: dsinit: odd dmawc (%d)\n", ui->ui_unit, odd);

	return(offl);
}

/*
 * f_bsize is the size of the buffers used for the i/o. the buffers are
 * obtained by taking the user buffer and splitting it into NDSB buffers.
 * the user buffer should be a multiple of the track size of the disk.
 * this is for efficiency of disk i/o.
 *
 * using uio_resid, iov_base, and f_bsize each buffer header is set up.
 * the converters only need the base address of the buffer and the word
 * count. the disk needs these in addition to the block number. if we
 * are doing d/a conversions then we start the disk up to fill up the
 * buffers.
 *
 * after everything is ready to go we turn on the RUN bit and let 'em rip.
 */
dsstart(unit, rw, uio)
	struct uio *uio;
{
	register struct dsdevice *dsaddr;
	register struct uba_device *ui;
	register struct ds_softc *sc;
	register struct ds_softf *sf;
	register struct iovec *iov;
	register struct buf *bp;
	int dummy;
	int bits;
	int blr;
	int opl;
	int i;

	sc = &ds_softc[unit];
	sf = &ds_softf[unit];

	iov = uio->uio_iov;

	sf->f_bsize = (iov->iov_len / NDSB);
/* begin debug */
	if (sf->f_bsize == 0) {
		uprintf("dsstart: zero bsize (1st check)\n");
		goto bad;
	}
/* end debug */

	/*
	 * make sure we have all
	 * necessary info.
	 */
	if ((sc->c_args & MINARG) != MINARG) {
		sc->c_errs |= EDS_ARGS;
		goto bad;
	}

	/*
	 * either the converters will be writing
	 * to memory or the disk will be.
	 */
	if (useracc(iov->iov_base, iov->iov_len, B_WRITE) == NULL) {
		sc->c_errs |= EDS_ACC;
		return(EFAULT);
	}

	/*
	 * check for misaligned buffer.
	 * 
	 * the "% DEV_BSIZE" check isn't correct. it doesn't hurt
	 * though. it probably should be deleted.
	 */
	if (((iov->iov_len % DEV_BSIZE) != 0) || (iov->iov_len & 01)) {
		sc->c_errs |= EDS_MOD;
		goto bad;
	}

	/*
	 * check for tiny buffer
	 */
	if (sf->f_bsize < DEV_BSIZE) {
		sc->c_errs |= EDS_SIZE;
		goto bad;
	}

	/*
	 * check for unreasonable buffer
	 * offset for first buffer
	 */
	if (sf->f_boff >= sf->f_bsize) {
		sc->c_errs |= EDS_ARGS;
		goto bad;
	}

	u.u_procp->p_flag |= SPHYSIO;

	sc->c_flags |= DS_BSY;

	vslock(iov->iov_base, (int) iov->iov_len);	/* fasten seat belts */

	/*
	 * point each c_dsb somewhere into
	 * the user's buffer, set up each c_dsb.
	 * the buffer's rw flag is set to the
	 * inverse of our operation since it
	 * applies to the bulk storage device
	 * i/o to be done.
	 */
	for (i = 0; i < NDSB; i++) {
		bp = &sc->c_dsb[i];
		bp->b_un.b_addr = iov->iov_base + (i * sf->f_bsize);
		bp->b_error = 0;
		bp->b_proc = u.u_procp;
		bp->b_flags = bs[sf->f_dev].d_flags | B_PHYS | ((rw == B_READ) ? B_WRITE : B_READ);
		bp->b_dev = bs[sf->f_dev].d_cdev;
		bp->b_bcount = sf->f_bsize;
		bp->b_flags |= B_BUSY;
		if (rw == B_WRITE) {
			if ((sc->c_flags & DS_NDSK) == 0) {
/* begin debug */
				if (sf->f_bsize == 0) {
					uprintf("dsstart: zero bsize");
					uprintf(" i=%d, unit=%d\n", i, unit);

					/* for printf in wait loop below */
					ui = dsdinfo[unit];
					sc->c_errs |= EDS_ARGS;
					goto out;
				}
/* end debug */
				bp->b_blkno = dsblock(sf, sc);
				sc->c_blkno++;

/* uprintf("dsstart: blk=%ld, count=%d\n", bp->b_blkno, bp->b_bcount); */
				(*bs[sf->f_dev].d_io)(bp);

				/* iodone will wake us up */
				while ((bp->b_flags & B_DONE) == 0)
					sleep((caddr_t) bp, DSPRI);

				/* oops */
				if (bp->b_flags & B_ERROR) {
/* printf("dsstart: disk error (block=%ld)\n", bp->b_blkno); */
					sc->c_errs |= EDS_DISK;
					goto out;
				}
			}
			else
				bp->b_flags |= B_DONE;

			sf->f_dcnt -= sf->f_bsize;
		}
		else
			bp->b_flags |= B_DONE;
	}

	ui = dsdinfo[unit];
	dsaddr = (struct dsdevice *) ui->ui_addr;

	opl = spl6();
	/*
	 * if reading then going to
	 * memory so set W2M
	 */
	if (rw == B_WRITE)
		dsaddr->asccsr |= ASC_PLAY;
	else {
		dsaddr->asccsr |= ASC_RECORD;
		dsaddr->dmacsr |= DMA_W2M;
	}

	dsaddr->dmacsr |= DMA_CHN | DMA_SFL;
	dummy = dsaddr->dmasar;
	dsaddr->asccsr &= ~ ASC_MON;
	dummy = dsaddr->ascrst;
 	dsaddr->asccsr |= ASC_IE;
	dsaddr->dmacls = 0;
	dsaddr->dmacsr |= DMA_IE;
	dsaddr->dmawc = -1;

	/*
	 * if there is a buffer offset
	 * then adjust b_addr temporarily.
	 * this fudging around is done
	 * unconditionally since f_boff
	 * is zeroed in dsopen so if the user
	 * hasn't specified a boff then this
	 * shouldn't affect b_addr.
	 */
	if ((blr = size(sf, sf->f_ccnt) - sf->f_boff) <= 0) {
		sc->c_errs |= EDS_ARGS;
		goto bad;
	}
	sc->c_ubinfo[0] = ubasetup(ui->ui_ubanum, &sc->c_dsb[0], UBA_NEEDBDP);
	sc->c_bubinfo = sc->c_ubinfo[0] + sf->f_boff;
	dsaddr->dmablr = ~ (blr >> 1);
	dsaddr->dmasax = (sc->c_bubinfo >> 16) & 03;
	dsaddr->dmasar = sc->c_bubinfo;
	sf->f_ccnt -= blr;
#ifdef debug
	if (sf->f_ccnt <= 0)
		uprintf("dsstart: dsb0: f_ccnt=%d\n", sf->f_ccnt);
#endif debug

	sc->c_ubinfo[1] = ubasetup(ui->ui_ubanum, &sc->c_dsb[1], UBA_NEEDBDP);

	/*
	 * set monitor mode or
	 * broadcast mode.
	 */
	bits = 0;
	if (sc->c_flags & DS_MON)
		bits |= ASC_MON;
	if (sc->c_flags & DS_BRD)
		bits |= ASC_BRD;

	/*
	 * as they say in california,
	 * ``go for it''
	 */
	dsaddr->asccsr |= ASC_RUN | bits;

	/*
	 * if the file is so small and only uses the first
	 * buffer we would load dmablr with 0 but we can't
	 * do that because we get errors (data late, external
	 * interrupt), so we fudge and give it f_bsize. dsintr
	 * will shut down the converters before it tries to play
	 * the 2nd (bogus) block.
	 */
	if ((blr = size(sf, sf->f_ccnt)) <= 0) {
#ifdef debug
		uprintf("dsstart: dsb1: blr=%d\n", blr);
#endif debug
		blr = sf->f_bsize;
	}

	dsaddr->dmablr = ~ (blr >> 1);
	dsaddr->dmasax = (sc->c_ubinfo[1] >> 16) & 03;
	dsaddr->dmasar = sc->c_ubinfo[1];
	sf->f_ccnt -= blr;
#ifdef debug
	if (sf->f_ccnt <= 0)
		uprintf("dsstart: dsb1: f_ccnt=%d\n", sf->f_ccnt);
#endif debug
	splx(opl);

	/*
	 * wait for interrupt routine to signal
	 * end of conversions.
	 */
	while (sc->c_flags & DS_BSY)
		sleep((caddr_t) &ds_softc[ui->ui_unit], DSPRI);

	/*
	 * wait for disk i/o to complete.
	 * release unibus resources.
	 */
out:	for (i = 0; i < NDSB; i++) {
		if ((sc->c_dsb[i].b_flags & B_DONE) == 0) {
			int waittime;
			for (waittime = 0; waittime < 7; waittime++) {
				sleep((caddr_t) &lbolt, DSPRI);
				if (sc->c_dsb[i].b_flags & B_DONE)
					goto done;
			}

			printf("ds%d: dsb%d: wait timeout\n", ui->ui_unit, i);
		}

done:		if (sc->c_ubinfo[i] != 0) {
			ubarelse(ui->ui_ubanum, &sc->c_ubinfo[i]);
			sc->c_ubinfo[i] = 0;
		}
	}

	/*
	 * either the disk or the
	 * converters were writing
	 * to memory, thus the B_READ.
	 */
	vsunlock(iov->iov_base, (int) iov->iov_len, B_READ);

	u.u_procp->p_flag &= ~ SPHYSIO;

	if (sc->c_errs)
bad:		return(EIO);

	return(0);
}

/*
 * this is where the real work is done. we copy any device registers that
 * we will be looking at to decrease the amount of traffic on the ds 200
 * bus. also you can't do a "read-modify-write" (I think that this is called
 * a DATIP followed by a DATO in DEC manuals) on any of the ds registers
 * while the converters are running.
 *
 * note that you must write something to the dma11 clear status register
 * or you'll never get any more interrupts.
 *
 * c_wticks gets cleared on each interrupt. dswatch() increments
 * c_wticks and if c_wticks gets over a threshold then the
 * addacs are assumed to have hung and we do a reset.
 */
dsintr(dev)
	dev_t dev;
{
	register struct dsdevice *dsaddr;
	register struct ds_softc *sc;
	register struct ds_softf *sf;
	register struct buf *bp;
	register int bufno;
	register int i;
	int unit;
	int blr;

	unit = minor(dev) & ~RESETDEV;
	sc = &ds_softc[unit];
	sf = &ds_softf[unit];

	/*
	 * reset c_wticks
	 */
	sc->c_wticks = 0;

	dsaddr = (struct dsdevice *) dsdinfo[unit]->ui_addr;

	sc->c_asccsr = dsaddr->asccsr;
	sc->c_dmacsr = dsaddr->dmacsr;

	/*
	 * get current buffer
	 */
	bufno = sc->c_bufno % NDSB;
	bp = &sc->c_dsb[bufno];

	/*
	 * check to see if any disking
	 * needs to be done
	 */
	if (sf->f_dcnt > 0) {
		if ((bp->b_flags & B_DONE) == 0) {
/* printf("dsintr: block not done\n"); */
			sc->c_errs |= EDS_DISK;
			goto out;
		}

		if ((sc->c_flags & DS_NDSK) == 0) {
/* begin debug */
			if (sf->f_bsize == 0) {
				uprintf("dsintr: zero bsize\n");
				sc->c_errs |= EDS_CERR;
				goto out;
			}
/* end debug */

			bp->b_blkno = dsblock(sf, sc);
			sc->c_blkno++;

			bp->b_flags &= ~ (B_DONE | B_ERROR);
/* uprintf("dsintr: blk=%d, count=%d\n", bp->b_blkno, bp->b_bcount); */
			(*bs[sf->f_dev].d_io)(bp);
		}
		else
			bp->b_flags |= B_DONE;

		sf->f_dcnt -= sf->f_bsize;
	}

	/*
	 * check to see if converting
	 * is finished
	 */
	if ((sf->f_icnt -= sf->f_bsize) <= 0)
		goto out;

	/*
	 * check for converter error.
	 * all errors and normal termination
	 * come here.
	 */
	if ((sc->c_dmacsr & (DMA_ERR | DMA_BSY)) != DMA_BSY) {
		int flts;
		sc->c_errs |= EDS_CERR;
		printf("ds%d error: asccsr=%b, dmacsr=%b\n", unit, sc->c_asccsr, ASC_BITS, sc->c_dmacsr, DMA_BITS);

out:		sc->c_flags &= ~ DS_BSY;
		flts = dsaddr->asccsr & ASC_HZMSK;	/* save filters */
		dsaddr->asccsr = flts;			/* clears run */
		dsaddr->dmacsr = 0;			/* turn off dma11 */
		wakeup((caddr_t) &ds_softc[unit]);
		return(0);
	}

	/*
	 * reinitialize dma11
	 */
	dsaddr->dmacls = 0;

	/*
	 * load dma11 registers
	 */
	blr = size(sf, sf->f_ccnt);
	if (sf->f_ccnt > 0) {
		dsaddr->dmablr = ~ (blr >> 1);
		dsaddr->dmasax = (sc->c_ubinfo[bufno] >> 16) & 03;
		dsaddr->dmasar = sc->c_ubinfo[bufno];
	}

	/*
	 * try to catch disk errors
	 */
	for (i = 0; i < NDSB; i++) {
		if (sc->c_dsb[i].b_flags & B_ERROR) {
/* printf("dsintr: disk error (block=%ld)\n", sc->c_dsb[i].b_blkno); */
			sc->c_errs |= EDS_DISK;
			goto out;
		}
	}

	/*
	 * update converted byte count.
	 * update buffers converted count.
	 */
	sf->f_ccnt -= blr;
	sc->c_bufno++;

#ifdef ZEROBUF
	/*
	 * last buffer is full of
	 * zeroes
	 */
	if (sf->f_ccnt <= sf->f_bsize) {
		bufno = sc->c_bufno % NDSB;
		dsbclr(sc->c_dsb[bufno].b_un.b_addr, sc->c_dsb[bufno].b_bcount);
	}
#endif ZEROBUF

	return(0);
}

/*
 * last time i checked, this
 * didn't work
 */
dsbclr(base, bcnt)
	caddr_t base;
	unsigned int bcnt;
{
	/* debugging stuff */
	if (! kernacc(base, bcnt, B_READ)) {
		uprintf("can't dsbclr\n");
		return;
	}
	;				/* Avoid asm() label botch */
	/* end debugging stuff */

	{ asm("	movc5	$0,(sp),$0,8(ap),*4(ap)"); }
}

/*
 * a/d conversion
 */
dsread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int unit;

	unit = minor(dev) & ~RESETDEV;
	return(dsstart(unit, B_READ, uio));	/* writes on disk */
}

/*
 * d/a conversion
 */
dswrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int unit;

	unit = minor(dev) & ~RESETDEV;
	return(dsstart(unit, B_WRITE, uio));	/* reads from disk */
}

/* ARGSUSED */
dsioctl(dev, cmd, addr, flag)
	dev_t dev;
	caddr_t addr;
{
	register struct dsdevice *dsaddr;
	register struct ds_softc *sc;
	register struct ds_softf *sf;
	register struct ds_seq *dq;
	register struct ds_err *de;
	register struct ds_fs *df;
	register struct bs *bsp;
	struct uba_device *ui;
	struct ds_seq ds_seq;
	struct ds_err ds_err;
	struct ds_fs ds_fs;
	int unit;
	int flts;
	int i;

	unit = minor(dev) & ~RESETDEV;

	sc = &ds_softc[unit];
	sf = &ds_softf[unit];
	ui = dsdinfo[unit];
	dsaddr = (struct dsdevice *) ui->ui_addr;

	switch (cmd) {
		/* put in broadcast mode */
		case DSBRD:
			if (! (sc->c_args & A_SEQ)) {
				sc->c_errs |= EDS_ARGS;
				return(EINVAL);
			}

			sc->c_flags |= DS_BRD;
			break;

		/* put in monitor mode */
		case DSMON:
			if (! (sc->c_args & A_SEQ)) {
				sc->c_errs |= EDS_ARGS;
				return(EINVAL);
			}

			sc->c_flags |= DS_MON;

			break;

		/* set sequence */
		case DSSEQ:
			dq = &ds_seq;
			bcopy(addr, (caddr_t) dq, sizeof(struct ds_seq));
			if ((dq->reg > 15) || (dq->reg < 0))
				return(EINVAL);

			dsseq(ui, (int) dq->reg, (int) dq->conv, (int) dq->dirt);
			break;

		/* mark last sequence register */
		case DSLAST:
			dq = &ds_seq;
			bcopy(addr, (caddr_t) dq, sizeof(struct ds_seq));
			if ((dq->reg > 15) || (dq->reg < 0))
				return(EINVAL);

			dsaddr->ascseq[dq->reg] |= bit(7);
			sc->c_args |= A_SEQ;
			break;

		/* starting block number */
		case DSBNO:
			df = &ds_fs;
			bcopy(addr, (caddr_t) df, sizeof(struct ds_fs));
			if (df->bnosiz < 0)
				return(EINVAL);

			sf->f_bno = df->bnosiz;
			sc->c_args |= A_BNO;
			break;

		/* no. of bytes to convert */
		case DSCOUNT:
			df = &ds_fs;
			bcopy(addr, (caddr_t) df, sizeof(struct ds_fs));

			/*
			 * DSCOUNT is in bytes so
			 * must be modulo sizeof(short)
			 */
			if ((df->bnosiz & 01) || (df->bnosiz <= 0))
				return(EINVAL);

			sf->f_todo = sf->f_dcnt = sf->f_ccnt = sf->f_icnt = df->bnosiz;
			sc->c_args |= A_CNT;
			break;

		/* bs device to use */
		case DSDEV:
			dq = &ds_seq;
			bcopy(addr, (caddr_t) dq, sizeof(struct ds_seq));

#ifdef MAJMIN
			for (bsp = &bs[0]; bsp < &bs[NBS]; bsp++) {
				if (dq->dirt == bsp->d_cdev)
					break;
			}

			if (bsp == &bs[NBS])
				return(EINVAL);

			if (bsdevchk(dq->dirt) == -1)
				return(EBUSY);
#else MAJMIN
			if ((dq->dirt >= NBS) || (dq->dirt < 0))
				return(EINVAL);

			if (bsdevchk(bs[dq->dirt].d_bdev) == -1)
				return(EBUSY);
#endif MAJMIN

			sf->f_dev = dq->dirt;
			sc->c_args |= A_DEV;
			break;

		/* set sample rate */
		case DSRATE:
			dq = &ds_seq;
			bcopy(addr, (caddr_t) dq, sizeof(struct ds_seq));
			dsaddr->ascsrt = dq->dirt;
			break;

		case DS20KHZ:
			dsaddr->asccsr &= ~ ASC_HZMSK;
			dsaddr->asccsr |= ASC_HZ20;	/* set 20kHz filter */
			break;

		case DS10KHZ:
			dsaddr->asccsr &= ~ ASC_HZMSK;
			dsaddr->asccsr |= ASC_HZ10;	/* set 10kHz filter */
			break;

		case DS5KHZ:
			dsaddr->asccsr &= ~ ASC_HZMSK;
			dsaddr->asccsr |= ASC_HZ05;	/* set 5kHz filter */
			break;

		case DSBYPAS:
			dsaddr->asccsr &= ~ ASC_HZMSK;
			dsaddr->asccsr |= ASC_BYPASS;	/* set bypass */
			break;

		/* no bs device i/o */
		case DSNODSK:
			sc->c_flags |= DS_NDSK;
			sc->c_args |= A_DEV | A_BNO;
			break;

		/* fetch errors */
		case DSERRS:
			de = &ds_err;
			de->dma_csr = sc->c_dmacsr;
			de->asc_csr = sc->c_asccsr;
			de->errors = sc->c_errs;
			bcopy((caddr_t) de, addr, sizeof(struct ds_err));
			break;

		/* byte offset into 1st buffer */
		case DSBOFF:
			df = &ds_fs;
			bcopy(addr, (caddr_t) df, sizeof(struct ds_fs));

			/*
			 * DSBOFF is in bytes so
			 * must be modulo sizeof(short)
			 */
			if ((df->bnosiz & 01) || (df->bnosiz < 0))
				return(EINVAL);

			sf->f_boff = df->bnosiz;
			break;

		/* how many samples actually converted */
		case DSDONE:
			df = &ds_fs;
			df->bnosiz = sf->f_todo - sf->f_icnt;
			df->bnosiz += ~ dsaddr->dmawc;
			bcopy((caddr_t) df, addr, sizeof(struct ds_fs));
			break;

		case DSNBLKS:
			df = &ds_fs;
			bcopy(addr, (caddr_t) df, sizeof(struct ds_fs));

			if (df->bnosiz < 1)
				return(EINVAL);

			sc->c_nblist = df->bnosiz;
			sc->c_args |= A_NBLKS;
			break;

		case DSBLKS:
			if ((sc->c_args & A_NBLKS) == 0) {
				sc->c_errs |= EDS_ARGS;
				return(EINVAL);
			}

			for (i = sc->c_nblist * sizeof(daddr_t); i > 0; i -= LISTSIZE) {
				struct buf	*bp;
				unsigned int	amt;

				bp = geteblk(LISTSIZE);
				clrbuf(bp);

				amt = i > LISTSIZE ? LISTSIZE : i;

				if (copyin(*(caddr_t *)addr, bp->b_un.b_addr, amt)) {
					dsfreeall(&sc->c_blist);
					return(EFAULT);
				}

				*(caddr_t *)addr += amt;

				dslink(&sc->c_blist, bp);
			}
			sc->c_curb = sc->c_blist.av_forw;

			sc->c_args |= A_BNO | A_BLIST;
			break;

		default:
			return(ENOTTY);
			break;
	}

	return(0);
}

/*
 * link a buffer onto the buffer list
 * of blocks
 */
dslink(dp, bp)
	register struct buf	*dp, *bp;
{
	dp->av_back->av_forw = bp;
	bp->av_back = dp->av_back;
	dp->av_back = bp;
	bp->av_forw = dp;
}

/*
 * return all of the buffers to
 * the system
 */
dsfreeall(dp)
	register struct buf	*dp;
{
	register struct buf	*bp;

	for (bp = dp->av_forw; bp != dp; bp = dp->av_forw) {
		bp->av_back->av_forw = bp->av_forw;
		bp->av_forw->av_back = bp->av_back;
		brelse(bp);
	}
}

daddr_t
dsblock(sf, sc)
	register struct ds_softf	*sf;
	register struct ds_softc	*sc;
{
	daddr_t				blkno;
	daddr_t				*daddrs;
	register struct buf		*bp;

	if (sc->c_args & A_BLIST) {
		/* circular block list */
		blkno = sc->c_blkno % sc->c_nblist;

		/*
		 * if we're at the end of this buffer then
		 * move on to the next one.
		 */
		if ((blkno % NDADDRS) == 0) {
			/* special case for first time only */
			if (sc->c_bufno != 0)
				sc->c_curb = sc->c_curb->av_forw;
		}

		/*
		 * convert the buffer data from an array of
		 * bytes to an array of longs, then index
		 * into that to get the block.
		 */
		daddrs = (daddr_t *) sc->c_curb->b_un.b_addr;
		blkno = daddrs[blkno % NDADDRS];
	}
	else
		blkno = ((sf->f_todo - sf->f_dcnt) / 512) + sf->f_bno;

	return(blkno);
}

/*
 * user level entry to dsinit. check that
 * unit is valid and call dsinit. this is
 * used by user programs via the dsinit
 * system call to reset the dacs.
 */
dsuinit() {
	register struct uba_device *ui;
	register struct ds_softc *sc;
	register int unit;
	register struct a {
		int dsunit;
	} *uap;

	uap = (struct a *) u.u_ap;

	if ((unit = uap->dsunit) >= NDS)
		return(ENXIO);

	if ((ui = dsdinfo[unit]) == NULL)
		return(ENXIO);

	sc = &ds_softc[ui->ui_unit];

	/*
	 * possible abnormal termination;
	 * only the current user or root can
	 * terminate an active run
	 */
	if (sc->c_flags & DS_OPEN) {
		if (sc->c_uid != u.u_ruid) {
			if (u.u_uid != 0)
				return(ENXIO);
		}
	}

	if (dsinit(ui->ui_unit)) {
		uprintf("ds%d: asc offline\n", ui->ui_unit);
		return(EIO);
	}

	return(0);
}

/*
 * set up asc sequence registers.
 * dir == 0 means d/a transfer,
 * dir == 1 means a/d transfer.
 *
 * this has been transformed into
 * a macro
 *
dsseq(ui, reg, conv, dir)
	register struct uba_device *ui;
{
	register struct dsdevice *dsaddr;
	register int i, j;

	dsaddr = (struct dsdevice *) ui->ui_addr;

	dir = ((dir & 01) << 6);

	dsaddr->ascseq[reg] = conv | dir;
}
*/

/*
 * check to see if the indicated bulk storage
 * device is a mounted filesystem.
 */
bsdevchk(bsdev)
	register int bsdev;
{
	register struct mount *mp;

	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++) {
		if (bsdev == mp->m_dev) {
			return(EBUSY);
		}
	}

	return(0);
}

/*
 * all this just to generate
 * an interrupt; rather
 * involved isn't it?
 */
dsprobe(reg)
	caddr_t reg;
{
	register int br, cvec;		/* value-result */
	register struct dsdevice *dsaddr;
	int dummy;
	int offl;

#ifdef lint	
	br = 0; cvec = br; br = cvec;
#endif lint

	dsaddr = (struct dsdevice *) reg;

	dsaddr->dmacsr = 0;
	dsaddr->dmacls = 0;
	dsaddr->ascseq[0] = DABASE+0 | ((DA & 01) << 6);
	dsaddr->ascseq[0] |= bit(7);	/* last sequence register */
	dsaddr->ascsrt = 0100;
	dsaddr->dmacsr = DMA_IE;
	dsaddr->asccsr = ASC_RUN | ASC_IE;

	DELAY(40000);

	/*
	 * now shut everything down.
	 * too bad we have to duplicate
	 * the code from dsinit but to
	 * call dsinit we need to give
	 * it a unit.
	 */
	if ((dsaddr->dmacsr & DMA_OFL) == 0) {
		dsaddr->asccsr = 0;
		dsaddr->ascrst = 0;
		offl = 0;
	}
	else
		offl = 1;

	dsaddr->dmablr = 0;
	dsaddr->dmasax = 0;
	dsaddr->dmasar = 0;
	dsaddr->dmacsr = 0;
	dsaddr->dmawc = 0;
	dsaddr->dmaacx = 0;
	dsaddr->dmaac = 0;
	dsaddr->dmadr = 0;
	dsaddr->dmaiva = 0;
	dsaddr->dmaclr = 0;
	dsaddr->dmacls = 0;
	dummy = dsaddr->dmasar;			/* clears sar flag */

#ifdef notdef
	/*
	 * put converters in monitor mode
	 */
	if (offl == 0) {
		dsaddr->ascseq[0] = ADBASE+0 | ((AD & 01) << 6);
		dsaddr->ascseq[1] = ADBASE+1 | ((AD & 01) << 6);
		dsaddr->ascseq[1] |= bit(7);	/* last sequence register */
		dsaddr->asccsr = ASC_RUN | ASC_MON;
	}
#endif notdef

	return(sizeof(struct dsdevice));
}

dsattach(ui)
	struct uba_device *ui;
{
	extern int dswatch();
	static int dswstart = 0;
	register struct ds_softc *sc;

	/*
	 * set up the blist linked list
	 * to the empty list. this must
	 * be done first because dsinit()
	 * calls dsfreeall().
	 */
	sc = &ds_softc[ui->ui_unit];
	sc->c_blist.av_forw = &sc->c_blist;
	sc->c_blist.av_back = &sc->c_blist;

	(void) dsinit(ui->ui_unit);

	/*
	 * start watchdog
	 */
	if (dswstart == 0) {
		timeout(dswatch, (caddr_t) 0, hz);
		dswstart++;
	}

	return(0);
}

/*
 * c_wticks gets cleared on each interrupt. dswatch()
 * increments c_wticks and if c_wticks gets over
 * a threshold then the addacs are assumed to have
 * hung and we do a reset.
 */
dswatch() {
	register struct uba_device *ui;
	register struct ds_softc *sc;
	register int ds;

	/* requeue us */
	timeout(dswatch, (caddr_t) 0, hz);

	for (ds = 0; ds < NDS; ds++) {
		if ((ui = dsdinfo[ds]) == NULL)
			continue;
		if (ui->ui_alive == 0)
			continue;

		sc = &ds_softc[ds];
		if ((sc->c_flags & DS_BSY) == 0) {
			sc->c_wticks = 0;
			continue;
		}

		sc->c_wticks++;
		if (sc->c_wticks >= 25) {
			sc->c_wticks = 0;
			printf("ds%d: lost interrupt\n", ds);
			ubareset(ui->ui_ubanum);
		}
	}

	return(0);
}

/*
 * zero uba vector.
 * shut off converters.
 * set error bit.
 */
dsreset(uban) {
	register struct uba_device *ui;
	register struct ds_softc *sc;
	register int ds;
	register int i;

	for (ds = 0; ds < NDS; ds++) {
		if ((ui = dsdinfo[ds]) == NULL)
			continue;
		if (ui->ui_alive == 0)
			continue;
		if (ui->ui_ubanum != uban)
			continue;

		printf(" ds%d", ds);

		sc = &ds_softc[ds];

		/*
		 * this used to be after the ubarelse's,
		 * seems like it should go before them
		 */
		if (dsinit(ds))
			printf("<dsinit error>");

		/*
		 * release unibus resources
		 */
		for (i = 0; i < NDSB; i++) {
			if (sc->c_ubinfo[i] != 0) {
				printf("<%d>", (sc->c_ubinfo[i] >> 28) & 0xf);
				sc->c_ubinfo[i] = 0;
			}
		}
	}

	return(0);
}
#else
dsuinit() {
	return(0);
}
#endif

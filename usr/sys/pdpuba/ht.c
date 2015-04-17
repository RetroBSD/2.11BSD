/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ht.c	2.2 (2.11BSD GTE) 1/2/93
 */

/*
 * TJU77/TWU77/TJE16/TWE16 tape driver
 */
#include "ht.h"
#if	NHT > 0
#include "param.h"
#include "buf.h"
#include "ioctl.h"
#include "conf.h"
#include "file.h"
#include "user.h"
#include "mtio.h"
#include "fs.h"
#include "htreg.h"
#include "systm.h"

struct	buf	httab, chtbuf;
static	short	rh70;		/* ht.c was ONLY user of B_RH70 and that bit
				 * was wanted for something else (B_LOCKED)
				*/
struct	htdevice	*HTADDR;

#define	INF	32760

struct	softc	{
	char	sc_openf;
	char	sc_lastiow;
	daddr_t	sc_blkno;
	daddr_t	sc_nxrec;
	u_short	sc_erreg;
	u_short	sc_fsreg;
	short	sc_resid;
	short	sc_dens;
}	tu_softc[NHT];


#define	SIO	1
#define	SSFOR	2
#define	SSREV	3
#define	SRETRY	4
#define	SCOM	5
#define	SOK	6


/* bits in minor device */
#define	TUUNIT(dev)	(minor(dev) & 03)
#define	H_NOREWIND	004
#define	H_1600BPI	010

htattach(addr, unit)
register struct htdevice *addr;
{
	/*
	 * This driver supports only one controller.
	 */
	if (unit == 0) {
		HTADDR = addr;
		if (fioword(&(addr->htbae)) != -1)
			rh70 = 1;
		return(1);
	}
	return(0);
}

htopen(dev, flag)
dev_t	dev;
{
	register ds;
	register htunit = TUUNIT(dev);
	register struct softc *sc = &tu_softc[htunit];
	int olddens, dens;

	httab.b_flags |= B_TAPE;
	if (!HTADDR || htunit >= NHT)
		return(ENXIO);
	if (sc->sc_openf)
		return(EBUSY);
	sc->sc_blkno = (daddr_t) 0;
	sc->sc_nxrec = (daddr_t) INF;
	sc->sc_lastiow = 0;
	olddens = sc->sc_dens;
	dens = sc->sc_dens =
	    ((minor(dev)&H_1600BPI)?HTTC_1600BPI:HTTC_800BPI)|
		HTTC_PDP11|htunit;
	ds = htcommand(dev, HT_SENSE, 1);
	sc->sc_dens = olddens;
	if ((ds & HTFS_MOL) == 0) {
		uprintf("tu%d: not online\n", htunit);
		return(EIO);
	}
	if ((flag & FWRITE) && (ds & HTFS_WRL)) {
		uprintf("tu%d: no write ring\n", htunit);
		return(EIO);
	}
	if ((flag & FWRITE) && (ds & HTFS_BOT) == 0 &&
	    dens != sc->sc_dens) {
		uprintf("tu%d: can't change density in mid-tape\n", htunit);
		return (EIO);
	}
	sc->sc_openf = 1;
	sc->sc_dens = dens;
	return(0);
}

htclose(dev, flag)
dev_t	dev;
{
	register struct softc *sc = &tu_softc[TUUNIT(dev)];

	if (flag == FWRITE || ((flag & FWRITE) && sc->sc_lastiow)) {
		htcommand(dev, HT_WEOF, 1);
		htcommand(dev, HT_WEOF, 1);
		htcommand(dev, HT_SREV, 1);
	}
	if ((minor(dev) & H_NOREWIND) == 0)
		htcommand(dev, HT_REW, 1);
	sc->sc_openf = 0;
}

/*ARGSUSED*/
htcommand(dev, com, count)
u_short	count;
dev_t	dev;
{
	register s;
	register struct	buf *bp;

	bp = &chtbuf;
	s = splbio();
	while(bp->b_flags & B_BUSY) {
		/*
		 * This special check is because B_BUSY never
		 * gets cleared in the non-waiting rewind case.
		 */
		if (bp->b_repcnt == 0 && (bp->b_flags & B_DONE))
			break;
		bp->b_flags |= B_WANTED;
		sleep((caddr_t) bp, PRIBIO);
	}
	bp->b_flags = B_BUSY | B_READ;
	splx(s);
	bp->b_dev = dev;
	if (com == HT_SFORW || com == HT_SREV)
		bp->b_repcnt = count;
	bp->b_command = com;
	bp->b_blkno = (daddr_t) 0;
	htstrategy(bp);
	/*
	 * In case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return(0);
	iowait(bp);
	if(bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= B_ERROR;
	return (bp->b_resid);
}

htstrategy(bp)
register struct	buf *bp;
{
	register int s;
	register struct softc *sc = &tu_softc[TUUNIT(bp->b_dev)];

	if (rh70 == 0)
		mapalloc(bp);
	if (bp->b_flags & B_PHYS) {
		sc->sc_blkno = sc->sc_nxrec = dbtofsb(bp->b_blkno);
		sc->sc_nxrec++;
	}
	bp->av_forw = NULL;
	s = splbio();
	if (httab.b_actf == NULL)
		httab.b_actf = bp;
	else
		httab.b_actl->av_forw = bp;
	httab.b_actl = bp;
	if (httab.b_active == 0)
		htstart();
	splx(s);
}

htstart()
{
	register struct buf *bp;
	register den;
	daddr_t	blkno;
	register struct softc *sc;

    loop:
	if ((bp = httab.b_actf) == NULL)
		return;
	sc = &tu_softc[TUUNIT(bp->b_dev)];
	sc->sc_erreg = HTADDR->hter;
	sc->sc_fsreg = HTADDR->htfs;
	sc->sc_resid = HTADDR->htfc;
	HTADDR->htcs2 = 0;	/* controller 0 - do we need this? */
	if ((HTADDR->httc & 03777) != sc->sc_dens)
		HTADDR->httc = sc->sc_dens;
	sc->sc_lastiow = 0;
	if (sc->sc_openf < 0 || HTADDR->htcs2 & HTCS2_NEF || !(HTADDR->htfs & HTFS_MOL))
		goto abort;
	if (bp == &chtbuf) {
		if (bp->b_command == HT_SENSE) {
			bp->b_resid = HTADDR->htfs;
			goto next;
		}
		httab.b_active = SCOM;
		HTADDR->htfc = 0;
		HTADDR->htcs1 = bp->b_command | HT_IE | HT_GO;
		return;
	}
	if (dbtofsb(bp->b_blkno) > sc->sc_nxrec)
		goto abort;
	if (dbtofsb(bp->b_blkno) == sc->sc_nxrec && bp->b_flags & B_READ) {
		/*
		 * Reading at end of file returns 0 bytes.
		 * Buffer will be cleared (if written) in rwip.
		*/
		bp->b_resid = bp->b_bcount;
		goto next;
	}
	if ((bp->b_flags & B_READ) == 0)
		/*
		 * Writing sets EOF
		*/
		sc->sc_nxrec = dbtofsb(bp->b_blkno) + 1;
	if ((blkno = sc->sc_blkno) == dbtofsb(bp->b_blkno)) {
		httab.b_active = SIO;
		HTADDR->htba = bp->b_un.b_addr;
		if (rh70)
			HTADDR->htbae = bp->b_xmem;
		HTADDR->htfc = -bp->b_bcount;
		HTADDR->htwc = -(bp->b_bcount >> 1);
		den = ((bp->b_xmem & 3) << 8) | HT_IE | HT_GO;
		if(bp->b_flags & B_READ)
			den |= HT_RCOM;
		else {
			if(HTADDR->htfs & HTFS_EOT) {
				bp->b_resid = bp->b_bcount;
				bp->b_error = ENXIO;
				httab.b_active = 0;
				goto next;
			}
			den |= HT_WCOM;
		}
		HTADDR->htcs1 = den;
	} else {
		if (blkno < dbtofsb(bp->b_blkno)) {
			httab.b_active = SSFOR;
			HTADDR->htfc = blkno - dbtofsb(bp->b_blkno);
			HTADDR->htcs1 = HT_SFORW | HT_IE | HT_GO;
		} else {
			httab.b_active = SSREV;
			HTADDR->htfc = dbtofsb(bp->b_blkno) - blkno;
			HTADDR->htcs1 = HT_SREV | HT_IE | HT_GO;
		}
	}
	return;

    abort:
	bp->b_flags |= B_ERROR;

    next:
	httab.b_actf = bp->av_forw;
	iodone(bp);
	goto loop;
}

htintr()
{
	register struct buf *bp;
	register state;
	int err, htunit;
	register struct softc *sc;

	if ((bp = httab.b_actf) == NULL)
		return;
	htunit = TUUNIT(bp->b_dev);
	sc = &tu_softc[htunit];
	sc->sc_erreg = HTADDR->hter;
	sc->sc_fsreg = HTADDR->htfs;
	sc->sc_resid = HTADDR->htfc;
	if ((bp->b_flags & B_READ) == 0)
		sc->sc_lastiow = 1;
	state = httab.b_active;
	httab.b_active = 0;
	if (HTADDR->htcs1 & HT_TRE) {
		err = HTADDR->hter;
		if (HTADDR->htcs2 & HTCS2_ERR || (err & HTER_HARD))
			state = 0;
		if (bp->b_flags & B_PHYS)
			err &= ~HTER_FCE;
		if ((bp->b_flags & B_READ) && (HTADDR->htfs & HTFS_PES))
			err &= ~(HTER_CSITM | HTER_CORCRC);
		if ((HTADDR->htfs & HTFS_MOL) == 0) {
			if(sc->sc_openf)
				sc->sc_openf = -1;
		}
		else
			if (HTADDR->htfs & HTFS_TM) {
				HTADDR->htwc = -(bp->b_bcount >> 1);
				sc->sc_nxrec = dbtofsb(bp->b_blkno);
				state = SOK;
			}
			else
				if (state && err == 0)
					state = SOK;
		if (httab.b_errcnt > 4)
			printf("tu%d: hard error bn %D er=%b ds=%b\n",
			       htunit, bp->b_blkno,
			       sc->sc_erreg, HTER_BITS,
			       sc->sc_fsreg, HTFS_BITS);
		htinit();
		if (state == SIO && ++httab.b_errcnt < 10) {
			httab.b_active = SRETRY;
			sc->sc_blkno++;
			HTADDR->htfc = -1;
			HTADDR->htcs1 = HT_SREV | HT_IE | HT_GO;
			return;
		}
		if (state != SOK) {
			bp->b_flags |= B_ERROR;
			state = SIO;
		}
	} else
		if (HTADDR->htcs1 & HT_SC)
			if(HTADDR->htfs & HTFS_ERR)
				htinit();

	switch (state) {
		case SIO:
		case SOK:
			sc->sc_blkno++;

		case SCOM:
			httab.b_errcnt = 0;
			httab.b_actf = bp->av_forw;
			iodone(bp);
			bp->b_resid = -(HTADDR->htwc << 1);
			break;

		case SRETRY:
			if((bp->b_flags & B_READ) == 0) {
				httab.b_active = SSFOR;
				HTADDR->htcs1 = HT_ERASE | HT_IE | HT_GO;
				return;
			}

		case SSFOR:
		case SSREV:
			if(HTADDR->htfs & HTFS_TM) {
				if(state == SSREV) {
					sc->sc_nxrec = dbtofsb(bp->b_blkno) - HTADDR->htfc;
					sc->sc_blkno = sc->sc_nxrec;
				} else
					{
					sc->sc_nxrec = dbtofsb(bp->b_blkno) + HTADDR->htfc - 1;
					sc->sc_blkno = sc->sc_nxrec + 1;
				}
			} else
				sc->sc_blkno = dbtofsb(bp->b_blkno);
			break;

		default:
			return;
	}
	htstart();
}

htinit()
{
	register ocs2;
	register omttc;
	
	omttc = HTADDR->httc & 03777;	/* preserve old slave select, dens, format */
	ocs2 = HTADDR->htcs2 & 07;	/* preserve old unit */

	HTADDR->htcs2 = HTCS2_CLR;
	HTADDR->htcs2 = ocs2;
	HTADDR->httc = omttc;
	HTADDR->htcs1 = HT_DCLR | HT_GO;
}

/*ARGSUSED*/
htioctl(dev, cmd, data, flag)
	dev_t dev;
	u_int cmd;
	caddr_t data;
	int flag;
{
	register struct buf *bp = &chtbuf;
	register struct softc *sc = &tu_softc[minor(dev)&07];
	register callcount;
	int	fcount;
	struct	mtop *mtop;
	struct	mtget *mtget;
	/* we depend on the values and order of the MT codes here */
	static	htops[] = {HT_WEOF, HT_SFORW, HT_SREV, HT_SFORW,
		HT_SREV, HT_REW, HT_REWOFFL, HT_SENSE};

	switch (cmd) {
	case MTIOCTOP:
		mtop = (struct mtop *)data;
		switch (mtop->mt_op) {
		case MTWEOF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;
		case MTFSF: case MTBSF:
			callcount = mtop->mt_count;
			fcount = INF;
			break;
		case MTFSR: case MTBSR:
			callcount = 1;
			fcount = mtop->mt_count;
			break;
		case MTREW: case MTOFFL: case MTNOP:
			callcount = 1;
			fcount = 1;
			break;
		default:
			return (ENXIO);
		}
		if (callcount <= 0 || fcount <= 0)
			return (EINVAL);
		while (--callcount >= 0) {
			htcommand(dev, htops[mtop->mt_op], fcount);
			if ((mtop->mt_op == MTFSR || mtop->mt_op == MTBSR) &&
			    bp->b_resid)
				return (EIO);
			if ((bp->b_flags&B_ERROR) || sc->sc_fsreg&HTFS_BOT)
				break;
		}
		return (geterror(bp));
	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = sc->sc_fsreg;
		mtget->mt_erreg = sc->sc_erreg;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISHT;
		break;
	default:
		return (ENXIO);
	}
	return (0);
}
#endif NHT

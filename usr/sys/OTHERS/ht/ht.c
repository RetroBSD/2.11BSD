/*
 * TJU77/TWU77/TJE16/TWE16 tape driver
 */

/*
 *	SCCS id	@(#)ht.c	2.2 (2.11BSD GTE) 1/2/93
 *	Eric Haag's Driver
 *	I received this from Eric when I first got 2.9 up
 *	(Oct 84) and have recently sent it to Steve Malone
 *	both Steve & I have Massbus (RH70) tapes
 */

#include "ht.h"
#if	NHT > 0
#include "param.h"
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/htreg.h>
#ifdef	HT_IOCTL
#include <sys/mtio.h>
#endif

struct	buf	httab;
struct	buf	chtbuf;

struct	htdevice	*HTADDR;

#define	INF	32760

struct	softc	{
	char	sc_openf;
	char	sc_lastiow;
	daddr_t	sc_blkno;
	daddr_t	sc_nxrec;
	u_short	sc_erreg;
	u_short	sc_fsreg;
#ifdef	HT_IOCTL
	short	sc_resid;
#endif
}	tu_softc[NHT];


#define	SIO	1
#define	SSFOR	2
#define	SSREV	3
#define	SRETRY	4
#define	SCOM	5
#define	SOK	6

#define	TUUNIT(dev)	(minor(dev) & 077)

/* bits in minor device */
#define	H_800BPI	0100
#define	H_NOREWIND	0200

htattach(addr, unit)
register struct htdevice *addr;
{
	if (unit >= NHT)
		return(0);
	if ((addr != (struct htdevice *) NULL) && (fioword(addr) != -1)) {
		HTADDR = addr;
		if (fioword(&(addr->htbae)) != -1)
			httab.b_flags |= B_RH70;
		return(1);
	}
	HTADDR = (struct hpdevice *) NULL;
	return(0);
}

htopen(dev, flag)
dev_t	dev;
{
	register ds;
	register htunit = TUUNIT(dev);
	register struct tu_softc *sc = &tu_softc[htunit];

	httab.b_flags |= B_TAPE;
	if (HTADDR == (struct htdevice *) NULL || htunit >= NHT) {
		u.u_error = ENXIO;
		return;
	}
	else
		if (sc->sc_openf) {
			u.u_error = EBUSY;
			return;
		}
	sc->sc_blkno = (daddr_t) 0;
	sc->sc_nxrec = (daddr_t) INF;
	sc->sc_lastiow = 0;
	ds = htcommand(dev, HT_SENSE, 1);
	if ((ds & HTFS_MOL) == 0) {
		uprintf("tu%d: not online\n", htunit);
		u.u_error = EIO;
		return;
	}
	if ((flag & FWRITE) && (ds & HTFS_WRL)) {
		uprintf("tu%d: no write ring\n", htunit);
		u.u_error = EIO;
		return;
	}
	if (u.u_error == 0)
		sc->sc_openf++;
}

htclose(dev, flag)
dev_t	dev;
{
	register struct tu_softc *sc = &tu_softc[TUUNIT(dev)];

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
unsigned	count;
dev_t	dev;
{
	register s;
	register struct	buf *bp;

	bp = &chtbuf;
	s = spl5();
	while(bp->b_flags & B_BUSY) {
#ifndef	SLU70
		/*
		 * This special check is because B_BUSY never
		 * gets cleared in the non-waiting rewind case.
		 */
		if (bp->b_repcnt == 0 && (bp->b_flags & B_DONE))
			break;
#endif
		bp->b_flags |= B_WANTED;
		sleep((caddr_t) bp, PRIBIO);
	}
	bp->b_flags = B_BUSY | B_READ;
	splx(s);
	bp->b_dev = dev;
#ifdef	HT_IOCTL
	if (com == HT_SFORW || com == HT_SREV)
		bp->b_repcnt = count;
#endif
	bp->b_command = com;
	bp->b_blkno = (daddr_t) 0;
	htstrategy(bp);
#ifndef	SLU70
	/*
	 * In case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return(0);
#endif
	iowait(bp);
	if(bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= B_ERROR;
	return (bp->b_resid);
}

htstrategy(bp)
register struct	buf *bp;
{
	int s;
	register daddr_t *p;
	register struct tu_softc *sc = &tu_softc[TUUNIT(bp->b_dev)];

/* This is almost certainly not in the right place and more work needs
 * to be done in htstart().  See /sys/pdpuba/ht.c
*/
	if (bp->b_flags & B_PHYS) {
		sc->sc_blkno = sc->sc_nxrec = dbtofsb(bp->b_blkno);
		sc->sc_nxrec++;
	}

	if(bp != &chtbuf) {
		if ((httab.b_flags & B_RH70) == 0)
			mapalloc(bp);
		p = &sc->sc_nxrec;
		if(dbtofsb(bp->b_blkno) > *p) {
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
			iodone(bp);
			return;
		}
		if(dbtofsb(bp->b_blkno) == *p && bp->b_flags & B_READ) {
			bp->b_resid = bp->b_bcount;
			iodone(bp);
			return;
		}
		if ((bp->b_flags & B_READ) == 0) {
			*p = dbtofsb(bp->b_blkno) + 1;
			sc->sc_lastiow = 1;
		}
	}
	bp->av_forw = NULL;
	s = spl5();
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
	int htunit;
	daddr_t	blkno;
	register struct softc *sc;

    loop:
	if ((bp = httab.b_actf) == NULL)
		return;
	htunit = minor(bp->b_dev) & 0177;
	sc = &tu_softc[TUUNIT(bp->b_dev)];
	sc->sc_erreg = HTADDR->hter;
	sc->sc_fsreg = HTADDR->htfs;
#ifdef	HT_IOCTL
	sc->sc_resid = HTADDR->htfc;
#endif
	HTADDR->htcs2 = ((htunit >> 3) & 07);
	den = HTTC_1600BPI | HTTC_PDP11 | (htunit & 07);
	if (htunit & H_800BPI)
		den = HTTC_800BPI | HTTC_PDP11 | (htunit & 07);
	if ((HTADDR->httc & 03777) != den)
		HTADDR->httc = den;
	if (HTADDR->htcs2 & HTCS2_NEF || (HTADDR->htfs & HTFS_MOL) == 0)
		goto abort;
	htunit &= 077;
	blkno = sc->sc_blkno;
	if (bp == &chtbuf) {
		if (bp->b_command == HT_SENSE) {
			bp->b_resid = HTADDR->htfs;
			goto next;
		}
		httab.b_active = SCOM;
#ifdef	HT_IOCTL
		HTADDR->htfc = -bp->b_bcount;
#else
		HTADDR->htfc = 0;
#endif
		HTADDR->htcs1 = bp->b_command | HT_IE | HT_GO;
		return;
	}
	if (sc->sc_openf < 0 || dbtofsb(bp->b_blkno) > sc->sc_nxrec)
		goto abort;
	if (blkno == dbtofsb(bp->b_blkno)) {
		httab.b_active = SIO;
		HTADDR->htba = bp->b_un.b_addr;
		if(httab.b_flags & B_RH70)
			HTADDR->htbae = bp->b_xmem;
		HTADDR->htfc = -bp->b_bcount;
		HTADDR->htwc = -(bp->b_bcount >> 1);
		den = ((bp->b_xmem & 3) << 8) | HT_IE | HT_GO;
		if(bp->b_flags & B_READ)
			den |= HT_RCOM;
		else {
			if(HTADDR->htfs & HTFS_EOT) {
				bp->b_resid = bp->b_bcount;
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
	state = httab.b_active;
	httab.b_active = 0;
	sc = &tu_softc[htunit];
	sc->sc_erreg = HTADDR->hter;
	sc->sc_fsreg = HTADDR->htfs;
#ifdef	HT_IOCTL
	sc->sc_resid = HTADDR->htfc;
#endif
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
		if (httab.b_errcnt > 6)
#ifdef	UCB_DEVERR
			printf("tu%d: hard error bn %D er=%b ds=%b\n",
			       htunit, bp->b_blkno,
			       sc->sc_erreg, HTER_BITS,
			       sc->sc_fsreg, HTFS_BITS);
#else
			deverror(bp, sc->sc_erreg, sc->sc_fsreg);
#endif
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

#ifdef	HT_IOCTL

/*ARGSUSED*/
htioctl(dev, cmd, addr, flag)
dev_t	dev;
caddr_t	addr;
{
	register struct buf *bp = &chtbuf;
	register struct softc *sc = &tu_softc[TUUNIT(dev)];
	register callcount;
	int	fcount;
	struct	mtop mtop;
	struct	mtget mtget;
	/* we depend on the values and order of the MT codes here */
	static	htops[] = {HT_WEOF, HT_SFORW, HT_SREV, HT_SFORW,
		HT_SREV, HT_REW, HT_REWOFFL, HT_SENSE};

	switch (cmd)	{

		case MTIOCTOP:
			if (copyin(addr, (caddr_t) &mtop, sizeof(mtop))) {
				u.u_error = EFAULT;
				return;
			}
			switch(mtop.mt_op) {
				case MTWEOF:
					callcount = mtop.mt_count;
					fcount = 1;
					break;
				case MTFSF:
				case MTBSF:
					callcount = mtop.mt_count;
					fcount = INF;
					break;
				case MTFSR:
				case MTBSR:
					callcount = 1;
					fcount = mtop.mt_count;
					break;
				case MTREW:
				case MTOFFL:
				case MTNOP:
					callcount = 1;
					fcount = 1;
					break;
				default:
					u.u_error = ENXIO;
					return;
			}
			if (callcount <= 0 || fcount <= 0) {
				u.u_error = ENXIO;
				return;
			}
			while (--callcount >= 0) {
				htcommand(dev, htops[mtop.mt_op], fcount);
				if ((mtop.mt_op == MTFSR || mtop.mt_op == MTBSR)
				    && bp->b_resid) {
					u.u_error = EIO;
					break;
				}
				if ((bp->b_flags & B_ERROR)
				    || sc->sc_fsreg & HTFS_BOT)
					break;
			}
			geterror(bp);
			return;
		case MTIOCGET:
			mtget.mt_erreg = sc->sc_erreg;
			mtget.mt_dsreg = sc->sc_fsreg;
			mtget.mt_resid = sc->sc_resid;
			mtget.mt_type = MT_ISHT;
			if (copyout((caddr_t) &mtget, addr, sizeof(mtget)))
				u.u_error = EFAULT;
			return;
		default:
			u.u_error = ENXIO;
	}
}
#endif	HT_IOCTL
#endif	NHT



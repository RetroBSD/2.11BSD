/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ts.c	2.3 (2.11BSD GTE) 11/26/94
 */

/*
 *	TS11 tape driver
 */

#include "ts.h"
#if	NTS > 0
#include "param.h"
#include "buf.h"
#include "conf.h"
#include "file.h"
#include "user.h"
#include "ioctl.h"
#include "fs.h"
#include "tsreg.h"
#include "mtio.h"
#include "map.h"
#include "uba.h"

/*
 * Software state per tape transport:
 *
 * 1. A tape drive is a unique-open device: we refuse opens when it is already
 * 2. We keep track of the current position on a block tape and seek
 *    before operations by forward/back spacing if necessary.
 * 3. We remember if the last operation was a write on a tape, so if a tape
 *    is open read write and the last thing done is a write we can
 *    write a standard end of tape mark (two eofs).
 * 4. We remember the status registers after the last command, using
 *    them internally and returning them to the SENSE ioctl.
 */

struct	ts_softc {
	struct	tsdevice *sc_addr; /* CSR of controller */
	char	sc_openf;	/* lock against multiple opens */
	char	sc_lastiow;	/* last op was a write */
	short	sc_resid;	/* copy of last bc */
	daddr_t	sc_blkno;	/* block number, for block device tape */
	daddr_t	sc_nxrec;	/* position of end of tape, if known */
	struct	ts_cmd	sc_cmd;	/* the command packet */
	struct	ts_char	sc_char;/* status packet, for returned status */
	struct	ts_sts	sc_sts; /* characteristics packet */
	u_short	sc_uba;		/* Unibus addr of cmd pkt for tsdb */
	ubadr_t	sc_uadr;	/* actual unibus address */
	struct	tty *sc_ttyp;	/* record user's tty for errors */
} *ts_softc[NTS];

static	ubadr_t	TS_ubadr;

struct	buf	tstab[NTS];

/*
 * There is a ctsbuf per tape controller.
 * It is used as the token to pass to the internal routines
 * to execute tape ioctls.
 * In particular, when the tape is rewinding on close we release
 * the user process but any further attempts to use the tape drive
 * before the rewind completes will hang waiting for ctsbuf.
 */
struct	buf	ctsbuf[NTS];

#define	INF		((daddr_t) ((u_short) 65535))

/* bits in minor device */
#define	TSUNIT(dev)	(minor(dev)&03)		/* not used */
#define	TSCTLR(dev)	((minor(dev) >> 6) & 3)
#define	T_NOREWIND	04

/*
 * States for tstab.b_active, the state flag.
 * This is used to sequence control in the driver.
 */
#define SSEEK		1	/* seeking */
#define	SIO		2	/* doing seq. i/o */
#define	SCOM		3	/* sending a control command */
#define	SREW		4	/* sending a drive rewind */

u_short softspace[NTS][(sizeof(struct ts_softc)/2) + 1];

static	char	*tsmsg1 = "ts%d: not online\n";
static	char	*tsmsg2 = "ts%d: no write ring\n";

tsattach(addr, unit)
struct tsdevice *addr;
register int unit;
{
	register u_short sp = (u_short)softspace[unit];
	register struct ts_softc *sc;
	struct buf tbuf;

	if (unit >= NTS)
		return(0);
	/*
	 * We want space for an array of NTS ts_softc structures,
	 * where the sc_cmd field of each is long-aligned, i.e. the
	 * core address is a 4-byte multiple.  The compiler only
	 * guarantees word alignment.  We reserve and extra 3 bytes
	 * so that we can slide the array down by 2 if the compiler
	 * gets it wrong.  Only allocate 1 UMR to map all of the
	 * communication area instead of a UMR per controller.
	 *
	 * On a UNIBUS system the ts_softc structure is aligned so
	 * that the UNIBUS virtual address of sc_cmd falls on a 4
	 * byte boundary - the physical address may be on a 2 byte bound.
	 *
	 * On non-UNIBUS systems the ts_softc structure is aligned so
	 * that sc_cmd falls on a physical 4 byte boundary.
	 */
	sc = (struct ts_softc *)sp;
	if (((u_short)&sc->sc_cmd  - (ubmap ? (u_short)softspace : 0)) & 3)
		sp += 2;
	if (ubmap && TS_ubadr == 0) {
		tbuf.b_xmem = 0;	/* won't work above 64k any way */
		tbuf.b_un.b_addr = (caddr_t) softspace;
		tbuf.b_flags = B_PHYS;
		tbuf.b_bcount = sizeof (softspace);
		mapalloc(&tbuf);
		TS_ubadr = ((long)((unsigned)tbuf.b_xmem)) << 16
				| ((long)((unsigned)tbuf.b_un.b_addr));
	}
	sc = ts_softc[unit] = (struct ts_softc *) sp;
	sc->sc_addr = addr;
	sc->sc_uadr = TS_ubadr + ((u_short)&sc->sc_cmd - 
				(ubmap ? (u_short)softspace : 0));
	sc->sc_uba = loint(sc->sc_uadr) 
		   | hiint(sc->sc_uadr);	/* register format */
	return(1);
}

/*
 * Open the device.  Tapes are unique open
 * devices so we refuse if it is already open.
 * We also check that a tape is available and
 * don't block waiting here:  if you want to wait
 * for a tape you should timeout in user code.
 */
tsopen(dev, flag)
dev_t	dev;
int	flag;
{
	register ts11 = TSCTLR(dev);
	register struct ts_softc *sc = ts_softc[ts11];

	if (ts11 >= NTS || !sc || sc->sc_openf)
		return(ENXIO);
	if (tsinit(ts11)) {
		printf("ts%d: init failure tssr=%b\n",
			ts11, sc->sc_addr->tssr, TSSR_BITS);
		return(ENXIO);
	}
	tstab[ts11].b_flags |= B_TAPE;
	tscommand(dev, TS_SENSE, 1);
	if ((sc->sc_sts.s_xs0 & TS_ONL) == 0) {
		uprintf(tsmsg1, ts11);
		return(EIO);
	}
	if ((flag & (FREAD | FWRITE)) == FWRITE
	    && (sc->sc_sts.s_xs0 & TS_WLK)) {
		uprintf(tsmsg2, ts11);
		return(EIO);
	}
	sc->sc_openf = 1;
	sc->sc_blkno = (daddr_t) 0;
	sc->sc_nxrec = INF;
	sc->sc_lastiow = 0;
	sc->sc_ttyp = u.u_ttyp;
	return(0);
}

/*
 * Close tape device.
 *
 * If tape was open for writing or last operation was
 * a write, then write two EOF's and backspace over the last one.
 * Unless his is a non-rewinding special file, rewind the tape.
 * Make the tape available to others.
 */
tsclose(dev, flag)
register dev_t	dev;
register int flag;
{
	register struct ts_softc *sc = ts_softc[TSCTLR(dev)];

	if (flag == FWRITE || ((flag & FWRITE) && sc->sc_lastiow)) {
		tscommand(dev, TS_WEOF, 1);
		tscommand(dev, TS_WEOF, 1);
		tscommand(dev, TS_SREV, 1);
	}
	if ((minor(dev) & T_NOREWIND) == 0 )
		/*
		 * 0 count means don't hang waiting for rewind complete.
		 * Rather ctsbuf stays busy until the operation completes
		 * preventing further opens from completing by
		 * preventing a TS_SENSE from completing.
		 */
		tscommand(dev, TS_REW, 0);
	sc->sc_openf = 0;
}

/*
 * Execute a command on the tape drive
 * a specified number of times.
 */
tscommand(dev, com, count)
	dev_t	dev;
register u_short count;
{
	register int s;
	register struct buf *bp;

	bp = &ctsbuf[TSCTLR(dev)];
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
	bp->b_repcnt = count;
	bp->b_command = com;
	bp->b_blkno = (daddr_t) 0;
	tsstrategy(bp);
	/*
	 * In case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return;
	iowait(bp);
	if(bp->b_flags & B_WANTED)
		wakeup((caddr_t) bp);
	bp->b_flags &= B_ERROR;
}

/*
 * Queue a tape operation.
 */
tsstrategy(bp)
register struct buf *bp;
{
	register int s;
	int ts11 = TSCTLR(bp->b_dev);
	struct ts_softc *sc = ts_softc[ts11];
	register struct buf *dp = &tstab[ts11];

	if (bp->b_flags & B_PHYS) {	/* if RAW I/O call */
		mapalloc(bp);
		sc->sc_blkno = sc->sc_nxrec = dbtofsb(bp->b_blkno);
		sc->sc_nxrec++;
	}
	bp->av_forw = NULL;
	s = splbio();
	if (dp->b_actf == NULL)
		dp->b_actf = bp;
	else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	/*
	 * If the controller is not busy, get
	 * it going.
	 */
	if (dp->b_active == 0)
		tsstart(ts11);
	splx(s);
}

/*
 * Start activity on a ts controller.
 */
tsstart(ts11)
	int ts11;
{
	daddr_t	blkno;
	int	cmd;
	register struct ts_softc *sc;
	register struct ts_cmd *tc;
	register struct buf *bp;
	struct buf *um = &tstab[ts11];

	/*
	 * Start the controller if there is something for it to do.
	 */
loop:
	if ((bp = um->b_actf) == NULL)
		return;
	sc = ts_softc[ts11];
	tc = &sc->sc_cmd;
	/*
	 * Default is that last command was NOT a write command;
	 * if we do a write command we will notice this in tsintr().
	 */
	sc->sc_lastiow = 0;
	if (sc->sc_openf < 0 || (sc->sc_addr->tssr & TS_OFL)) {
		/*
		 * Have had a hard error on a non-raw tape
		 * or the tape unit is now unavailable
		 * (e.g. taken off line).
		 */
		bp->b_flags |= B_ERROR;
		goto next;
	}
	if (bp == &ctsbuf[ts11]) {
		/*
		 * Execute control operation with the specified count.
		 */
		um->b_active = bp->b_command == TS_REW ?  SREW : SCOM;
		goto dobpcmd;
	}
	/*
	 * The following checks handle boundary cases for operation
	 * on non-raw tapes.  On raw tapes the initialization of
	 * sc->sc_nxrec by tsphys causes them to be skipped normally
	 * (except in the case of retries).
	 */
	if (dbtofsb(bp->b_blkno) > sc->sc_nxrec) {
		/*
		 * Can't read past known end-of-file.
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		goto next;
	}
	if (dbtofsb(bp->b_blkno) == sc->sc_nxrec && bp->b_flags & B_READ) {
		/*
		 * Reading at end of file returns 0 bytes.
		 * Buffer will be cleared (if written) in rwip.
		 */
		bp->b_resid = bp->b_bcount;
		goto next;
	}
	if((bp->b_flags & B_READ) == 0)
		/*
		 * Writing sets EOF
		 */
		sc->sc_nxrec = dbtofsb(bp->b_blkno) + 1;
	/*
	 * If the data transfer command is in the correct place,
	 * set up all registers and do the transfer.
	 */
	if ((blkno = sc->sc_blkno) == dbtofsb(bp->b_blkno)) {
		um->b_active = SIO;
		tc->c_loba = (u_short)bp->b_un.b_addr;
		tc->c_hiba = bp->b_xmem;
		tc->c_size = bp->b_bcount;
		if ((bp->b_flags & B_READ) == 0)
			cmd = TS_WCOM;
		else
			cmd = TS_RCOM;
		if (um->b_errcnt)
			cmd |= TS_RETRY;
		tc->c_cmd = TS_ACK | TS_CVC | TS_IE | cmd;
		sc->sc_addr->tsdb = sc->sc_uba;
		return;
	}
	/*
	 * Tape positioned incorrectly;
	 * set to seek forward or backward to the correct spot.
	 * This happens for raw tapes only on error retries.
	 */
	um->b_active = SSEEK;
	if (blkno < dbtofsb(bp->b_blkno)) {
		bp->b_command = TS_SFORW;
		bp->b_repcnt = dbtofsb(bp->b_blkno) - blkno;
	} else
		{
		bp->b_command = TS_SREV;
		bp->b_repcnt = blkno - dbtofsb(bp->b_blkno);
	}
dobpcmd:
	tc->c_repcnt = bp->b_repcnt;
	/*
	 * Do the command in bp.
	 */
	tc->c_cmd = TS_ACK | TS_CVC | TS_IE | bp->b_command;
	sc->sc_addr->tsdb = sc->sc_uba;
	return;

next:
	/*
	 * Done with this operation due to error or
	 * the fact that it doesn't do anything.
	 * Dequeue the transfer and continue processing this slave.
	 */
	um->b_errcnt = 0;
	um->b_actf = bp->av_forw;
	iodone(bp);
	goto loop;
}

/*
 * TS interrupt routine
 */
tsintr(dev)
	int dev;
{
	int state;
	register struct buf *bp, *um = &tstab[dev];
	register struct ts_softc *sc = ts_softc[dev];

	if ((bp = um->b_actf) == NULL)
		return;

	/*
	 * If last command was a rewind, and tape is still
	 * rewinding, wait for the rewind complete interrupt.
	 *
	 * SHOULD NEVER GET AN INTERRUPT IN THIS STATE, but it
	 * happens when a rewind completes.
	 */
	if (um->b_active == SREW) {
		um->b_active = SCOM;
		if ((sc->sc_addr->tssr & TS_SSR) == 0)
			return;
	}
	/*
	 * An operation completed... record status
	 */
	if ((bp->b_flags & B_READ) == 0)
		sc->sc_lastiow = 1;
	state = um->b_active;
	um->b_active = 0;

	/*
	 * Check for errors.
	 */
	if (sc->sc_addr->tssr & TS_SC) {
		switch (sc->sc_addr->tssr & TS_TC) {
			case TS_UNREC:	/* unrecoverable */
			case TS_FATAL:	/* fatal error */
			case TS_ATTN:	/* attention (shouldn't happen) */
			case TS_RECNM:	/* recoverable, no motion */
				break;

			case TS_SUCC:	/* successful termination */
				goto ignoreerr;
				/*NOTREACHED*/
			
			case TS_ALERT:	/* tape status alert */
				/*
				 * If we hit the end of the tape file,
				 * update our position.
				 */
				if (sc->sc_sts.s_xs0 & (TS_TMK | TS_EOT)) {
					tsseteof(bp);	/* set blkno and nxrec */
					state = SCOM;	/* force completion */
					/*
					 * Stuff bc so it will be unstuffed
					 * correctly later to get resid.
					 */
					sc->sc_sts.s_rbpcr = bp->b_bcount;
					goto opdone;
					/*NOTREACHED*/
				}
				/*
				 * If we were reading raw tape and the record
				 * was too long or too short, then we don't
				 * consider this an error.
				 */
				if ((bp->b_flags & B_PHYS) && (bp->b_flags & B_READ)
				    && sc->sc_sts.s_xs0 & (TS_RLS | TS_RLL))
					goto ignoreerr;
					/*NOTREACHED*/

			case TS_RECOV:	/* recoverable, tape moved */
				/*
				 * If this was an i/o operation,
				 * retry up to 8 times.
				 */
				if (state == SIO) {
					if (++(um->b_errcnt) < 7)
						goto opcont;
					else
						sc->sc_blkno++;
				} else
					{
					/*
					 * Non-i/o errors on non-raw tape
					 * cause it to close.
					 */
					if (sc->sc_openf > 0 && !(bp->b_flags & B_PHYS))
						sc->sc_openf = -1;
				}
				break;

			case TS_REJECT:
				if (state == SIO && sc->sc_sts.s_xs0 & TS_WLE)
					tprintf(sc->sc_ttyp, tsmsg2, dev);
				if ((sc->sc_sts.s_xs0 & TS_ONL) == 0)
					tprintf(sc->sc_ttyp, tsmsg1, dev);
				break;
		}
		/*
		 * Couldn't recover error.
		 */
		tprintf(sc->sc_ttyp,"ts%d: hard error bn%D xs0=%b", dev,
		     bp->b_blkno, sc->sc_sts.s_xs0, TSXS0_BITS);
		if (sc->sc_sts.s_xs1)
			tprintf(sc->sc_ttyp," xs1=%b", sc->sc_sts.s_xs1, TSXS1_BITS);
		if (sc->sc_sts.s_xs2)
			tprintf(sc->sc_ttyp," xs2=%b", sc->sc_sts.s_xs2, TSXS2_BITS);
		if (sc->sc_sts.s_xs3)
			tprintf(sc->sc_ttyp," xs3=%b", sc->sc_sts.s_xs3, TSXS3_BITS);
		tprintf(sc->sc_ttyp,"\n");
		bp->b_flags |= B_ERROR;
		goto opdone;
		/*NOTREACHED*/
	}
	/*
	 * Advance tape control finite state machine.
	 */
ignoreerr:
	switch (state) {
		case SIO:
			/*
			 * Read/write increments tape block number.
			 */
			sc->sc_blkno++;
			goto opdone;
			/*NOTREACHED*/

		case SCOM:
			/*
			 * For forward/backward space record
			 * update current position.
			 */
			if (bp == &ctsbuf[dev])
				switch (bp->b_command) {
					case TS_SFORW:
						sc->sc_blkno += bp->b_repcnt;
						break;

					case TS_SREV:
						sc->sc_blkno -= bp->b_repcnt;
						break;
				}
			goto opdone;
			/*NOTREACHED*/

		case SSEEK:
			sc->sc_blkno = dbtofsb(bp->b_blkno);
			goto opcont;
			/*NOTREACHED*/
		
		default:
			panic("tsintr");
			/*NOTREACHED*/
	}

opdone:
	/*
	 * Reset error count and remove
	 * from device queue.
	 */
	um->b_errcnt = 0;
	um->b_actf = bp->av_forw;
	bp->b_resid = sc->sc_sts.s_rbpcr;
	iodone(bp);
	if (um->b_actf == NULL)
		return;
opcont:
	tsstart(dev);
}

tsseteof(bp)
register struct buf *bp;
{
	register int ts11 = TSCTLR(bp->b_dev);
	register struct ts_softc *sc = ts_softc[ts11];

	if (bp == &ctsbuf[ts11]) {
		if (sc->sc_blkno > dbtofsb(bp->b_blkno)) {
			/* reversing */
			sc->sc_nxrec = dbtofsb(bp->b_blkno) - sc->sc_sts.s_rbpcr;
			sc->sc_blkno = sc->sc_nxrec;
		}
		else
			{
			/* spacing forward */
			sc->sc_blkno = dbtofsb(bp->b_blkno) + sc->sc_sts.s_rbpcr;
			sc->sc_nxrec = sc->sc_blkno - 1;
		}
		return;
	}
	/* eof on read */
	sc->sc_nxrec = dbtofsb(bp->b_blkno);
}

/*
 * Initialize the TS11.
 */
tsinit(ts11)
{
	register struct	ts_softc *sc = ts_softc[ts11];
	register struct ts_cmd *tcmd = &sc->sc_cmd;
	register struct ts_char *tchar = &sc->sc_char;
	int cnt;
	/*
	 * Now initialize the TS11 controller.
	 * Set the characteristics.
	 */
	if (sc->sc_addr->tssr & (TS_NBA | TS_OFL)) {
		tcmd->c_cmd = TS_ACK | TS_CVC | TS_INIT;
		sc->sc_addr->tsdb = sc->sc_uba;
		for (cnt = 0; cnt < 10000; cnt++) {
			if (sc->sc_addr->tssr & TS_SSR)
				break;
		}
		if (cnt >= 10000)
			return (1);
		tchar->char_bptr = (u_short)loint(sc->sc_uadr)+
			((u_short)&sc->sc_sts-(u_short)tcmd);
		tchar->char_bae = hiint(sc->sc_uadr);
		tchar->char_size = sizeof(struct ts_sts);
		tchar->char_mode = TS_ESS;
		tcmd->c_cmd = TS_ACK | TS_CVC | TS_SETCHR;
		tcmd->c_loba = (u_short)loint(sc->sc_uadr)+
			((u_short)tchar-(u_short)tcmd);
		tcmd->c_hiba = hiint(sc->sc_uadr);
		tcmd->c_size = sizeof(struct ts_char);
		sc->sc_addr->tsdb = sc->sc_uba;
		for (cnt = 0; cnt < 10000; cnt++) {
			if (sc->sc_addr->tssr & TS_SSR)
				break;
		}
		if (sc->sc_addr->tssr & TS_NBA) {
			printf("ts%d: set char. failure\n", ts11);
			return (1);
		}
	}
	return(0);
}

/*ARGSUSED*/
tsioctl(dev, cmd, data, flag)
	dev_t dev;
	u_int cmd;
	caddr_t data;
{
	int ts11 = TSCTLR(dev);
	register struct ts_softc *sc = ts_softc[ts11];
	register struct buf *bp = &ctsbuf[ts11];
	register callcount;
	u_short	fcount;
	struct	mtop *mtop;
	struct	mtget *mtget;
	/* we depend on the values and order of the MT codes here */
	static tsops[] =
	{TS_WEOF,TS_SFORWF,TS_SREVF,TS_SFORW,TS_SREV,TS_REW,TS_OFFL,TS_SENSE};

	switch (cmd) {

	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *)data;
		switch(mtop->mt_op) {
		case MTWEOF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;
		case MTFSF: case MTBSF:
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
			tscommand(dev, tsops[mtop->mt_op], fcount);
			if ((mtop->mt_op == MTFSR || mtop->mt_op == MTBSR) &&
			    bp->b_resid)
				return (EIO);
			if ((bp->b_flags&B_ERROR) || sc->sc_sts.s_xs0&TS_BOT)
				break;
		}
		return (geterror(bp));
	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = 0;
		mtget->mt_erreg = sc->sc_sts.s_xs0;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISTS;
		break;
	default:
		return (ENXIO);
	}
	return (0);
}
#endif NTS

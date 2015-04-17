
/**********************************************************************
 *   Copyright (c) Digital Equipment Corporation 1984, 1985, 1986.    *
 *   All Rights Reserved. 					      *
 *   Reference "/usr/src/COPYRIGHT" for applicable restrictions.      *
 **********************************************************************/

/*
 * ULTRIX-11 TK50/TU81 - TMSCP tape driver
 *
 * SCCSID: @(#)tk.c	3.18	4/21/86
 *
 * Chung-Wu Lee, Jan-10-86
 *
 *	enable CACHE/NOCACHE.
 *
 * Chung-Wu Lee, Dec-31-85
 *
 *	start supporting up tp 4 controllers per system.
 *
 * Chung-Wu Lee, Aug-09-85
 *
 *	add ioctl routine.
 *
 * Chung-Wu Lee, Jan-22-85
 */

#define	PANIC	1	/* panic on fatal controller error */
#define	NOPANIC	0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/seg.h>
#include <sys/errlog.h>
#include <sys/devmaj.h>
#include <sys/file.h>
#include <sys/mtio.h>
#include <sys/tk_info.h>	/* tk_info.h inlcudes tmscp.h */

int	wakeup();
extern	int	hz;

struct tk_info tk_info[];		/* information for MTIOCGET */
char	tk_on[];		/* 1 = doing online when open */ 
char	tk_rew[];		/* 1 = rewinding */ 
char	tk_wait[];		/* 1 = open is waiting during the rewinding */
char	tk_eot[];		/* 1 = tape hits the EOT */
char	tk_nrwt[];		/* 1 = don't wait on rewinding */
char	tk_clex[];		/* 1 = current reposition is clear serious exception only */
char	tk_fmt[];		/* Tmscp format */
char	tk_cache[];		/* Tmscp cache */
char	tk_cse[];		/* 1 = clear serious exception */
int	tk_csr[];	/* (c.c) Hardware register I/O page address */
int	tk_ivec[];	/* (c.c) TMSCP controller interrupt vector addressess */
int	ntk;		/* (c.c) number of TMSCP been configured */
char	*tk_dct[];	/* TMSCP cntlr type for error messages (TK50 or TU81) */

struct	tk_drv	tk_drv[];	/* drive types, status, unit size */

char	tk_ctid[];	/* controller type ID + U-code rev */

#ifdef	TKDBUG
int tk_error = 0;	/* set to cause hex dump of error log packets */
#endif	TKDBUG

struct tk_softc tk_softc[];

struct	tk	tk[];

struct	tmscp *tkgetcp();

/*
 * block I/O device error log buffer.
 */
struct	tk_ebuf	tk_ebuf[];
int	tk_elref[];	/* used with command reference number to */
			/* associate datagrams with end messages */

struct	buf tktab[];	/* controller queue */
struct	buf tkwtab;	/* I/O wait queue */
struct	buf ctkbuf[];	/* buffer for tkcmd */

/*
 * Initialize the device and set the unit online.
 */
tkopen(dev, flag)
	dev_t dev;
	int flag;	/* -1 = called from main(), don't set u.u_error */
{
	register struct tk_softc *sc;
	register struct tmscp *mp;
	register struct tk_regs *tkaddr;
	register struct	tk_drv	*tp;
	int unit, s, i;

	/*
	 * The B_TAPE flag tells the system this is a MAGTAPE, i.e.,
	 * no delayed writes, no read ahead, and don't allow tape
	 * device to suck up all the system buffers.
	 */
	unit = minor(dev) & 7;
	s = spl5();
	if(unit >= ntk)
		goto bad;
	tktab[unit].b_flags |= B_TAPE;
	tkaddr = tk_csr[unit];
	tp = &tk_drv[unit];
	if(tkaddr == 0) {
		if (flag < 0) {
			tk_ctid[unit] = 0;
			return;
		} else
			goto bad;
	}
	if(flag >= 0) {
		if(flag&FNDELAY) {
			tp->tk_openf = 1;
			splx(s);
			return;
		}
		if(tp->tk_openf != 0 || tk_wait[unit] != 0) {
			/* unit online or an open is waiting already */
			u.u_error = ETO;
			splx(s);
			return;
		}
		else if(tk_rew[unit] == 1) { /* rewinding */
			tk_wait[unit] = 1;
			tp->tk_flags |= TK_REWT;
			sleep((caddr_t)&tk_wait[unit], PSWP+1);
			if (tp->tk_flags & TK_REWT)
				goto wbad;
		}
	}
	tp->tk_flags &= TK_DEOT;
	tk_nrwt[unit] = 0;
	tk_clex[unit] = 0;
	sc = &tk_softc[unit];
	if (sc->sc_state != S_RUN) {
		if (sc->sc_state == S_IDLE)
			if(tkinit(unit))
				goto bad;	/* fatal cntlr error */
		/* wait for initialization to complete */
		s = spl6();
		timeout(wakeup,(caddr_t)&tk_softc[unit],15*hz);
		sleep((caddr_t)&tk_softc[unit], PSWP+1);
		splx(s);
		s = spl5();
		if (sc->sc_state != S_RUN)
			goto bad;
	}
/*
 * Get the status of the unit and save it,
 * by attempting to force unit online.
 */

	if(flag >= 0) {
		if(tp->tk_online == 0) {
			if (((tk_ctid[unit] >> 4) & 017) == TU81) {
				if (minor(dev) & 0100)
					tk_fmt[unit] = M_TF_GCR;
				else
					tk_fmt[unit] = M_TF_PE;
			}
			else
				tk_fmt[unit] = 0;
			tkcommand(unit,M_O_ONLIN,0);
			if(tp->tk_online == 0)
				goto bad;
		} else {
			if (((tk_ctid[unit] >> 4) & 017) == TU81) {
				i = 0;
				if (minor(dev) & 0100) {
					if (tk_fmt[unit] != M_TF_GCR) {
						tk_fmt[unit] = M_TF_GCR;
						i = 1;
					}
				} else {
					if (tk_fmt[unit] != M_TF_PE) {
						tk_fmt[unit] = M_TF_PE;
						i = 1;
					}
				}
				if(i == 1)
					tkcommand(unit,M_O_SETUC,0);
			}
		}
		tkcommand(unit,M_O_GTUNT,0);

		/* Status saved from response packet by interrupt routine tkrsp() */
		if(tp->tk_online == 0)		/* NED or off-line */
			goto bad;
		if (tp->tk_flags & TK_EOT) {
			tp->tk_flags &= ~TK_EOT;
			tk_eot[unit] = 1;
		} else
			tk_eot[unit] = 0;
		if ((flag & FWRITE) && (tp->tk_flags & TK_WRTP)) {
			tp->tk_flags &= ~TK_WRTP;
			u.u_error = ETWL;	/* write locked */
		}
		if (u.u_error == 0)
			tp->tk_openf = 1;
		else
			tp->tk_openf = 0;
	}
	splx(s);
	return;

bad:
	if(flag >= 0)
		tp->tk_openf = 0;
wbad:
	if(flag >= 0)
		u.u_error = ENXIO;
	splx(s);
}

tkclose(dev, flag)
	dev_t dev;
	int flag;
{
	register int unit;
	register struct tk_softc *sc;
	register struct tk_drv *tp;

	unit = minor(dev) & 7;
	sc = &tk_softc[unit];
	tp = &tk_drv[unit];
	if(tp->tk_online != 0 && tp->tk_openf != 0) {
		if(((flag & (FWRITE|FREAD)) == FWRITE) ||
		   ((flag&FWRITE) && (tp->tk_flags&TK_WRITTEN))) {
			if (tkcmd(dev, TMS_WRITM, 1) == -1)
				goto badcls;
			if (tkcmd(dev, TMS_WRITM, 1) == -1)
				goto badcls;
			if (tkcmd(dev, TMS_BSF, 1) == -1)
				goto badcls;
		}
		if((minor(dev) & 0200) == 0) {
			tk_nrwt[unit] = 1;
			if (tkcmd(dev, TMS_REW, 0) == -1)
				goto badcls;
		}
		else
			if (tkcmd(dev, TMS_CSE, 0) == -1)
				goto badcls;
	}
	tp->tk_openf= 0;
	tk_cse[unit] = 0;
	return;

badcls:
	u.u_error = ENXIO;
	return;
}

tkioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct tk_softc *sc;
	register struct buf *bp;
	register struct tk_drv *tp;
	struct buf *wbp, *wdp, *xbp;
	register callcount;	/* number of times to call cmd routine */
	struct mtop *mtop;	/* mag tape cmd op to perform */
	struct mtget *mtget;	/* mag tape struct to get info in */
	int fcount;		/* number of files (or records) to space */
	int unit, wunit, s;

	/* we depend of the values and order of the TMS ioctl codes here */
	static tmsops[] =
	 {TMS_WRITM,TMS_FSF,TMS_BSF,TMS_FSR,TMS_BSR,TMS_REW,TMS_OFFL,TMS_SENSE,
	  TMS_CACHE,TMS_NOCACHE,TMS_CSE,TMS_CLX,TMS_CLS,TMS_ASYNC,TMS_NOASYNC,
	  TMS_ENAEOT,TMS_DISEOT};

	unit = minor(dev) & 7;
	tp = &tk_drv[unit];
	sc = &tk_softc[unit];
	bp = &ctkbuf[unit];
	callcount = fcount = 0;
	switch (cmd) {

	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *)data;
		switch (mtop->mt_op) {

		case MTWEOF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;
		case MTBSF: case MTBSR:
		case MTFSF: case MTFSR:
			callcount = 1;
			fcount = mtop->mt_count;
			break;
		case MTREW: case MTOFFL:
			tk_nrwt[unit] = 1;
		case MTCACHE: case MTNOCACHE:
			callcount = 1;
			fcount = 1;
			break;
		case MTASYNC: case MTNOASYNC:
			return(0);
		case MTCLX:
		case MTCLS:
			wdp = &tkwtab;
			wbp = wdp->av_forw;
			while (wbp != 0 && wbp != wdp) {
				xbp = wbp->av_forw;
				wunit = minor(wbp->b_dev) & 7;
				/*
		 		* Unlink buffer from I/O wait queue.
		 		*/
				if (wunit == unit) {
					wbp->av_back->av_forw = wbp->av_forw;
					wbp->av_forw->av_back = wbp->av_back;
					if (wbp != bp) {
						wbp->b_error = EIO;
						wbp->b_flags |= B_ERROR;
						iodone(wbp);
					}
				}
				wbp = xbp;
			}
			while ((wbp = tktab[unit].b_actf) != NULL) {
				tktab[unit].b_actf = wbp->av_forw;
				if (wbp != bp) {
					wbp->b_error = EIO;
					wbp->b_flags |= B_ERROR;
					iodone(wbp);
				}
			}
			if (tp->tk_flags&TK_BUSY) {
				bp->b_flags &= ~B_BUSY;
				tp->tk_flags &= ~TK_BUSYF;
				iodone(bp);
			}
			if (tp->tk_flags&TK_WAIT) {
				bp->b_flags &= ~B_WANTED;
				tp->tk_flags &= ~TK_WAITF;
				wakeup((caddr_t)bp);
			}
			if (tk_wait[unit] != 0) {
				tk_wait[unit] = 0;
				wakeup((caddr_t)&tk_wait[unit]);
			}
			if(tkinit(unit)) {
				u.u_error = ENXIO;
				return(0);
			}
			s = spl6();
			timeout(wakeup,(caddr_t)&tk_softc[unit],15*hz);
			sleep((caddr_t)&tk_softc[unit], PSWP+1);
			splx(s);
			s = spl5();
			if (sc->sc_state != S_RUN) {
				u.u_error = ENXIO;
				return(0);
			}
			tk_rew[unit] = 0;
			tk_eot[unit] = 0;
			tk_nrwt[unit] = 0;
			tk_clex[unit] = 0;
			tk_fmt[unit] = 0;
			tp->tk_flags &= TK_DEOT;
			tp->tk_online = 0;
			tp->tk_openf= 0;
/*			tkstart(unit);	/* for multi-driver */
			return(0);
		case MTENAEOT:
			tp->tk_flags &= ~TK_DEOT;
			return(0);

		case MTDISEOT:
			tp->tk_flags |= TK_DEOT;
			return(0);

		case MTCSE:
			tp->tk_flags |= TK_CSE;
			callcount = 1;
			fcount = 1;
			break;
		default:
			return (ENXIO);
		}	/* end switch mtop->mt_op */

		if (callcount <= 0 || fcount <= 0)
			return (EINVAL);
		while (--callcount >= 0) {
			tkcmd(dev, tmsops[mtop->mt_op], fcount);
			if ((mtop->mt_op == MTFSR || mtop->mt_op == MTBSR) &&
			    bp->b_resid)
				return (EIO);
			if (mtop->mt_op == MTOFFL || mtop->mt_op == MTREW) {
				tk_eot[unit] = 0;
				if (mtop->mt_op == MTOFFL) {
					tp->tk_online = 0;
					tp->tk_openf = 0;
				}
			}
			if (bp->b_flags & B_ERROR)	/* like hitting BOT */
				break;
			}
		return (geterror(bp));

	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_type = MT_ISTK;
		mtget->mt_dsreg = tk_info[unit].tk_flags << 8;
		mtget->mt_dsreg |= tk_info[unit].tk_endcode;
		mtget->mt_erreg = tk_info[unit].tk_status;
		mtget->mt_resid = tk_info[unit].tk_resid;
		mtget->mt_softstat = 0;
		if (tk_eot[unit])
			mtget->mt_softstat |= MT_EOT;
		if (tp->tk_flags&TK_DEOT)
			mtget->mt_softstat |= MT_DISEOT;
		if (tk_cache[unit])
			mtget->mt_softstat |= MT_CACHE;
		break;

	default:
		return (ENXIO);
	}
	return (0);
}

/*
 * Initialize a TK50,
 * initialize data structures, and start hardware
 * initialization sequence.
 */
static
tkinit(unit)
{
	register struct tk_softc *sc;
	register struct tk_regs *tkaddr;
	unsigned cnt;

	sc = &tk_softc[unit];
	tkaddr = tk_csr[unit];
	/*
	 * Start the hardware initialization sequence.
	 */
	if(tkaddr->tkasa & TK_ERR) {	/* fatal cntlr error */
	bad:
		tkfatal(unit, NOPANIC, sc->sc_state, tkaddr->tkasa);
		return(1);
	}
	tkaddr->tkaip = 0;		/* start initialization */
	cnt = 0177777;	/* cntlr should enter S1 by 100us */
	while ((tkaddr->tkasa & TK_STEP1) == 0) {
		if(--cnt == 0)
			goto bad;
	}
	tkaddr->tkasa = TK_ERR|(NCMDL2<<11)|(NRSPL2<<8)|TK_IE|(tk_ivec[unit]/4);
	/*
	 * Initialization continues in interrupt routine.
	 */
	sc->sc_state = S_STEP1;
	sc->sc_credits = 0;
	return(0);
}

tkstrategy(bp)
	register struct buf *bp;
{
	register daddr_t *p;
	int	 unit, s, fs;

	mapalloc(bp);
	unit = minor(bp->b_dev) & 7;
	if((unit >= ntk) || (tk_drv[unit].tk_dt == 0)) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		if (bp == &ctkbuf[unit])
			tk_drv[unit].tk_flags &= ~TK_BUSY;
		iodone(bp);
		return;
	}
	s = spl5();
	/*
	 * Link the buffer onto the controller queue,
	 */
	bp->av_forw = 0;
	if(tktab[unit].b_actf == NULL)
		tktab[unit].b_actf = bp;
	else
		tktab[unit].b_actl->av_forw = bp;
	tktab[unit].b_actl = bp;
	if(tktab[unit].b_active == NULL)
		tkstart(unit);
	splx(s);
}

static
tkstart(unit)
{
	register struct buf *bp, *dp;
	register struct tmscp *mp;
	struct tk_softc *sc;
	struct tk_regs *tkaddr;
	int i;

	sc = &tk_softc[unit];
loop:
	if((bp = tktab[unit].b_actf) == NULL) {
		tktab[unit].b_active = NULL;
		return;
	}
	tktab[unit].b_active++;
	el_bdact |= (1 << TK_BMAJ);	/* tell error log cntlr active */
	tkaddr = tk_csr[unit];
	if ((tkaddr->tkasa&TK_ERR) || sc->sc_state != S_RUN) {
/*		tkinit(unit);	*/
		/* SHOULD REQUEUE OUTSTANDING REQUESTS, LIKE TKRESET */
/*		return;	*/
		tkfatal(unit, PANIC, sc->sc_state, tkaddr->tkasa);
	}
	if ((tk_drv[unit].tk_flags & TK_DEOT) == 0) {
		if (tk_eot[unit] == 1 && (tk_drv[unit].tk_flags&TK_CSE) == 0 && bp != &ctkbuf[unit])
			goto next;
	}
	/*
	 * If no credits, can't issue any commands
	 * until some outstanding commands complete.
	 */
	if (sc->sc_credits < 2)
		return (0);
	if(tk_drv[unit].tk_online == 0) {
		tk_cse[unit] = 0;
		tkcommand(unit,M_O_ONLIN,1);
		goto loop;
	}
	if ((mp = tkgetcp(unit)) == NULL)
		return (0);
 	sc->sc_credits--;	/* committed to issuing a command */
	mp->m_cmdref = bp;	/* pointer to get back */
	mp->m_elref = tk_elref[unit]++;	/* error log ref # */
	mp->m_unit = 0;
	if (tk_cse[unit]) {
		tk_cse[unit] = 0;
		mp->m_modifier = M_M_CLSEX;
	}
	else
		mp->m_modifier = 0;
	if (bp == &ctkbuf[unit]) {
		mp->m_bytecnt = 0;	/* object count lo */
		mp->m_zzz2 = 0;		/* object count hi */
		mp->m_tmcnt = 0;	/* tape mark count lo */
		mp->m_buf_h = 0;	/* tape mark count hi */
		switch (bp->b_resid) {
			case TMS_WRITM:
				mp->m_opcode = M_O_WRITM;
				break;
			case TMS_FSF:
				mp->m_opcode = M_O_REPOS;
				mp->m_modifier |= M_M_DLEOT;
				mp->m_tmcnt = bp->b_bcount;
				break;
			case TMS_BSF:
				mp->m_opcode = M_O_REPOS;
				mp->m_modifier |= M_M_REVRS;
				mp->m_tmcnt = bp->b_bcount;
				break;
			case TMS_FSR:
				mp->m_opcode = M_O_REPOS;
				mp->m_modifier |= M_M_OBJCT;
				mp->m_reccnt = bp->b_bcount;
				break;
			case TMS_BSR:
				mp->m_opcode = M_O_REPOS;
				mp->m_modifier |= (M_M_REVRS | M_M_OBJCT);
				mp->m_reccnt = bp->b_bcount;
				break;
			case TMS_REW:
				mp->m_opcode = M_O_REPOS;
				mp->m_modifier |= (M_M_REWND | M_M_CLSEX);
				tk_eot[unit] = 0;
				break;
			case TMS_OFFL:
				mp->m_opcode = M_O_AVAIL;
				mp->m_modifier |= (M_M_UNLOD | M_M_CLSEX);
				break;
			case TMS_CSE:
				mp->m_opcode = M_O_REPOS;
				mp->m_modifier |= M_M_CLSEX;
				mp->m_tmcnt = 0;
				tk_clex[unit] = 1;
				break;
			case TMS_CACHE:
			case TMS_NOCACHE:
				tk_cache[unit] = 0;
				if (bp->b_resid == TMS_CACHE)
					tk_cache[unit] = M_U_WBKNV;
				mp->m_opcode = M_O_SETUC;
				mp->m_format = tk_fmt[unit];
				mp->m_unitflgs = tk_cache[unit];
				mp->m_param = 0;
				mp->m_speed = 0;
				break;
			default:
				printf("Bad ioctl on tms unit %d\n", unit);
				mp->m_opcode = M_O_REPOS;
				break;
		}
	}
	else {
		tk_drv[unit].tk_flags &= ~TK_WRITTEN;
		mp->m_opcode = bp->b_flags&B_READ ? M_O_READ : M_O_WRITE;
		mp->m_bytecnt = bp->b_bcount;
		mp->m_zzz2 = 0;
		mp->m_buf_l = bp->b_un.b_addr;
		mp->m_buf_h = bp->b_xmem;
	}
	*((int *)mp->m_dscptr + 1) |= TK_OWN|TK_INT;
	i = tkaddr->tkaip;		/* initiate polling */
	/*
	 * Move the buffer to the I/O wait queue.
	 */
	tktab[unit].b_actf = bp->av_forw;
	dp = &tkwtab;
	if(dp->av_forw == 0) {
	/*
	 * Init I/O wait queue links.
	 */
		dp->av_forw = dp;
		dp->av_back = dp;
	}
	bp->av_forw = dp;
	bp->av_back = dp->av_back;
	dp->av_back->av_forw = bp;
	dp->av_back = bp;
	goto loop;

next:
	tktab[unit].b_actf = bp->av_forw;
	bp->b_resid = bp->b_bcount;
	bp->b_error = ENOSPC;
	bp->b_flags |= B_ERROR;
	tk_info[unit].tk_status = M_S_SEX;
	tk_info[unit].tk_endcode = bp->b_flags&B_READ ? M_O_READ : M_O_WRITE;
	tk_info[unit].tk_flags = M_E_EOT|M_E_SEREX;
	tk_info[unit].tk_resid = 0;
	iodone(bp);
	goto loop;
}

/*
 * TK50 interrupt routine.
 */
tkintr(dev)
{
	register struct tk_regs *tkaddr;
	register struct tk_softc *sc;
	register struct tk *tkp;
	struct tmscp *mp;
	int unit, i;
	char	*ubm_off;	/* address offset (if UB map used) */

	if(ubmaps)
		ubm_off = (char *)&cfree;	/* cfree = UB virtual addr 0 */
	else
		ubm_off = 0;
	unit = minor(dev) & 7;
	tkp = &tk[unit];
	tkaddr = tk_csr[unit];
	sc = &tk_softc[unit];
	switch (sc->sc_state) {
	case S_IDLE:
		logsi(tkaddr);
		return;

	case S_STEP1:
#define	STEP1MASK	0174377
#define	STEP1GOOD	(TK_STEP2|TK_IE|(NCMDL2<<3)|NRSPL2)
		if (tkmask(unit, STEP1MASK, STEP1GOOD, 1000, 30))
			return;
		tkaddr->tkasa = ((char *)&tk[unit].tk_ca.ca_kringbase-ubm_off);
/*		tkaddr->tkasa = ((int)&tk[unit].tk_ca.ca_kringbase);	*/
		sc->sc_state = S_STEP2;
		return;

	case S_STEP2:
#define	STEP2MASK	0174377
#define	STEP2GOOD	(TK_STEP3|TK_IE|(tk_ivec[unit]/4))
		if (tkmask(unit, STEP2MASK, STEP2GOOD, 200, 100))
			return;
		tkaddr->tkasa = 0;    /* ringbase will always be in low 64kb */
		sc->sc_state = S_STEP3;
		return;

	case S_STEP3:
#define	STEP3MASK	0174000
#define	STEP3GOOD	TK_STEP4
		if (tkmask(unit, STEP3MASK, STEP3GOOD, 200, 100))
			return;
		tk_ctid[unit] = tkaddr->tkasa & 0377; /* save controller type ID */
		i = (tk_ctid[unit] >> 4) & 017;	/* cntlr type for error msg's */
		if(i == TK50)
			tk_dct[unit] = "TK50";
		else if(i == TU81)
			tk_dct[unit] = "TU81";
		else
			tk_dct[unit] = "TMSCP";
		/*
		 * Tell the controller to start normal operations.
		 * Also set the NPR burst size to the default,
		 * which is controller dependent.
		 */
		tkaddr->tkasa = TK_GO;
		sc->sc_state = S_SCHAR;

		/*
		 * Initialize the data structures.
		 */
		for (i = 0; i < NRSP; i++) {
			tkp->tk_ca.ca_rspdsc[i].rh = TK_OWN|TK_INT;
			tkp->tk_ca.ca_rspdsc[i].rl = &tkp->tk_rsp[i].m_cmdref;
			(char *)tkp->tk_ca.ca_rspdsc[i].rl -= ubm_off;
			tkp->tk_rsp[i].m_dscptr = &tkp->tk_ca.ca_rspdsc[i].rl;
			tkp->tk_rsp[i].m_header.tk_msglen = 
			    sizeof(struct tmscp) - sizeof(struct tmscp_header);
		}
		for (i = 0; i < NCMD; i++) {
			tkp->tk_ca.ca_cmddsc[i].ch = TK_INT;
			tkp->tk_ca.ca_cmddsc[i].cl = &tkp->tk_cmd[i].m_cmdref;
			(char *)tkp->tk_ca.ca_cmddsc[i].cl -= ubm_off;
			tkp->tk_cmd[i].m_dscptr = &tkp->tk_ca.ca_cmddsc[i].cl;
			tkp->tk_cmd[i].m_header.tk_msglen = 
			    sizeof(struct tmscp) - sizeof(struct tmscp_header);
			tkp->tk_cmd[i].m_header.tk_vcid = 1; /* tape VCID=1 */
		}
		sc->sc_lastcmd = 0;
		sc->sc_lastrsp = 0;
		if ((mp = tkgetcp(unit)) == NULL) {
			sc->sc_state = S_IDLE;
			wakeup((caddr_t)&tk_softc[unit]);
			return;
		}
		mp->m_opcode = M_O_STCON;
		mp->m_modifier = 0;
		mp->m_cntflgs = M_C_ATTN|M_C_MISC|M_C_THIS;
		*((int *)mp->m_dscptr + 1) |= TK_OWN|TK_INT;
		i = tkaddr->tkaip;	/* initiate polling */
		return;

	case S_SCHAR:
	case S_RUN:
		break;

	default:
		logsi(tkaddr);	/* will print SI followed by address */
		printf("\n%s unit %d: state=%d\n", tk_dct[unit], unit, sc->sc_state);
		return;
	}

	if (tkaddr->tkasa&TK_ERR) {
/*
		printf("%s unit %d: fatal error (%o)\n", tk_dct[unit], unit, tkaddr->tkasa);
		tkaddr->tkaip = 0;
		wakeup((caddr_t)&tk_softc[unit]);
 */
		tkfatal(unit, PANIC, sc->sc_state, tkaddr->tkasa);
	}
	/*
	 * Check for response ring transition.
	 */
	if (tkp->tk_ca.ca_rspint) {
		tkp->tk_ca.ca_rspint = 0;
		for (i = sc->sc_lastrsp;; i++) {
			i %= NRSP;
			if (tkp->tk_ca.ca_rspdsc[i].rh&TK_OWN)
				break;
			tkrsp(unit, tkp, sc, i);
			tkp->tk_ca.ca_rspdsc[i].rh |= TK_OWN;
		}
		sc->sc_lastrsp = i;
	}
	/*
	 * Check for command ring transition.
	 */
	if (tkp->tk_ca.ca_cmdint) {
		tkp->tk_ca.ca_cmdint = 0;
		wakeup((caddr_t)&tk[unit].tk_ca.ca_cmdint);
	}
	tkstart(unit);
}

/*
 * Process a response packet
 */
tkrsp(unit, tkp, sc, i)
	register struct tk *tkp;
	register struct tk_softc *sc;
	int i;
{
	register struct tmscp *mp;
	register struct tk_drv *tp;
	struct buf *bp;
	int st;
	int opcode, flags;
	int msglen;

	tp = &tk_drv[unit];
	mp = &tkp->tk_rsp[i]; /* need changes */
	msglen = mp->m_header.tk_msglen;
	mp->m_header.tk_msglen = sizeof(struct tmscp)-sizeof(struct tmscp_header);
	sc->sc_credits += mp->m_header.tk_credits & 017;
	if(sc->sc_tcmax < sc->sc_credits)	/* save xfer credit limit */
		sc->sc_tcmax = sc->sc_credits;
	if ((mp->m_header.tk_credits & 0360) > 020)
		return;
	/*
	 * If it's an error log message (datagram),
	 * pass it on for more extensive processing.
	 */
	if ((mp->m_header.tk_credits & 0360) == 020) {
		tkerror(unit, (struct tmslg *)mp, msglen);
		return;
	}
	if (mp->m_unit != 0)
		return;
	/*
	 * store information if not clear serious exception
	 */
	if (tk_clex[unit] == 0) {
		tk_info[unit].tk_status = mp->m_status;
		tk_info[unit].tk_endcode = mp->m_opcode;
		tk_info[unit].tk_flags = mp->m_flags;
		tk_info[unit].tk_resid = 0;
	}
	else
		tk_clex[unit] = 0;
	st = mp->m_status&M_S_MASK;
	opcode = mp->m_opcode&0377;
	flags = mp->m_flags&0377;
	switch (opcode) {
	case M_O_STCON|M_O_END:
		if (st == M_S_SUCC)
			sc->sc_state = S_RUN;
		else
			sc->sc_state = S_IDLE;
		el_bdact &= ~(1 << TK_BMAJ);
		tktab[unit].b_active = 0;
		wakeup((caddr_t)&tk_softc[unit]);
		break;

	case M_O_ONLIN|M_O_END:
		if (flags != 0 && ((flags&M_E_SEREX) || (flags&M_E_PLS)))
			tp->tk_online = 0;	/* mark drive offline */
		else if ((st == M_S_SUCC) || (st == M_S_OFFLN)) {
			tp->tk_dt = *((int *)&mp->m_mediaid)&0177;
			if(st == M_S_SUCC)
				tp->tk_online = 1;
			else
				tp->tk_online = 0;
		} else {
			tp->tk_dt = 0;		/* mark drive NED */
			tp->tk_online = 0;	/* mark drive offline */
		}
		if(tp->tk_online == 0) {
			printf("\n%s unit %d OFFLINE: status=%o\n",
				tk_dct[unit], unit, mp->m_status);
			while((bp = tktab[unit].b_actf) != NULL) {
				tktab[unit].b_actf = bp->av_forw;
				bp->b_error = ENXIO;
				bp->b_flags |= B_ERROR;
				if (bp == &ctkbuf[unit])
					tp->tk_flags &= ~TK_BUSY;
				iodone(bp);
			}
		}
		if(mp->m_cmdref)
			wakeup((caddr_t)mp->m_cmdref);
		break;

/*
 * The AVAILABLE ATTENTION messages occurs when the
 * unit becomes available after spinup,
 * marking the unit offline will force an online command
 * prior to using the unit and also make sure it will
 * do the initialization if there is off and on happen.
 */
	case M_O_AVATN:
		tk_eot[unit] = 0;	/* magtape not at EOT */
		tp->tk_online = 0;	/* mark unit offline */
		break;

	case M_O_END:
/*
 * An endcode without an opcode (0200) is an invalid command.  An illegal
 * opcode or parameter error will cause an invalid command error.
 */
		printf("\n%s unit %d: inv cmd err, ", tk_dct[unit], unit);
		printf("endcd=%o, stat=%o\n", opcode, mp->m_status);
		if ((char *)mp->m_cmdref == &tk_drv[unit].tk_dt) {
			tp->tk_online = 0;	/* mark drive offline */
			wakeup((caddr_t)mp->m_cmdref);
		} else {
			bp = (struct buf *)mp->m_cmdref;
			/*
		 	* Unlink buffer from I/O wait queue.
		 	*/
			bp->av_back->av_forw = bp->av_forw;
			bp->av_forw->av_back = bp->av_back;
			bp->b_error = EIO;
			bp->b_flags |= B_ERROR;
			iodone(bp);
		}
		break;
	case M_O_SETUC|M_O_END:
		if ((char *)mp->m_cmdref == &tk_drv[unit].tk_dt) {
			if(st != M_S_SUCC)
				tp->tk_online = 0;	/* mark drive offline */
			wakeup((caddr_t)mp->m_cmdref);
			break;
		}
	case M_O_WRITE|M_O_END:
	case M_O_READ|M_O_END:
	case M_O_REPOS|M_O_END:
	case M_O_WRITM|M_O_END:
	case M_O_AVAIL|M_O_END:
		/* Say last op was a write (need to write TMs on close) */
		if (opcode == (M_O_WRITE|M_O_END))
			tp->tk_flags |= TK_WRITTEN;
		bp = (struct buf *)mp->m_cmdref;
		/*
		 * Unlink buffer from I/O wait queue.
		 */
		bp->av_back->av_forw = bp->av_forw;
		bp->av_forw->av_back = bp->av_back;

		if (st == M_S_TAPEM || st == M_S_LED || st == M_S_BOT) {
			if (opcode == (M_O_READ|M_O_END)) {
				if((bp->b_flags&B_PHYS) == 0)
					clrbuf(bp);
				bp->b_resid = bp->b_bcount;
				tk_info[unit].tk_resid = bp->b_resid;
			}
			tk_cse[unit] = 1;
			iodone(bp);
			break;
		}
/*
 * Fatal I/O error,
 * format a block device error log record and
 * log it if possible, print error message if not.
 */
		if (st != M_S_SUCC) {
			bp->b_flags |= B_ERROR;
			if (st&M_S_BOT)
				tk_eot[unit] = 0;
		}
		if (mp->m_status & M_SS_EOT) {
			if (tp->tk_flags & TK_DEOT)
				tk_cse[unit] = 1;
			tk_eot[unit] = 1;
		} else if (mp->m_flags & M_E_EOT) {
			tk_eot[unit] = 1;
		} else {
			tk_eot[unit] = 0;
			tp->tk_flags &= ~TK_CSE;
		}
		if((bp->b_flags&B_ERROR) || (mp->m_flags&M_E_ERLOG)) {
			fmtbde(bp, &tk_ebuf[unit], (int *)mp,
				((sizeof(struct tmscp_header)/2)+(msglen/2)), 0);
			tk_ebuf[unit].tk_bdh.bd_csr = tk_csr[unit];
			/* hi byte of XMEM is drive type for ELP ! */
			tk_ebuf[unit].tk_bdh.bd_xmem |= (tp->tk_dt << 8);
			tk_ebuf[unit].tk_reg[0] = msglen;    /* real message length */
			if(st == M_S_SUCC)
				tk_ebuf[unit].tk_bdh.bd_errcnt = 2;	/* soft error */
			else
				tk_ebuf[unit].tk_bdh.bd_errcnt = 0;	/* hard error */
			if (st == M_S_OFFLN || st == M_S_AVLBL) {
				tp->tk_online = 0;	/* mark unit offline */
				printf("%s unit %d Offline\n", tk_dct[unit], unit);
			}
			else if((mp->m_status&M_S_MASK) == M_S_WRTPR)
				printf("%s unit %d Write Locked\n",tk_dct[unit],unit);
			else {
				if(!logerr(E_BD, &tk_ebuf[unit],
				(sizeof(struct elrhdr) + sizeof(struct el_bdh) +
				msglen + sizeof(struct tmscp_header))))
				deverror(bp, ((mp->m_opcode&0377) | (mp->m_flags<<8)), mp->m_status);
			}
		}
		bp->b_resid = bp->b_bcount - mp->m_bytecnt;
		tk_info[unit].tk_resid = bp->b_resid;
		if (tk_nrwt[unit] == 1) {
			tk_rew[unit] = 0;
			tk_nrwt[unit] = 0;
			if (tk_wait[unit] != 0) {
				tk_wait[unit] = 0;
				tp->tk_flags &= ~TK_REWT;
				wakeup((caddr_t)&tk_wait[unit]);
			}
		}
		else {
			if (bp == &ctkbuf[unit])
				tp->tk_flags &= ~TK_BUSY;
			iodone(bp);
		}
		break;

	case M_O_GTUNT|M_O_END:
		if (mp->m_flags & M_E_EOT)
			tp->tk_flags |= TK_EOT;
		if (mp->m_unitflgs & (M_U_WRPH|M_U_WRPS))
			tp->tk_flags |= TK_WRTP;
		if (mp->m_unitflgs&M_U_WBKNV)
			tk_cache[unit] = M_U_WBKNV;
		else
			tk_cache[unit] = 0;
		if (st != M_S_SUCC) {
			tp->tk_online = 0;	/* mark unit offline */
			printf("%s unit %d Offline\n", tk_dct[unit], unit);
		}
		wakeup((caddr_t)mp->m_cmdref);
		break;
	default:
		printf("\n%s unit %d: bad/unexp packet, len=%d opcode=%o\n",
			tk_dct[unit], unit, msglen, opcode);
	}
	/*
	 * If no commands outstanding, say controller
	 * is no longer active.
	 */
	if(sc->sc_credits == sc->sc_tcmax)
		el_bdact &= ~(1 << TK_BMAJ);
}


/*
 * Process an error log message
 *
 * Format an error log record of the type, tk datagram,
 * and log it if possible.
 * Also print a cryptic error message, this may change later !
 */
tkerror(unit, mp, msglen)
	register struct tmslg *mp;
{
	register int i;
	register int *p;
	struct buf *bp;

	if((mp->me_unit != 0) ||
	    (unit >= ntk) ||	/* THIS CHECK MUST STAY */
	    (mp->me_cmdref & 1)) {
		return;
	}
	tk_ebuf[unit].tk_hdr.e_time = time;
/*
 * The following test will not work if error log
 * reference numbers are added to non data transfer commands.
 */
	bp = (struct buf *)mp->me_cmdref;
	if(bp && mp->me_elref) {	/* only for xfer commands */
		tk_ebuf[unit].tk_bdh.bd_dev = bp->b_dev;
	} else
		tk_ebuf[unit].tk_bdh.bd_dev = ((TK_BMAJ<<8)|unit);
	tk_ebuf[unit].tk_bdh.bd_flags = 0100000;	/* say TK datagram */
	/* hi byte of XMEM is drive type for ELP ! */
	tk_ebuf[unit].tk_bdh.bd_xmem = (tk_drv[unit].tk_dt << 8);
	tk_ebuf[unit].tk_bdh.bd_act = el_bdact;
	if((mp->me_flags & (M_LF_SUCC | M_LF_CONT)) == 0)
		tk_ebuf[unit].tk_bdh.bd_errcnt = 0;	/* hard error */
	else
		tk_ebuf[unit].tk_bdh.bd_errcnt = 2;	/* soft error */
	tk_ebuf[unit].tk_bdh.bd_nreg = ((msglen/2) + (sizeof(struct tmscp_header)/2));
	p = (int *)mp;
	for(i=0; i<((msglen/2)+(sizeof(struct tmscp_header)/2)); i++)
		tk_ebuf[unit].tk_reg[i] = *p++;
	tk_ebuf[unit].tk_reg[0] = msglen;	/* real message length */
	if(logerr(E_BD, &tk_ebuf[unit], (sizeof(struct elrhdr) +
		sizeof(struct el_bdh) + msglen + sizeof(struct tmscp_header))))
			return;	/* no msg printed if error logged */
	printf("\n%s unit %d: %s err, ", tk_dct[unit], unit,
		mp->me_flags&(M_LF_SUCC | M_LF_CONT) ? "soft" : "hard");
	switch (mp->me_format&0377) {
	case M_F_CNTERR:
		printf("cont err, event=0%o\n", mp->me_event);
		break;

	case M_F_BUSADDR:
		printf("host mem access err, event=0%o, addr=%o 0%o\n",
			mp->me_event, mp->me_busaddr[1], mp->me_busaddr[0]);
		break;

	case M_F_TAPETRN:
		printf("xfer err, unit=%d, event=0%o\n",
			unit, mp->me_event);
		break;

	case M_F_STIERR:
		printf("STI err, unit=%d, event=0%o\n", unit,
			mp->me_event);
		break;

	case M_F_STIDEL:
		printf("STI drv err, unit=%d, event=0%o\n", unit,
			mp->me_event);
		break;

	case M_F_STIFEL :
		printf("STI fmt err, unit=%d, event=0%o\n", unit,
			mp->me_event);
		break;

	default:
		printf("unknown err, unit=%d, format=0%o, event=0%o\n",
			unit, mp->me_format&0377, mp->me_event);
	}

/*
 * The tkaerror flag is normally set to zero,
 * setting it to one with ADB will cause the entire
 * error log packet to be printed as octal words,
 * for some poor sole to decode.
 */
#ifdef	TKDBUG
	if (tk_error) {

		p = (int *)mp;
		for(i=0; i<((msglen + sizeof(struct tmscp_header))/2); i++) {
			printf("%o ", *p++);
			if((i&07) == 07)
				printf("\n");
		}
		printf("\n");
	}
#endif	TKDBUG
}


/*
 * Find an unused command packet
 * Only an immediate command can take the
 * last command descriptor.
 */
struct tmscp *
tkgetcp(unit)
{
	register struct tmscp *mp;
	register struct tkca *cp;
	register struct tk_softc *sc;
	int i;

	cp = &tk[unit].tk_ca;
	sc = &tk_softc[unit];
	i = sc->sc_lastcmd;
	if ((cp->ca_cmddsc[i].ch & (TK_OWN|TK_INT)) == TK_INT) {
		cp->ca_cmddsc[i].ch &= ~TK_INT;
		mp = &tk[unit].tk_cmd[i];
		mp->m_unit = mp->m_modifier = 0;
		mp->m_opcode = mp->m_flags = 0;
		mp->m_bytecnt = mp->m_zzz2 = 0;
		mp->m_buf_l = mp->m_buf_h = 0;
		mp->m_elgfll = mp->m_elgflh = 0;
		mp->m_copyspd = 0;
		sc->sc_lastcmd = (i + 1) % NCMD;
		return(mp);
	}
	return(NULL);
}

tkfatal(unit, tkp, st, sa)
{
	printf("\nTMSCP cntlr %d fatal error: ", unit);
	printf("CSR=%o SA=%o state=%d\n", tk_csr[unit], sa, st);
	if(tkp)
		panic("TMSCP fatal");
}

static
tkcmd(dev, com, count)
{
	register struct buf *bp;
	register struct tk_drv *tp;
	int unit;

	unit = minor(dev) & 7;
	tp = &tk_drv[unit];
	bp = &ctkbuf[unit];
	spl5();
	while(bp->b_flags&B_BUSY) {
		bp->b_flags |= B_WANTED;
		tp->tk_flags |= (TK_WAIT|TK_WAITF); 
		sleep((caddr_t)bp, PRIBIO);
		if ((tp->tk_flags&TK_WAITF) == 0)
			return(-1);
	}
	spl0();
	bp->b_dev = dev;
	bp->b_resid = com;
	bp->b_bcount = count;
	bp->b_blkno = 0;
	tp->tk_flags |= (TK_BUSY|TK_BUSYF);
	bp->b_flags = B_BUSY|B_READ;
	tkstrategy(bp);
	if(tk_nrwt[unit] == 1) {
		tp->tk_flags &= ~TK_BUSY;
		tk_rew[unit] = 1;
	} else {
		iowait(bp);
		if (tp->tk_flags&TK_WAIT) {
			tp->tk_flags &= ~TK_BUSY; 
			return(-1);
		}
	}
	if(bp->b_flags&B_WANTED) {
		tp->tk_flags &= ~TK_BUSY;
		wakeup((caddr_t)bp);
	}
	bp->b_flags &= B_ERROR;
}
tkcommand(unit,op,flag)
{
	register struct tk_softc *sc;
	register struct tmscp *mp;
	struct tk_regs *tkaddr;
	int i;

	while((mp = tkgetcp(unit)) == NULL) {
		if (flag)
			return(1);
		else
			sleep((caddr_t)&tk[unit].tk_ca.ca_cmdint, PSWP+1);
	}
	tkaddr = tk_csr[unit];
	sc = &tk_softc[unit];
	sc->sc_credits--;
	mp->m_opcode = op;
	mp->m_modifier = 0;
	mp->m_unit = 0;
	if (op != M_O_GTUNT) {
		mp->m_format = tk_fmt[unit];
		mp->m_modifier = M_M_CLSEX;
		mp->m_unitflgs = tk_cache[unit];
		mp->m_param = 0;
		mp->m_speed = 0;
	}
	if (flag)
		mp->m_cmdref = 0;
	else
		mp->m_cmdref = &tk_drv[unit].tk_dt;
	mp->m_elref = 0;	/* MBZ, see tkerror() */
	*((int *)mp->m_dscptr + 1) |= TK_OWN|TK_INT;
	i = tkaddr->tkaip;
	if (flag == 0)
		sleep((caddr_t)mp->m_cmdref, PSWP+1);
	return(0);
}
tkmask(unit, mask, good, delay, times)
int unit, mask, good, delay, times;
{
	struct tk_regs *tkaddr;
	struct tk_softc *sc;
	register i, j;

	tkaddr = tk_csr[unit];
	sc = &tk_softc[unit];

	j = 0;
	while(1) {
		if ((tkaddr->tkasa&mask) != good) {
			for(i=0;i<delay;i++);
			if (j++ < times)
				continue;
			sc->sc_state = S_IDLE;
			wakeup((caddr_t)&tk_softc[unit]);
			return(1);
		}
		else
			return(0);
	}
}

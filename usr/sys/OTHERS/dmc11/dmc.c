/*
 *  DMC-11 device driver
 *
 *  NOTE:
 *	This driver uses the old, in-address-space abuffers.
 *	Since those buffers no longer exist, this driver would
 *	need to be converted to use its own, local buffers
 *	before it could be used.
 */

/*
 *	SCCS id	@(#)dmc.c	2.2 (2.11BSD GTE) 11/26/94
 */

#include "param.h"
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/proc.h>

#define CLOSE           /* include ioctl(NETCLOSE,,) */

#define DMCPRI  PZERO-1
#define NDMC    1

#define BUFSIZ  256     /* size of buffers */
#define NRBUFS  (int)(BSIZE/BUFSIZ-1) /* # of extra receive buffer headers */
#define NTBUFS  1       /* number of transmit buffer headers */
        /*
         * note: dmc only allows 7 receive, 7 transmit buffers to be queued,
         * thus NRBUFS and NTBUFS must be <= 7.
         * On an 11/70 the Unibus map must be allocated; if mapalloc() is
         * used, only one buffer can have the map.  NTBUFS might as
         * well be 1 so the next (only) write outstanding is the one
         * with the map allocated.
         */
#define LOOPCNT 10      /* max time waiting for ~RDYI */
#define MAXERRS 7       /* number of DCK or TIMEOUTs before fail */

struct device {
        char    inctrl;
        char    mstrctrl;
        char    outctrl;
        char    bsel3;
        unsigned bufad;
        unsigned port1;
};

struct device *dmcaddr[] = {
        (struct device *)0160200,
        (struct device *)0160210
};


struct buf dmctbufs[NDMC][NTBUFS];
struct buf dmcrbufs[NDMC][NRBUFS];

struct dmcinfo {
        struct buf *bp; /* system receive buffer to relse */
        caddr_t addr;           /* saved addr for dmcitrans */
        unsigned cnt;           /* saved xmem + count for dmcitrans */
} dmcinfo[NDMC];

struct buf dmcutab[NDMC];

#define MCLR    0100    /* mstrctrl -- master clear */
#define RDYI    0200    /* inctrl -- port avail */
#define RQI     0040    /* inctrl -- request input transfer */
#define IEI     0100    /* inctrl -- interrupt enable, input */
#define RDYO    0200    /* outctrl -- output transfer ready */
#define IEO     0100    /* outctrl -- output interrupt enable */
#define TRBIT   0004    /* inctrl, outctrl -- mask for t/r flag */
#define TFLAG   0000    /* inctrl, outctrl -- flag indicates transmission */
#define RFLAG   0004    /* inctrl, outctrl -- flag indicates reception */
#define TTYPE   0003    /* inctrl, outctrl -- mask for transfer type */
#define BACC    0000    /* inctrl, outctrl -- buf addr, char count */
#define CTRL    0001    /* inctrl, outctrl -- control in/out */
#define INVAL   0002    /* inctrl -- invalid (to shut down gracefully) */
#define BASEI   0003    /* inctrl -- base in */

#define FDUP    00000   /* port1 (control in) -- full duplex operation */
#define HDUP    02000   /* port1 (control in) -- half duplex operation */

        /* control out error states: */
#define FATAL   01630   /* fatal errors */
#define DCK     00001   /* data check */
#define TIMEOUT 00002   /* no response, 21 sec. */
#define ORUN    00004   /* overun error (waiting for recv buffer) */
#define MAINT   00010   /* maintenance mesg recd */
#define DLOST   00020   /* data lost (recv buffer too small) */
#define DISCON  00100   /* DSR transition */
#define RESTART 00200   /* DDCMP start received */
#define NXM     00400   /* non-existent memory */
#define PROCERR 01000   /* procedure error */

#define OPEN    001
#define CLOSING 002     /* closing gracefully */
#define STATE   003
#define ERROR   004     /* fatal line error */
#define TOLD    010     /* console message already printed */
#define R_WAITING 0100  /* read waiting */
#define W_WAITING 0200  /* write waiting */
#define TR_WAITING 0400 /* itrans waiting */
#define WAITING   0700

#define EXTSHFT 14

#ifndef hiint
#define hiint(x) (int)(x>>16)
#define loint(x) (int)(x&177777)
#endif


#define tbufq   b_forw          /* empty transmit buffer headers */
#define outbufq b_back          /* filled transmit buffers */
#define rbufq   av_forw         /* empty receive buffers */
#define inbufq  av_back         /* filled receive buffers */

/* redeclarations for dmcutab: */

#define xcnt    b_resid         /* saved port1 (xmem+char cnt) */
#define state   b_flags


int     dmcbase[NDMC][64];

dmcopen(dev)
dev_t dev;
{
        register int unit;
        register struct buf *dp, *bp, *hp;
        long base;
        if ((unit=minor(dev)) >= NDMC || unit < 0) {
                u.u_error = ENXIO;
                return;
        }
        dp = &dmcutab[unit];
        if (dp->state & OPEN)
                return;

        dp->state = 0;

        dp->rbufq = bp = getiblk();
        dmcinfo[unit].bp = bp;
        bp->b_dev = dev;
        dmcinit(unit);

        dp->tbufq = dmctbufs[unit];
        for (hp = dp->tbufq; hp < &dmctbufs[unit][NTBUFS-1]; hp++)
                hp->tbufq = hp + 1;
        hp->tbufq = 0;
        dp->outbufq = 0;
        dp->state = OPEN;
}

dmcclose(dev)
dev_t dev;
{
        register int unit;
        register struct buf *dp;

        unit = minor(dev);
        dp = &dmcutab[unit];
        dmcaddr[unit]->outctrl &= ~IEO;
        brelse(dmcinfo[unit].bp);
        dp->state = 0;
}

dmcread(dev, uio)
dev_t dev;
struct uio *uio;
{
        register struct buf *dp;
        register struct buf *bp, *pbp;
        register int unit;

        unit=minor(dev);
        dp = &dmcutab[unit];

        if ((!(dp->state & OPEN))       /* last read after NETCLOSE */
            || (uio->uio_resid < bp->b_bcount)) {
                u.u_error = EINVAL;
                return;
        }

        for (;;) {
                (void) _spl5();
                if ((bp = dp->inbufq) != 0)
                        break;
                if (dp->state & ERROR) {
                        u.u_error = EIO;
                        dp->state &= ~(ERROR|TOLD);
                        dmcinit(unit);
			(void) _spl0();
                        return;
                }
                dp->state |= R_WAITING;
                sleep((caddr_t)dp, DMCPRI);
        }
        (void) _spl0();

        if (bp->b_bcount > 0) {
                uiomove(mapin(bp), bp->b_bcount, uio);
                mapout(bp);
        }
        dp->inbufq = bp->inbufq;
        for (pbp=dp; pbp->rbufq; pbp=pbp->rbufq)
                ;
        pbp->rbufq = bp;
        bp->rbufq = 0;
        dmcitrans(unit,BACC | RFLAG, bp->b_un.b_addr, bp->xcnt);
}

dmcwrite(dev, uio)
dev_t dev;
struct uio *uio;
{
        register struct buf *bp, *dp, *pbp;
        register int unit;

        if (chkphys(BYTE))
                return;
        unit = minor(dev);
        if (uio->uio_resid > BUFSIZ) {
                u.u_error = EINVAL;
                return;
        }
        dp = &dmcutab[unit];

        for (;;) {
                (void) _spl5();
                if (dp->state & ERROR) {
                        u.u_error = EIO;
                        /*
                         * If it is not necessary to report all errors
                         * on reads (presumably to one read process)
                         * the error could be cleared here.  However,
                         * dmcinit should not be called until the outbufq
                         * is empty, which is easiest to do by reading
                         * until a read returns -1, at which point the error
                         * is cleared.  Otherwise, do:
                         * dp->state &= ~(ERROR|TOLD);
                         * dmcinit(unit);
                         */
			(void) _spl0();
                        return;
                }
                if (bp = dp->tbufq)
                        break;
                dp->state |= W_WAITING;
                sleep((caddr_t)dp, DMCPRI);
        }
        (void) _spl0();
        physbuf(bp,dev,B_WRITE);
        u.u_procp->p_flag |= SLOCK;
        mapalloc(bp);
        dp->tbufq = bp->tbufq;
        for (pbp=dp; pbp->outbufq; pbp=pbp->outbufq)
                ;
        pbp->outbufq = bp;
        bp->outbufq = 0;

        dmcitrans(unit, BACC|TFLAG, bp->b_un.b_addr,
                ((unsigned)bp->b_xmem<<EXTSHFT)|bp->b_bcount);
        iowait(bp);
        uio->uio_resid = 0;
        bp->b_flags = 0;
        u.u_procp->p_flag &= ~SLOCK;
        bp->tbufq = dp->tbufq;
        dp->tbufq = bp;
        if (dp->state & W_WAITING) {
                dp->state &= ~W_WAITING;
                wakeup((caddr_t)dp);
        }
}

#ifdef CLOSE
#define NETCLOSE (('n'<<8)|1)

dmcioctl(dev,cmd,addr,flag)
dev_t dev;
caddr_t addr;
{
        register unit;
        unit = minor(dev);
        if (cmd == NETCLOSE) {
                dmcutab[unit].state |= CLOSING | ERROR;
                dmcitrans(unit,INVAL,(caddr_t)0,0);
        }
        else u.u_error = ENOTTY;
}
#endif

dmcoint(unit)
{
        register struct device *dmc;
        register struct buf *bp, *dp, *pbp;
        int errflgs;

        dp = &dmcutab[unit];
        dmc = dmcaddr[unit];
        if ((dmc->outctrl & TTYPE) == BACC) {
                if ((dmc->outctrl & TRBIT) == RFLAG) {
                        bp = dp->rbufq;
#ifdef DIAG
                        if (bp == 0) {
                                printf("dmc rbufq missing\n");
                                goto error;
                        }
                        if (bp->b_un.b_addr != dmc->bufad) {
                                printf("dmc receive out of order\n");
                                goto error;
                        }
#endif
                        bp->b_bcount = dmc->port1;
                        for (pbp=dp; pbp->inbufq; pbp=pbp->inbufq)
                                ;
                        pbp->inbufq = bp;
                        bp->inbufq = 0;
                        dp->rbufq = bp->rbufq;
                        if (dp->state & R_WAITING) {
                                dp->state &= ~R_WAITING;
                                wakeup((caddr_t)dp);
                        }
                } else {
                        bp = dp->outbufq;
#ifdef DIAG
                        if  (bp == 0) {
                                printf("dmc outbufq missing\n");
                                goto error;
                        }
                        if (bp->b_un.b_addr != dmc->bufad) {
                                printf("dmc transmit out of order\n");
                                goto error;
                        }
#endif
                        dp->outbufq = bp->outbufq;
                        iodone(bp);
                }
                dp->state &= ~(ERROR | TOLD);
                dp->b_errcnt = 0;
        } else {
                errflgs = dmc->port1;
#ifdef CLOSE
                if ((errflgs & PROCERR) && (dp->state & CLOSING)){
                        /* shutting down gracefully */
                        dp->state &= ~(CLOSING|OPEN);
                        dmc->outctrl &= ~IEO;
                        goto error;
                }
#endif
                if (((errflgs & (FATAL|TIMEOUT)) || ((errflgs&DCK) &&
                    ++dp->b_errcnt>MAXERRS)) && ((dp->state & TOLD)==0)) {
                        printf("DMC/%d error=%o\n",unit,errflgs);
                        goto error;
                }
#ifdef DIAG
                else if (dp->state & TOLD == 0) {
                        printf("dmc nonfatal error %o\n",errflgs);
                        dp->state &= TOLD;
                }
#endif
        }
        dmc->outctrl &= ~RDYO;
        return;
error:
        dmc->outctrl &= ~RDYO;
        dp->state |= ERROR|TOLD;
        for (bp=dp->outbufq; bp; bp=bp->outbufq) {
                bp->b_flags = B_ERROR | B_DONE;
                wakeup((caddr_t)bp);
        }
        if (dp->state & WAITING) {
                dp->state &= ~WAITING;
                wakeup((caddr_t)dp);
        }
}

dmciint(unit)
{
        register struct buf *dp;
        register struct device *dmc;
        register struct dmcinfo *infp;

        dp = &dmcutab[unit];
        dmc = dmcaddr[unit];
        infp = &dmcinfo[unit];

        if (dp->b_active) {
                dmc->bufad = infp->addr;
                dmc->port1 = infp->cnt;
                dmc->inctrl &= ~RQI;
                dp->b_active = 0;
                if (dp->state & TR_WAITING) {
                        dp->state &= ~TR_WAITING;
                        wakeup((caddr_t)dp);
                }
#ifdef DIAG
        } else {
                printf("bad DMC/%d input interrupt\n",unit);
                dp->state |= TOLD;
#endif
        }
}

dmcinit(unit)
{
        register struct buf *bp, *dp, *hp;
        long base;

        dp = &dmcutab[unit];
        dmcaddr[unit]->mstrctrl |= MCLR;
        dmcitrans(unit,BASEI, (caddr_t)dmcbase[unit], 0);
        dmcitrans(unit,CTRL, (caddr_t)0, FDUP);
        dmcaddr[unit]->outctrl |= IEO;

        bp = dmcinfo[unit].bp;
        base = (long)bp->b_un.b_addr + (((long)bp->b_xmem)<<16);
        bp->xcnt = ((unsigned)hiint(base)<<EXTSHFT) | BUFSIZ;
        dmcitrans(unit, BACC|RFLAG,  bp->b_un.b_addr, bp->xcnt);
        for (hp = &dmcrbufs[unit][0]; hp <= &dmcrbufs[unit][NRBUFS-1]; hp++) {
                bp->rbufq = hp;
                base += BUFSIZ;
                hp->b_un.b_addr = (caddr_t)loint(base);
                hp->b_xmem = (char)hiint(base);
                hp->xcnt = ((unsigned)hiint(base)<<EXTSHFT) | BUFSIZ;
                dmcitrans(unit,BACC | RFLAG, hp->b_un.b_addr, hp->xcnt);
                bp = hp;
        }
        bp->rbufq = 0;
        dp->b_errcnt = 0;
        dp->inbufq = 0;
}

dmcitrans(unit, ttype, addr, cnt)
caddr_t addr;
unsigned cnt;
{
        register struct buf *dp;
        register struct device *dmc;
        register int i;
        register struct dmcinfo *infp;

        dp = &dmcutab[unit];
        infp = &dmcinfo[unit];
        for (;;) {
                if (dp->state & ERROR)
                        return;
                (void) _spl5();
                if (!dp->b_active)
                        break;
                dp->state |= TR_WAITING;
                sleep((caddr_t)dp,DMCPRI);
        }
        (void) _spl0();
        infp->addr = addr;
        infp->cnt = cnt;
        dp->b_active++;

        /* start transfer */
        i = 0;
        dmc = dmcaddr[unit];
        while (dmc->inctrl & RDYI)
                if (++i > LOOPCNT) {
                        /*
                         * This shouldn't happen; if it does, it
                         * will cause a spurious initialization.
                         */
#ifdef DIAG
                        printf("dmc missed RDYI going off\n");
#endif
                        dp->state |= ERROR;
                        return;
                }
        dmc->inctrl |= RQI|ttype|IEI;
}

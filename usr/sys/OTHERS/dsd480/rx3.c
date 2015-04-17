/*
 *	SCCS id	@(#)rx3.c	2.2 (2.11BSD GTE) 1/2/93
 */

/*
 *
 * Data Systems Design (DSD480) floppy disk drive
 *
 * Lauri Rathmann
 * Tektronix
 *
 *
 * Notes:
 *	The drive is being used in MODE 3 (Extended IBM)
 *	The ioctl mechanism is being used to determine
 *	format, density, number of sectors, bytes per
 *	sector and number of sides.
 *
 * History:
 * August 1981
 *	Written (for 2.8BSD UNIX)
 *
 * January 1982 Modified slightly to work for standard V7 UNIX
 *	S. McGeady
 *
 * NOTE:
 *	This is a block rx3device: the 'c.c' file should contain
 *	lines that look like:
 *
 *	int	rx3open(), rx3close(), rx3strategy();
 *	struct	buf	rx3tab;
 *	...
 *	in bdevsw:
 *	rx3open, rx3close, rx3strategy, &rx3tab,
 *
 *	...
 *	int	rx3ioctl();
 *	...
 *	in cdevsw:
 *	nulldev, nulldev, nodev, nodev, rx3ioctl, nulldev, 0,
 *
 *	The block rx3device is used for all I/O, and the character rx3device
 *	is used only for the IOCTL to set up the rx3device format parameters
 */

#include "rx3.h"
#if	NRX3 > 0
#include "param.h"
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/rx2reg.h>

extern	struct	rx3device	*RX3ADDR;

#define	RXUNIT(dev)	(minor(dev)&07)

#define NSEC		4	/* Number of logical sections on a disk */
#define MSEC		5	/* Maximum number of retrys */
#define TTIME		(2 * hz)/* Timeout time in HZ */
#define trwait()  	while((rx3r->rx2cs & RX2_XREQ) == 0)

/*
 * type of disk information.  The floppy is logically divided
 * into four sections.  1) Track 0, side 0
 *			2) Track 1-76, side 0
 *			3) Track 0, side 1
 *			4) Track 1-76, side 1
 */
struct	rx3info {
	long	first_byte;		/* first byte of section */
	long	no_bytes;		/* number of bytes in section */
	char	first_track;		/* track offset for section */
	char	no_track;		/* number of tracks in section */
	char	sector_code;		/* Information about sectors */
	char	interleave;		/* interleave factor */
	char	secperrot;		/* sectors per rotation for track */
	u_short	skew;			/* skew factor */
	char	side_code;		/* 0=side 0, 1=side 1; 2=either side */
} rx3info[NRX3][NSEC] = {
	0L, 	252928L, 0, 77, 0, 2,  13, 6, 0,
	0L,	0L,      0, 0,  0, 1,  0,  0, 0,
	0L,	0L,      0, 0,  0, 1,  0,  0, 0,
	0L, 	252928L, 0, 77, 0, 1,  26, 0, 0
};

struct secinfo {
	char	no_sect;		/* number of sectors per cyclinder */
	short	bytpersec;		/* number of bytes per sector */
	char	density;		/* 0=single, 1=double */
	short	bytpertrk;		/* number of bytes per track */
	short	sect_size;		/* coded sector size */
} secinfo[] = {
	26, 128, 0, 26*128, 0,		/* 26  128 byte sect/trk */
	26, 256, 1, 26*256, 0,
	15, 256, 1, 15*256, 2,		/* 15 512 byte sectors/track */
	15, 512, 1, 15*512, 2,
	8,  512, 1, 8*512,  4,		/* 8 512 byte sectors/trakc */
	8, 1024, 1, 8*1024, 4
	};

struct	rx3stat {
	char	rx3open;		/* open/closed status */
	short	bytect;			/* remaining bytes */
	char	com;			/* save current command */
	char	sec_code;		/* pointer to sector data */
	caddr_t	addr;			/* address in memory */
	short	xmem;			/* high order bits of memory */
	off_t	seek;			/* current byte to begin operation */
	short	uid;			/* current user's uid */
} rx3stat[NRX3];

struct	rx3errs {
	short	retries;		/* number of retries */
	short	errs;			/* number of hard errors */
	short	errreg;			/* Last error register */
	short	stat1;			/* extended status location 1 */
	short	stat2;			/* extended status location 2 */
	short	stat3;			/* extended status location 3 */
	short	stat4;			/* extended status location 4 */
} rx3errs[NRX3];

struct	buf	rx3tab;	/* driver info */

/*
 * ioctl commands for
 * rx03 floppy
 */
#define	RX3SET		((('r') << 8) | 2)	/* set up parameters */
#define	RX3RD		((('r') << 8) | 3)	/* read parameters */
#define	RX3STAT		((('r') << 8) | 4)	/* read status */
#define	RX3ERR		((('r') << 8) | 5)	/* give error information */

/*
 * Commands.
 */
#define	XINIT		01
#define	XREAD		02
#define	XWRITE		04
#define	XFILL		010
#define	XEMPTY		020
#define	XERR		040

/*
 * States.
 */
#define	NCRC		10		/* number of crc retries */
#define	NDEN		2		/* number of density retries */

#ifdef	RX3_TIMEOUT
int	rx3_wticks;		/* used to keep track of lost interrupts */
int	rx3wstart;
int	rx3watch();
#endif	RX3_TIMEOUT

/*
 * Open drive for use.
 *
 */
/*ARGSUSED*/
rx3open(dev, flag)
register dev_t dev;
{
	register drv, result;

	drv = RXUNIT(dev);


	/* Make sure drive is a valid one */
	if (drv >= NRX3) {
		u.u_error = ENXIO;
		return;
	};

#ifdef	RX3_TIMEOUT
	/* Start timing for timeouts.  Set status to open */
	if (rx3wstart==0)
		timeout(rx3watch, (caddr_t) 0, TTIME);
	rx3wstart++;
#endif

	rx3stat[drv].rx3open++;
	rx3stat[drv].uid = u.u_uid;
	rx3errs[drv].retries = 0;
	rx3errs[drv].errs = 0;
}

/*
 * Close drive. Simply need to turn off timeouts and
 * set drive status to closed.
 */
/*ARGSUSED*/
rx3close(dev, flag)
register dev_t dev;
{
	register drv = RXUNIT(dev);

#ifdef	RX3_TIMEOUT
	rx3wstart--;
#endif	RX3_TIMEOUT
	rx3stat[drv].rx3open--;
	if (rx3stat[drv].rx3open < 0)
		rx3stat[drv].rx3open = 0;
}

rx3strategy(bp)
register struct buf *bp;
{
	register opl;
	int mdev,okay,i;
	off_t seek;

	if (bp->b_flags & B_PHYS)
		mapalloc(bp);
	/*
	 *  Make sure block number is within range.
	 */
	okay = 0;
	mdev = RXUNIT(bp->b_dev);
	seek = (dbtofsb(bp->b_blkno) << BSHIFT) + bp->b_bcount;
	for (i=0; i<NSEC;  i++)
		if ((rx3info[mdev][i].no_bytes+rx3info[mdev][i].first_byte) > 
								seek) okay = 1;
	if (!okay) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	};

	/*
	 * Link buffer into rx3device queue
	 */
	bp->av_forw = (struct buf *) NULL;
	opl = spl5();
	if (rx3tab.b_actf == (struct buf *) NULL)
		rx3tab.b_actf = bp;
	else
		rx3tab.b_actl->av_forw = bp;
	rx3tab.b_actl = bp;
	if (rx3tab.b_active == 0)
		rx3start();
	splx(opl);
}

/*
 *  Start processing command.
 */
rx3start()
{
	register struct buf *bp;
	register int mdev;


	if ((bp = rx3tab.b_actf) == (struct buf *) NULL)
		return;
	rx3tab.b_active++;

	/*
	 * get minor rx3device number from buffer
	 */
	mdev = RXUNIT(bp->b_dev);

	/*
	 *  Set up status for current buffer
	 */
	rx3stat[mdev].addr = bp->b_un.b_addr;
	rx3stat[mdev].xmem = bp->b_xmem;
	rx3stat[mdev].seek = dbtofsb(bp->b_blkno) << BSHIFT;
	rx3stat[mdev].bytect = bp->b_bcount;

	/*
	 * if command is read, initiate the command
	 * if command is write, fill the rx3device buffer,
	 *   then initiate the write
	 */
	if (bp->b_flags & B_READ)
		rx3io(bp, XREAD);
	else 
		rx3io(bp, XFILL);
}

/*
 * Do actual IO command.
 */
rx3io(bp, cmd)
register struct buf *bp;
register short cmd;
{
	register struct rx3device *rx3r;
	int sect, code, trk, side, drv;


	rx3r = RX3ADDR;
	drv = RXUNIT(bp->b_dev);
	rx3stat[drv].com = cmd;
        rx3leave(bp,drv,&code,&sect,&trk,&side);
 	rx3stat[drv].sec_code = code;

	switch (cmd) {
		case XINIT:  /* Read status */
			cmd = RX2_GO|RX2_RDSTAT|RX2_IE|drv<<4;
			rx3r->rx2cs = cmd;
			break;
	
		case XREAD:	/* Read Sector */
			cmd = RX2_GO|RX2_RSECT|RX2_IE|(drv<<4)|(secinfo[code].density<<8)|
				(side<<9);
			rx3r->rx2cs = cmd;
			trwait();
			rx3r->rx2sa = sect|(secinfo[code].sect_size<<5);
			trwait();
			rx3r->rx2ta = trk;
			break;

		case XEMPTY: 	/* Empty buffer */
			/* no disk i/o; unit not needed */
			cmd = RX2_GO|RX2_EMPTY|RX2_IE|(side<<9)|(secinfo[code].density<<8)|
				(drv<<4)|(rx3stat[drv].xmem<<12);
			rx3r->rx2cs = cmd;
			trwait();
				/* NOT 2's complement */
			if (rx3stat[drv].bytect <= secinfo[code].bytpersec)
				rx3r->rx2wc = rx3stat[drv].bytect>>1;
			else
				rx3r->rx2wc = secinfo[code].bytpersec>>1;
			trwait();
			rx3r->rx2ba = (int) rx3stat[drv].addr;
			break;

		case XWRITE:		/* Write buffer to disk */
			cmd = RX2_GO|RX2_WSECT|RX2_IE|(drv<<4)|(secinfo[code].density<<8)|
				(side<<9);
			rx3r->rx2cs = cmd;
			trwait();
			rx3r->rx2sa = sect|(secinfo[code].sect_size<<5);
			trwait();
			rx3r->rx2ta = trk;
			break;

		case XFILL:		/* Fill disk buffer */
			/* no disk i/o; unit not needed */
			cmd = RX2_GO|RX2_FILL|RX2_IE|(side<<9)|(secinfo[code].density<<8)|
				(drv<<4)|(rx3stat[drv].xmem<<12);
			rx3r->rx2cs = cmd;
			trwait();
				/* NOT 2's complement */
			if (rx3stat[drv].bytect <= secinfo[code].bytpersec)
				rx3r->rx2wc = rx3stat[drv].bytect>>1;
			else
				rx3r->rx2wc = secinfo[code].bytpersec>>1;
			trwait();
			rx3r->rx2ba = (int) rx3stat[drv].addr;
			break;

		case XERR: 	/* Read extended error status */
			cmd = RX2_IE|RX2_RDEC;
			rx3r->rx2cs = cmd;
			trwait();
			rx3r->rx2ba = (int) &rx3errs[drv].stat1;
			break;

		default:
			panic("rx3io");
			break;
	}
}

/*
 *	Process interrupt.
 */
rx3intr()
{
	register struct buf *bp;
	register struct rx3device *rx3r;
	int drv, code;
	long paddr;


	if (rx3tab.b_active == 0) {
		printf("rx3:  stray interrupt\n");
		return;
	}

	bp = rx3tab.b_actf;
	rx3r = RX3ADDR;
#ifdef	RX3_TIMEOUT
	rx3_wticks = 0;
#endif	RX3_TIMEOUT

	if (rx3r->rx2cs < 0) {
		rx3err(bp);
		return;
	}

	drv = RXUNIT(bp->b_dev);

	switch (rx3stat[drv].com) {
		case XINIT:
			rx3errs[drv].errreg = rx3r->rx2es;
			rx3tab.b_active = 0;
			rx3tab.b_errcnt = 0;
			rx3stat[drv].bytect = 0;
			iodone(bp);
			break;
		case XREAD:
			rx3io(bp, XEMPTY);
			break;
		case XFILL:
			rx3io(bp, XWRITE);
			break;
		case XWRITE:	/* Go on to next sector if should */
		case XEMPTY:
			code = rx3stat[drv].sec_code;
			if (rx3stat[drv].bytect <= secinfo[code].bytpersec) {
				rx3stat[drv].bytect = 0;
				rx3tab.b_errcnt = 0;
				rx3tab.b_active = 0;
				rx3tab.b_actf = bp->av_forw;
				iodone(bp);
				rx3start();
			}
			else {
				rx3stat[drv].bytect -= secinfo[code].bytpersec;
				paddr = (long) rx3stat[drv].addr + 
					(long) secinfo[code].bytpersec;
				rx3stat[drv].addr = (caddr_t) paddr;
				rx3stat[drv].xmem = rx3stat[drv].xmem +
					(int) (paddr>>16);
				rx3stat[drv].seek += secinfo[code].bytpersec;
				if (rx3stat[drv].com==XWRITE) 
					rx3stat[drv].com=XFILL;
				else rx3stat[drv].com=XREAD;
				rx3io(bp,rx3stat[drv].com);
			}
			break;
		case XERR:
			break;
			
		default:
			printf("rx3:  command %o\n", rx3stat[drv].com);
			break;
	}
}

/*
 * Handle an error condition.
 * Crc errors and density errors get retries,
 * only NDEN for density errors and NCRC for crc errors.
 * All other errors are considered hard errors.
 */
rx3err(bp)
register struct buf *bp;
{
	register struct rx3device *rx3r;
	register int drv;
	register int cmd;

	drv = minor(bp->b_dev);


	rx3tab.b_active = 0;
	rx3r = RX3ADDR;
	rx3errs[drv].errreg = rx3r->rx2es;

	/*
	 * Crc error.
	 */
	if (rx3r->rx2es & RX2ES_CRC) {
		rx3errs[drv].retries++;
		if (rx3tab.b_errcnt < NCRC) {
			rx3tab.b_errcnt++;
			rx3reset();
			return;
		}
	}

	/*
	 * Density error.
	 */
	if (rx3r->rx2es & RX2ES_DENSERR) {
		rx3errs[drv].retries++;
		if (rx3tab.b_errcnt < NDEN) {
			rx3tab.b_errcnt++;
			rx3reset();
			return;
		}
	}

	rx3errs[drv].errs++;
	bp->b_flags |= B_ERROR;
#ifdef	UCB_DEVERR
	harderr(bp, "rx3");
	printf("cs=%b er=%b\n", rx3r->rx2cs, RX2_BITS, rx3r->rx2es, RX2ES_BITS);
#else
	deverror(bp, (rx3r->rx2es&0377), (rx3r->rx2cs));
#endif	UCB_DEVERR
	
	rx3tab.b_active = 0;
	rx3tab.b_errcnt = 0;
	rx3tab.b_actf = bp->av_forw;

	iodone(bp);
	rx3reset();
}

/*
 * Calculate the physical sector and physical track on the
 * disk for a given logical sector.
 * 
 */
rx3leave(bp,drv,code,sect,trk,side)
int *code,*sect,*trk,*side;
register struct buf *bp;
{
	off_t seek, t0;
	int t1,t2,t3;
	int section;


	/*
	 * Determine track by searching the rx3info table
	 * looking at first_byte and no_bytes. 
	 *     first_byte <= seek < first_byte + no_bytes
	 */
	seek = rx3stat[drv].seek;
	section = 0;
	*trk = 0;
	while (section<NSEC && *trk==0) {
		if (rx3info[drv][section].first_byte <= rx3stat[drv].seek) {
			t0 = rx3info[drv][section].first_byte + 
			     rx3info[drv][section].no_bytes;
			if (rx3stat[drv].seek < t0) *trk = 1;
		}
		if (!*trk) {
			seek -= rx3info[drv][section].no_bytes;
			section++;
		}
	}
	if (!*trk) {
		bp->b_flags |= B_ERROR;
         	rx3tab.b_active = 0;
		iodone(bp);
		return(1);
	};
	*code = rx3info[drv][section].sector_code;
	t1 = secinfo[*code].bytpertrk;
	t2 = t1 * (rx3info[drv][section].side_code==2 ? 2 : 1 );
	*trk = seek/t2;		/* track offset */

	/*
	 * Determine side of disk.  Use section code.
 	 */
	t3 = seek % t2;			/* sector offset in track */
	if (rx3info[drv][section].side_code==0) *side = 0;
	else if (rx3info[drv][section].side_code==1) *side = 1;
	else {
		if (t3 < t1) *side = 0;
		else {
			t3 -= t1;
			*side = 1;
		}
	};

	/*
	 * Determine physical sector.
	 */
	t1 = t3 / secinfo[*code].bytpersec;	/* logical sector */
	t1 = t1 % secinfo[*code].no_sect;
	t2 = t1 / rx3info[drv][section].secperrot;	/* logical rotation */
	t3 = t1 % rx3info[drv][section].secperrot;	/* sector offset */
	t3 = t3 * rx3info[drv][section].interleave;
	t3 = t3 + (rx3info[drv][section].skew * *trk);
	/* sector count begins with 1 */
	*sect = (t3 + t2) % secinfo[*code].no_sect + 1;
	*trk += rx3info[drv][section].first_track;	/* physical track */
        return(0);
}

rx3ioctl(dev, cmd, addr, flag)
caddr_t addr;
{
	int drv;
	register struct rx3device *rx3r;
	
	drv = RXUNIT(dev);
	

	switch (cmd) {
		case RX3SET:	/* Setup parameters for disk */
			if (copyin(addr, (caddr_t)rx3info[drv], 
						sizeof(struct rx3info)*NSEC)) {
				u.u_error = EFAULT;
				return;
			};
			break;
		case RX3RD:	/* return disk parameters */
			if (copyout((caddr_t)rx3info[drv], addr,
						sizeof(struct rx3info)*NSEC)) {
				u.u_error = EFAULT;
				return;
			}
			break;
		case RX3STAT:
			break;
		case RX3ERR:
			if (copyout( (caddr_t)&rx3errs[drv], addr, 
						sizeof(struct rx3errs))) {
				u.u_error = EFAULT;
				return;
			}
			break;
		default:
			u.u_error = ENOTTY;
			break;
	}
}

/*
 * reset drives and restart controller.
 */
rx3reset() {
	register struct rx3device *rx3r;
	register int i;
	register short cmd;


	rx3r = RX3ADDR;

	/*
	 * If any drive is in use
	 * then do an init.
	 */
	for (i = 0; i < NRX3; i++) {
		if (rx3stat[i].rx3open) {
			rx3r->rx2cs = RX2_INIT;
			while ((rx3r->rx2cs & RX2_DONE) == 0);
			break;
		}
	}

	rx3start();
}

#ifdef	RX3_TIMEOUT
/*
 * Wake up every second and if an interrupt is pending
 * but nothing has happened increment wticks. If nothing
 * happens for 60 seconds, reset the controller and begin
 * anew.
 */
rx3watch()
{
	if (rx3wstart)
		timeout(rx3watch, (caddr_t) 0, TTIME);

	if (rx3tab.b_active == 0) {
		rx3_wticks = 0;		/* idling */
		return;
	}

	rx3_wticks++;

	if (rx3_wticks >= 60) {
		rx3_wticks = 0;
		printf("rx3:  lost interrupt\n");
		rx3reset();
	}
}
#endif	RX3_TIMEOUT
#endif	NRX3

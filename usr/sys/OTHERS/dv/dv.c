#
#define	DK_N	2
/*
 * DV disk driver
 *
 * Adapted for Version 7 UNIX 8/21/79 - Bob Kridle
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/user.h>

#ifdef	UCB_SCCSID
static	char sccs_id[] = "@(#)dv.c	3.1";
#endif


struct {
	int	dvcsr;
	int	dvdbr;
	int	dvmar;
	int	dvwcr;
	int	dvcbr;
	int	dvssr;
	union
	{
		int	i;
		char	c[2];
	} dvair;
	int	dvusr;
};

#define	DVADDR	((struct device *) 0164000 )
#define	NDV	4

struct dv_sizes {
	daddr_t	nblocks;
	int	cyloff;
} dv_sizes[] {
	10080,	0,		/* cyl 0 thru 41	*/
	10080,	42,		/* cyl 42 thru 83	*/
	77280,	84,		/* cyl 84 thru 405	*/
	97440,  0,		/* cyl 0 thru 405	*/
	54000,	180,		/* cyl 180 thru 405	*/
	65520,	0,		/* cyl 0 thru 272	*/
	31680,	273,		/* cyl 273 thru 405	*/
	2048,	123,		/* cyl 123 thru 132	*/
};

struct {
	int	hd1;
	int	hd2;
	int	cksum;
	unsigned int chadr;
} dvhdr;
int	dv_unit -1, dv_cyl, dv_head, dv_sctr, dv_count;
union
{
	long	l;
	int	in[2];
} dv_addr;

int	dv_debug	0;

int	dv_wcheck, dv_sw 0;


int wcwcnt[4];

struct	buf	dvtab;

char	dvsecmap[] {
	0, 4, 8,
	1, 5, 9,
	2, 6, 10,
	3, 7, 11,
};


#define	HTBCOM	000000
#define	CTBCOM	020000
#define	CNBCOM	030000
#define		INCHD	01
#define		RECAL	02
#define		RESET	020
#define		SEEK	040
#define		CLRDA	0100
#define	CHRCOM	070000
#define	CHWCOM	0130000
#define	CHCD	071000
#define	LDSCOM	0140000
#define	CTLRST	0170000
#define CHRDATA	060000
#define CHWHEAD	0110000
#define WENABL	0100000

#define	DRVRDY	04000
#define	ATTN	0400
#define	DONE	0200
#define	IENABLE	0100
#define	GO	01

/*
 * Use av_back to save sector,
 * b_resid for cylinder+track.
 */

#define	dvsec	av_back
#define	cylhd	b_resid

dvstrategy(bp)
register struct buf *bp;
{
	register unsigned trkno;
	register struct dv_sizes *sptr;

	if(bp->b_flags&B_PHYS)
		mapalloc(bp);
	sptr = &dv_sizes[minor(bp->b_dev)&07];
	if ((minor(bp->b_dev)&077) >= (NDV<<3) ||
	    bp->b_blkno >= sptr->nblocks) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	bp->av_forw = NULL;
	trkno = bp->b_blkno/12;
	bp->cylhd = ((sptr->cyloff+trkno/20)<<5)|(trkno%20);
	bp->dvsec = bp->b_blkno%12;
	spl5();
	disksort(&dvtab,bp);
	if (dvtab.b_active==0)
		dvstart();
	spl0();
}

dvstart()
{
	register struct buf *bp;

	if ((bp = dvtab.b_actf) == 0) {
/*
		dv_busy = 0;
*/
		return;
	}
	dv_cyl = bp->cylhd>>5;
	dv_head = bp->cylhd&037;
	dv_sctr = bp->dvsec;
	dv_count = -bp->b_bcount;
	dv_addr.in[0] = bp->b_xmem & 03;
	dv_addr.in[1] = bp->b_un.b_addr;
	dvexec();
}

dvexec()
{
	register struct buf *bp;
	register sctr, minord;
	int i, cnt;

	bp = dvtab.b_actf;
	sctr = dv_sctr;
	minord = minor(bp->b_dev);
	if (minord&64)
		sctr = dvsecmap[sctr];
	dvtab.b_active++;
	if (dv_unit!=((minord&077)>>3)) {	/* select unit */
		dv_unit = (minord&077)>>3;
		cnt = 0;
		for (i = 0; i < 2; i++)
			while((DVADDR->dvssr&DRVRDY) && --cnt) ;
		DVADDR->dvcbr = LDSCOM | dv_unit;
	}
	if(dvrdy()) return;
	DVADDR->dvcbr = CNBCOM | RESET | CLRDA;	/* reset and clear */
	if (dv_cyl != (~(DVADDR->dvssr|0177000))) {	/* seek */
		if(dvrdy()) return;
		DVADDR->dvcbr = CTBCOM | dv_cyl;
		if(dvrdy()) return;
		DVADDR->dvcbr = CNBCOM | SEEK | RESET;
		DVADDR->dvair.i = 1<<dv_unit;
		DVADDR->dvcsr = DONE | IENABLE;
		return;
	}
	if(dvrdy()) return;
	DVADDR->dvcbr = HTBCOM | dv_head;	/* select head */
	if(dv_count <= -512)
		DVADDR->dvwcr = -512; else
		DVADDR->dvwcr = dv_count;
	dvhdr.hd1 = (dv_head<<8)+dv_cyl;	/* set up header */
	dvhdr.hd2 = 0170000|sctr;
	dvhdr.cksum = -dvhdr.hd1-dvhdr.hd2;
	dvhdr.chadr = dv_addr.in[1];
	if(dvrdy()) return;
	DVADDR->dvmar = &dvhdr;
	if(dv_debug)
		printf("ST,h1=%o,h2=%o,h4=%o,mar=%o,wc=%o,x=%o,\n",
			dvhdr.hd1,
			dvhdr.hd2,
			dvhdr.chadr,
			DVADDR->dvmar,
			DVADDR->dvwcr,
			dv_addr.in[0]
		);
	if (minord & 128) {
		if (bp->b_flags & B_READ) {
			dv_addr.in[0] = 0;
			DVADDR->dvmar = dv_addr.in[1];
			DVADDR->dvcbr = CHRDATA | (sctr<<1);
		} else {
			DVADDR->dvcbr = CHWHEAD | (sctr<<1);
			DVADDR->dvair.i |= WENABL;
		}
	} else {
		if (bp->b_flags & B_READ)
			DVADDR->dvcbr = CHRCOM | (sctr<<1);
		else
			if (dv_wcheck)
				DVADDR->dvcbr = CHCD | (sctr<<1);
			else
				DVADDR->dvcbr = CHWCOM | (sctr<<1);
	}
	DVADDR->dvcsr = IENABLE | GO | (dv_addr.in[0]<<4);
}

int	dv_tmp;
int	dv_errct;
dvintr()
{
	register struct buf *bp;
	register int csr,i;

	if (dvtab.b_active == 0)
		return;
	if(dv_debug)
		printf("IN,mar=%o,wc=%o,cbr=%o,\n",
			DVADDR->dvmar,
			DVADDR->dvwcr,
			DVADDR->dvcbr
		);

	/***** I/O monitoring stuff *****/
/* Removed for v7 Upgrade
 *	dk_busy =& ~(1<<DK_N);
 *	dv_busy = 19;
 */
	/*********************************/

	bp = dvtab.b_actf;
	dvtab.b_active = 0;
	csr = DVADDR->dvcsr;
	DVADDR->dvcsr = DONE;
	if (csr&ATTN) { /* seek complete */
		dv_wcheck = 0;
		DVADDR->dvair.c[0] = 0;
		if(DVADDR->dvssr>0) { /* error */
			printf("Seek error\n");
			deverror(bp, DVADDR->dvssr, csr);
			DVADDR->dvcbr = CNBCOM | RECAL | RESET;
			dv_unit = -1;
			if(dvrdy()) return;
			DVADDR->dvcbr = CTLRST;
			if (++dvtab.b_errcnt<=10) {
				dvexec();
				return;
			}
			dvherr(0);
			return;
		} else {
			dvexec();
			return;
		}
	} else {	/* r/w complete */
		if (dv_count <= -512)
			i = -512;
		else	i = dv_count ;
		if ((csr < 0 || dv_addr.in[1]-i != DVADDR->dvmar) &&
			(minor(bp->b_dev)&128)==0) {
			if (dv_addr.in[1]-i != DVADDR->dvmar) {
				printf("hdr/xfer err ");
				printf("%o %o %o\n", dv_addr.in[1], DVADDR->dvmar, csr);
				deverror(bp, DVADDR->dvssr, csr);
				dv_wcheck = 0;
			}
			dv_tmp = csr;
			dv_errct++;
			dv_unit = -1;
			if (dv_wcheck) {
				printf("diva bad write\n");
				wcwcnt[(minor(bp->b_dev)&030) >> 3]++;
				deverror(bp, DVADDR->dvssr, csr);
			}
			dv_wcheck = 0;
			if((dvtab.b_errcnt&03)==03) {
				deverror(bp, DVADDR->dvssr, csr);
				DVADDR->dvcbr = CNBCOM | RECAL | RESET;
				if(dvrdy()) return;
			}
			DVADDR->dvcbr = CTLRST;
			if(++dvtab.b_errcnt<=12) {
				dvexec();
				return;
			}
			dvherr(0);
			return;
		} else {
			if (dv_sw && (bp->b_flags&B_READ)==B_WRITE &&
				dv_wcheck==0 && dv_count==-512) {
				dv_wcheck = 1;
				dvexec();
				return;
			}
			dv_wcheck = 0;
			if ((dv_count += 512)<0) { /* more to do */
				dv_addr.l += 512;
			if (++dv_sctr>=12) {
				dv_sctr = 0;
				if (++dv_head>=20) {
					dv_head = 0;
					dv_cyl++;
				}
			}
			dvexec();
			return;
			}
		}
	}
	if (csr < 0 && (minor(bp->b_dev)&128)) {
		DVADDR->dvcbr = CTLRST;
		dv_unit = -1;
		dvrdy();
		if (DVADDR->dvcsr < 0) {
			DVADDR->dvcbr = CNBCOM | RECAL | RESET;
			dvrdy();
		}
	}
	dv_wcheck = 0;
	dvtab.b_errcnt = 0;
	dvtab.b_actf = bp->av_forw;
/*  This is not right but a kludge for the moment */
	bp->b_resid = 0;
	iodone(bp);
	dvstart();
}


dvrdy()
{
	register int cnt, i;

	if ((DVADDR->dvssr & DRVRDY) == 0)
		return(0);
	cnt = i = 0;
	/*
	 *	waste a bit of time, max .7(approx) sec on a 70
	 */
	for (; i < 2; i++)
		while(--cnt && (DVADDR->dvssr&DRVRDY));
	if (DVADDR->dvssr & DRVRDY) {
		printf("diva not ready\n");
		dvherr(1);
		return(1);
	}
	return(0);
}

dvherr(n)
{
	register struct buf *bp;

	bp = dvtab.b_actf;
	printf("Hard error on diva\n");
	deverror(bp, DVADDR->dvssr, DVADDR->dvcsr);
	bp->b_flags |= B_ERROR;
	dvtab.b_errcnt = 0;
	dvtab.b_active = 0;
	dvtab.b_actf = bp->av_forw;
	iodone(bp);
	if(n==0)
		dvstart();
}

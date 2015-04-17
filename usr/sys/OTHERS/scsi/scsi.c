/*
	Driver for 5 1/4'' Disk Drive using the DTC510 Host Adaptor
		with Adaptec 4000 Controller
*/

#include "scsi.h"
#if NIMI>0 || NIOM>0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/scsireg.h>

#define imi	((struct dtc *)DTC1_ADDR)

struct	sc_sizes scsitab[TABSIZ][PARTSIZ] =
{
	/* 512 byte blks */
	/* table zero 0 - the imi disk
		322 tracks total, 18 sectors on Adaptec
		with 512b blocks and intrlv 2, 6 heads */
	{
		{ 0,	9792 },
		{ 9792, 19584 },
		{ 29376, 3672 },
		{ 0,	33048 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 }
	},
	/* table one 1 - the old imi disk */
	{
		{ 0,	9580 },
		{ 9580, 2048 },
		{ 11628, 19584 },
		{ 0,	31212 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 }
	},
	/* table two 2 - Wren Disk without Swap Space
		697 tracks with 18 512b sectors on Adaptec with
		interleave=2, 5 heads, leave 5 spare tracks  */
	{
		{ 0,	9792 }, /* 9792 is half of an iom */
		{ 9792,	19584 },
		{ 29376, 19584 },
		{ 48960, 13960 },
		{ 0,	62640 },
		{ 0,	0},
		{ 0,	0},
		{ 0,	0}
	},
	/* table three 3 - Wren Disk with Swap Space */
	{
		{ 0,	9792 }, /* 9792 is half of an iom */
		{ 9792, 2048 },
		{ 11840, 19584 },
		{ 31424, 19584 },
		{ 51008, 11632 },
		{ 0,	62640 },
		{ 0,	0 },
		{ 0,	0 }
	},
	/* table four 4 - */
	{
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 }
	},
	/* table 5 5 - */
	{
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 }
	},
	/* table six 6 - */
	{
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 },
		{ 0,	0 }
	},
	/* table seven 7 - Iomega disk in various flavors */
	{
		{ 0,	18584},	/* First filesys on Iomega fixit, swap at end */
		{ 18584, 1000}, /* Swap space on the fixit disk */
		{ 0,	 9792},	/* First partition on Iomega split disk */
		{ 9792,	19584}, /* Second partition on Iomega split disk */
		{ 0,	 9580}, /* First Partition for old imi backup */
		{ 9580,	19584},	/* Second partition for old imi backup */
		{ 0,	19584}, /* The whole iomega disk */
		{ 0,	0}	/* Nothing */
	}
};

char	imicmd[8];
char	Initdata[8];

struct	buf	imitab;

unsigned	imixblks;	/* Number of blks transferred */
static		int Didinit = 0;

imistrategy(bp)
register struct buf *bp;
{
	register unit;

	int part;
	int tab;

	part = bp->b_dev & PARTMSK;
	tab = (bp->b_dev >> TABPOS) & TABMSK;

/*
	if(bp->b_flags&B_PHYS)
		mapalloc(bp);
*/

	if(bp->b_blkno >= scsitab[tab][part].nblks) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	bp->av_forw = (struct buf *)NULL;
	spl5();
	if(imitab.b_actf == NULL)
		imitab.b_actf = bp;
	else
		imitab.b_actl->av_forw = bp;
	imitab.b_actl = bp;
	if(imitab.b_active == NULL)
		imistart(0);
	spl0();
}
/* zzz */
imistart(cont)
int cont;
{
	register struct buf *bp;
	long baddr;
	register drive;
	int part;
	int tab;
	register nblks;


	if((bp = imitab.b_actf) == NULL)
		return;
	imitab.b_active++;
#ifdef XEBEC
	if(Didinit == 0)
	{
		Initdata[0] = XIMI_NCYL >> 8;
		Initdata[1] = XIMI_NCYL & 0377;
		Initdata[2] = XIMI_NSURF;
		/* Reduce write current - drive proc does this */
		Initdata[3] = Initdata[4] = 0;
		/* Write precomp trk 256 */
		Initdata[5] = 255;
		Initdata[6] = 0;
		/* ECC Burst length */
		Initdata[7] = 11;

		imicmd[0] = INITSZ;

		imi->cmdaddr = imicmd;
		imi->bufaddr = Initdata;
		imi->xcmdaddr = 0;
		imi->xbufaddr = 0;
		imi->csr = GO;

		while((imi->csr & DONE) == 0)
			;

		if(imi->csr < 0)
			printf("imi init error %o\n", imi->csr);


		Didinit = 1;
	}
#endif

	if(cont == 0)
		imixblks = 0;
	part = bp->b_dev & PARTMSK;
	tab = (bp->b_dev >> TABPOS) & TABMSK;

	/* Setup the command block */
	baddr = (bp->b_blkno + scsitab[tab][part].boffset)
		+ imixblks;

	drive = (bp->b_dev >> DRIVPOS) & DRIVMSK;
	imicmd[1] = (drive << 5) | ((baddr >> 16) & 037);
	imicmd[2] = ((baddr >> 8) & 0377);
	imicmd[3] = (baddr & 0377);

	/* Number of blks up to 256 (0) in one operation */
	nblks = ((bp->b_bcount + 0777) >> 9) - imixblks;

	if(nblks > 255)
		imicmd[4] = 0;
	else
		imicmd[4] = nblks;

	imicmd[5] = 0;

	if(bp->b_flags & B_READ)
		imicmd[0] = READ;
	else
		imicmd[0] = WRITE;

	imi->cmdaddr = imicmd;
	imi->xcmdaddr = 0;

	imi->bufaddr = bp->b_un.b_addr;
	imi->xbufaddr = (bp->b_xmem >> 4) & 0x03;

	imi->csr = ((bp->b_xmem & 0x0f) << 2) | INTR | GO;
}

imiintr()
{
	register struct buf *bp;
	register unsigned short stat;
	register unsigned short nstat;
	long bnum;

	if(imitab.b_active == NULL)
		return;

	bp = imitab.b_actf;

	stat = imi->csr;
	if((stat & ERROR))
	{
/*
printf("stat = %o\n", stat);
*/
		if(stat & NONXMEM)
			;
		else
		if(imi->ccsr & CONTROL)		/* Controller or drive error */
		{
			imicmd[0] = RQSENSE;
			imicmd[1] = ((bp->b_dev >> DRIVPOS) & DRIVMSK) <<5;
			imicmd[2] = 0;
			imicmd[3] = 0;
			imicmd[4] = 0;
			imicmd[5] = 0;

			imi->cmdaddr = imi->bufaddr = imicmd;
			imi->xcmdaddr = imi->xbufaddr = 0;
			imi->csr = GO;

			while((imi->csr & DONE) == 0)
				;

			nstat = imicmd[0];
			bnum =(unsigned)imicmd[3] | ((unsigned)imicmd[2] << 8) | ((long)(imicmd[1] & 037) << 16);
			printf("err stat %x\n", nstat&0xffff);
/* 
					This error test is wrong- skip
/*

			if((nstat & CDFE) && (nstat & TYPE1))
			{
				printf("CE errblk,blkno %O,%O\n", bnum,bp->b_blkno);
				goto noerror;
				/* Continue if raw operation
			}
			else
			{
*/
				bp->b_flags |= B_ERROR;
				deverror(bp, nstat, stat);
				printf("errblk,blkno %O,%O\n", bnum,bp->b_blkno);
/*
			}
/*				Skip punctuation conditional of bad error clause
*/
		}

		/* try again */
/*				Should we really be trying again ?
				Can this lead to infinite loop of errors? */

		if((bp->b_flags & B_ERROR) == 0)
		{
			imistart(1);
			return;
		}
	}

noerror:
	imitab.b_errcnt = 0;
	if(imicmd[4] == 0)
		imixblks += 256;
	else
		imixblks += imicmd[4];

	if(((bp->b_flags & B_ERROR) == 0) && ((imixblks << 9) < bp->b_bcount))
	{
		imistart(1);
		return;
	}

	imitab.b_active = 0;
	imitab.b_actf = bp->av_forw;
	bp->b_resid = 0;
	iodone(bp);
	imistart(0);
}
#endif NIMI

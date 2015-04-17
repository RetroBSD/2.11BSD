/*
 *	SCCS id	@(#)hs.c	2.2	1/3/93
 */

/*
 *	RS03/04 disk driver
 */

#ifdef AUTOCONFIG
#include "param.h"
#include "hsreg.h"
#include "autoconfig.h"
hsprobe(addr)
struct hsdevice *addr;
{
	stuff(HS_IE | HS_DCLR | HS_GO, (&(addr->hscs1)));
	WAIT(10);
	stuff(0, (&(addr->hscs1)));
	return(ACP_IFINTR);
}
#else !AUTOCONFIG

#include "hs.h"
#if	NHS > 0
#include "param.h"
#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "user.h"
#include "seg.h"
#include "hsreg.h"

#define	HS_NRS03BLKS	1024
#define	HS_NRS04BLKS	2048

struct	hsdevice *HSADDR = (struct hsdevice *)0172040;

struct	buf	hstab;

hsroot()
{
	hsattach(HSADDR, 0);
}

hsattach(addr, unit)
register struct hsdevice *addr;
{
	if (unit != 0)
		return(0);
	if (fioword(addr) != -1) {
		HSADDR = addr;
		if (fioword(&(addr->hsbae)) != -1)
			hstab.b_flags |= B_RH70;
		return(1);
	}
	HSADDR = (struct hsdevice *) NULL;
	return(0);
}

hsstrategy(bp)
register struct buf *bp;
{
	register s, mblks;

	if (minor(bp->b_dev) < 8)
		mblks = HS_NRS03BLKS;
	else
		mblks = HS_NRS04BLKS;
	if (HSADDR == (struct hsdevice *) NULL) {
		bp->b_error = ENXIO;
		goto errexit;
	}
	if (bp->b_blkno < 0 || bp->b_blkno >= mblks) {
		bp->b_error = EINVAL;
errexit:
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}

	if ((hstab.b_flags & B_RH70) == 0)
		mapalloc(bp);
	bp->av_forw = 0;
	s = spl5();
	if (hstab.b_actf == 0)
		hstab.b_actf = bp;
	else
		hstab.b_actl->av_forw = bp;
	hstab.b_actl = bp;
	if (hstab.b_active == 0)
		hsstart();
	splx(s);
}

hsstart()
{
	register struct hsdevice *hsaddr = HSADDR;
	register struct buf *bp;
	register com_addr;

	if ((bp = hstab.b_actf) == 0)
		return;
	hstab.b_active++;
	com_addr = bp->b_blkno;
	if(minor(bp->b_dev) < 8)
		com_addr <<= 1; /* RJS03 */
	hsaddr->hscs2 = minor(bp->b_dev) & 07;
	hsaddr->hsda = com_addr << 1;
	if (hstab.b_flags & B_RH70)
		hsaddr->hsbae = bp->b_xmem;
	hsaddr->hsba = bp->b_un.b_addr;
	hsaddr->hswc = -(bp->b_bcount >> 1);
	com_addr = HS_IE | HS_GO | ((bp->b_xmem & 03) << 8);
	if(bp->b_flags & B_READ)
		hsaddr->hscs1 = com_addr | HS_RCOM;
	else
		hsaddr->hscs1 = com_addr | HS_WCOM;
#ifdef	HS_DKN
	dk_busy |= 1 << HS_DKN;
	dk_numb[HS_DKN]++;
	dk_wds[HS_DKN] += (bp->b_bcount >> 6) & 01777;
#endif	HS_DKN
}

hsintr()
{
	register struct hsdevice *hsaddr = HSADDR;
	register struct buf *bp;
	register i;

	if (hstab.b_active == 0)
		return;
#ifdef	HS_DKN
	dk_busy &= ~(1 << HS_DKN);
#endif	HS_DKN
	bp = hstab.b_actf;
	hstab.b_active = 0;
	if(hsaddr->hscs1 & HS_TRE) {
#ifdef	UCB_DEVERR
		harderr(bp, "hs");
		printf("cs1=%b cs2=%b\n", hsaddr->hscs1,
			HS_BITS, hsaddr->hscs2, HSCS2_BITS);
#else
		deverror(bp, hsaddr->hscs1, hsaddr->hscs2);
#endif
		hsaddr->hscs1 = HS_DCLR | HS_GO;
		if (++hstab.b_errcnt <= 10) {
			hsstart();
			return;
		}
		bp->b_flags |= B_ERROR;
	}
	hstab.b_errcnt = 0;
	hstab.b_actf = bp->av_forw;
	iodone(bp);
	hsstart();
}
#endif NHS
#endif AUTOCONFIG

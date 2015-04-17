/*
 * ISR 'ramdisk' driver.
 * This driver implements a 'ram disk' in main memory.
 *
 * For the block device, the driver works by remapping buffer header
 * addresses and simply returning.  This way the buffer copy is done only
 * once.  It is faster than any mass storage device in access time and
 * transfer time (both are immediate) and consumes less cpu time in the
 * driver than most.
 *
 * Gregory Travis, Institute for Social Research, September 1984
 * Mucked about, KB, March 1987.
 * Remucked and stomped, Casey Leedom, August 1987.
 */

#include "ram.h"
#if NRAM > 0
#include "param.h"
#include "../machine/seg.h"

#include "buf.h"
#include "user.h"
#include "conf.h"
#include "map.h"
#include "systm.h"
#include "dk.h"

#ifdef UCB_METER
static	int	ram_dkn = -1;		/* number for iostat */
#endif

u_int		ram_size = NRAM;	/* (patchable) ram disk size */
memaddr		ram_base;		/* base of ram disk area */

/* 
 * Allocate core for ramdisk(s)
 */
size_t
raminit()
{
	if (ram_size) {
		if (ram_base = malloc(coremap, ram_size*btoc(NBPG))) {
#ifdef UCB_METER
			dk_alloc(&ram_dkn, 1, "ram", 0L);
#endif
			return(ram_size*btoc(NBPG));
		}
		printf("ram: nospace\n");
	}
	return(0);
}

ramopen(dev)
	dev_t dev;
{
	if (minor(dev) || !ram_base)
		return (ENXIO);
	return (0);
}

ramstrategy(bp)
	register struct buf *bp;
{
	register memaddr ramaddr;

	if (!ram_base) {
		printf("ram: not allocated\n");
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		goto done;
	}
	if(bp->b_blkno < 0 ||
	    ((bp->b_blkno + ((bp->b_bcount+(NBPG-1)) >> PGSHIFT)) > ram_size)) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		goto done;
	}
#ifdef UCB_METER
	if (ram_dkn >= 0) {
		dk_xfer[ram_dkn]++;
		dk_wds[ram_dkn] += bp->b_bcount>>6;
	}
#endif
	ramaddr = ram_base + bp->b_blkno * btoc(NBPG);
	if (bp->b_flags & B_PHYS)
		ramstart(ramaddr, bp);
	else if (bp->b_flags & B_READ) {
#ifdef DIAGNOSTIC
		if(bp->b_flags & B_RAMREMAP)
			panic("ram: remap!");
#endif
		bp->b_flags |= B_RAMREMAP;	/* block read (sic) */
		bp->b_un.b_addr = (caddr_t)(ramaddr << 6);
		bp->b_xmem = (ramaddr >> 10) & 077;
	} else
		if ((bp->b_flags & B_RAMREMAP) == 0)
			ramstart(ramaddr, bp);	/* virgin write */
done:
	iodone(bp);
}

/*
 * Start transfer between ram disk (ramaddr) and buffer.  Actually completes
 * synchronously of course, but what the heck.
 */
ramstart(ramaddr, bp)
	memaddr ramaddr;
	register struct buf *bp;
{
	register u_int n, c;
	memaddr bufaddr;
	caddr_t seg6addr;
	int error;

	if (!(n = bp->b_bcount))
		return;
#ifdef UCB_METER
	if (ram_dkn >= 0)
		dk_busy |= 1 << ram_dkn;
#endif
	bufaddr = bftopaddr(bp);		/* click address of buffer */
	seg6addr = SEG6 + ((u_int)bp->b_un.b_addr & 077);
	c = MIN(n, (SEG6+8192)-seg6addr);
	for (;;) {
		if (bp->b_flags & B_READ) {
			mapseg5(ramaddr, ((btoc(8192)-1)<<8)|RO);
			if (error = fmove(bufaddr, ((btoc(8192)-1)<<8)|RW,
			    SEG5, seg6addr, c))
				break;
		} else {
			mapseg5(ramaddr, ((btoc(8192)-1)<<8)|RW);
			if (error = fmove(bufaddr, ((btoc(8192)-1)<<8)|RO,
			    seg6addr, SEG5, c))
				break;
		}
		if (!(n -= c))
			break;
		ramaddr += btoc(8192);
		bufaddr += btoc(8192);
		seg6addr = SEG6;
		c = MIN(n, 8192);
	}
	normalseg5();
#ifdef UCB_METER
	if (ram_dkn >= 0)
		dk_busy &= ~(1 << ram_dkn);
#endif
	if (error) {
		bp->b_error = error;
		bp->b_flags |= B_ERROR;
	}
}
#endif NRAM

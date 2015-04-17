/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)machdep2.c	2.9 (2.11BSD) 1999/2/19
 */

#include "param.h"
#include "../machine/seg.h"
#include "../machine/iopage.h"

#include "dir.h"
#include "inode.h"
#include "user.h"
#include "proc.h"
#include "fs.h"
#include "map.h"
#include "buf.h"
#include "text.h"
#include "file.h"
#include "clist.h"
#include "uba.h"
#include "callout.h"
#include "reboot.h"
#include "systm.h"
#include "ram.h"
#include "msgbuf.h"
#include "namei.h"
#include "ra.h"
#include "tms.h"
#include "ingres.h"
#include "disklabel.h"
#include "mount.h"

#if	NINGRES > 0
#include <sys/ingreslock.h>
#endif

#ifdef QUOTA
#include "quota.h"
#endif

size_t	physmem;	/* total amount of physical memory (for savecore) */
#if	NRAC > 0 || NTMSCP > 0
memaddr	_iostart, _iobase;
ubadr_t	_ioumr;
u_short	_iosize = ((NTMSCP * (ctob(btoc(1864)))) + (NRAC * (ctob(btoc(1096)))));
#endif

#ifdef	SOFUB_MAP
extern	size_t	sofub_addr, sofub_off;
extern	memaddr	sofub_base;
extern	u_int	sofub_size;
#endif

segm	seg5;		/* filled in by initialization */

/*
 * Machine dependent startup code
 */
startup()
{
#ifdef UCB_CLIST
	extern memaddr clststrt;
#endif
	extern ubadr_t	clstaddr;
	extern int end;
	register memaddr i, freebase, maxclick;
#if NRAM > 0
	size_t ramsize;
#endif

	printf("\n%s\n", version);

	saveseg5(seg5);		/* must be done before clear() is called */
	/*
	 * REMAP_AREA is the start of possibly-mapped area, for consistency
	 * check.  Only proc, text and file tables are after it, and it must
	 * lie at <= 0120000, or other kernel data will be mapped out.
	 */
	if (REMAP_AREA > SEG5)
		panic("remap > SEG5");

	/*
	 * Zero and free all of core:
	 *
	 * MAXCLICK is the maximum accessible physical memory, assuming an 8K
	 * I/O page.  On systems without a Unibus map the end of memory is
	 * heralded by the beginning of the I/O page (some people have dz's
	 * at 0160000).  On systems with a Unibus map, the last 256K of the
	 * 4M address space is off limits since 017000000 to 017777777 is the
	 * actual 18 bit Unibus address space.  61440 is btoc(4M - 256K), 
	 * and 65408 is btoc(4M - 8K).
	 *
	 * Previous cautions about 18bit devices on a 22bit Qbus were misguided.
	 * Since the GENERIC kernel was built with Q22 defined the limiting
	 * effect on memory size was not achieved, thus an 18bit controller
	 * could not be used to load the distribution.  ALSO, the kernel
	 * plus associated data structures do not leave enough room in 248kb
	 * to run the programs necessary to do _anything_.
	 */
#define MAXCLICK_22U	61440		/* 22 bit UNIBUS (UNIBUS mapping) */
#define MAXCLICK_22	65408		/* 22 bit QBUS */

	maxclick = ubmap ? MAXCLICK_22U : MAXCLICK_22;

	i = freebase = *ka6 + USIZE;
	UISD[0] = ((stoc(1) - 1) << 8) | RW;
	for (;;) {
		UISA[0] = i;
		if (fuibyte((caddr_t)0) < 0)
			break;
		++maxmem;
		/* avoid testing locations past "real" memory. */
		if (++i >= maxclick)
			break;
	}
	clear(freebase,i - freebase);
	mem_parity();			/* enable parity checking */
	clear(freebase,i - freebase);	/* quick check for parities */
	mfree(coremap,i - freebase,freebase);
	physmem = i;

	procNPROC = proc + nproc;
	textNTEXT = text + ntext;
	inodeNINODE = inode + ninode;
	fileNFILE = file + nfile;

	/*
	 * IMPORTANT! Mapped out clists should always be allocated first!
	 * This prevents needlessly having to restrict all memory use
	 * (maxclick) to 248K just because an 18-bit DH is present on a
	 * 22-bit Q-BUS machine.  The maximum possible location for mapped
	 * out clists this way is 232K (56K base + 15 * 8K overlays + 48K
	 * data space + 8K (maximum) user structure, which puts the maximum
	 * top of mapped out clists at 240K ...
	 */
#ifdef UCB_CLIST
#define C	(nclist * sizeof(struct cblock))
	if ((clststrt = malloc(coremap, btoc(C))) == 0)
		panic("clists");
	clstaddr = ((ubadr_t)clststrt) << 6;
#undef C
#else
	clstaddr = (ubadr_t)cfree;
#endif

/*
 * IMPORTANT.  The software Unibus/Qbus map is allocated now if support for
 * 18 bit controllers in a 22 bit system has been selected.  This buffer must
 * reside _entirely_ within the low 256kb of memory.  A 10kb buffer is 
 * allocated, this is sufficient to handle 'dump', 'restor' and the default
 * blocking factor of 'tar' (20 sectors).
 *
 * NOTE:  There is only 1 software map.  Multiple 18 bit controllers will
 * have their access to the 'bounce buffer' single threaded by the soft
 * map allocation routine sofub_alloc() in machdep.c.
 *
 * For more details see machdep.c.
*/

#ifdef	SOFUB_MAP
#define	B	(10240+64)

	sofub_size = (unsigned) B;

	if	((sofub_base = malloc(coremap, btoc(B))) == 0)
		panic("sofmap");			/* Paranoia */
	else if	(((sofub_base + btoc(B)) >> 10) > 3)	/* > 256kb! */
		{
		printf("sofmap > 256kb\n");
		mfree(coremap, btoc(B), sofub_base);	/* give it back */
		sofub_base = 0;
		}
	else
		{
		sofub_addr = sofub_base;
		sofub_off = (sofub_base>>10)&3;
		}
#undef	B
#endif /* SOFUB_MAP */

#ifdef EXTERNALITIMES
#define C (btoc(ninode * sizeof (struct icommon2)))
	if ((xitimes = malloc(coremap, C)) == 0)
		panic("xitimes");
	xitdesc = ((C - 1) << 8) | RW;
#undef C
#endif

#ifdef QUOTA
#define	C	(btoc(8192))
	if ((quotreg = malloc(coremap, C)) == 0)
		panic("quotamem");
	quotdesc = ((C - 1) << 8) | RW;
	QUOini();
#undef C
#endif

	{
register int B;

	nchsize = 8192 / sizeof(struct namecache);
	if (nchsize > (ninode * 11 / 10))
		nchsize = ninode * 11 / 10;
	B = (btoc(nchsize * sizeof(struct namecache)));
	if ((nmidesc.se_addr = malloc(coremap, B)) == 0)
		panic("nmidesc");
	nmidesc.se_desc = ((B - 1) << 8) | RW;
	namecache = (struct namecache *)SEG5;
	}

#if	NRAC > 0 || NTMSCP > 0
	if ((_iobase = malloc(coremap, btoc(_iosize))) == 0)
		panic("_iobase");
#endif

#define B	(size_t)(((long)nbuf * (MAXBSIZE)) / ctob(1))
	if ((bpaddr = malloc(coremap, B)) == 0)
		panic("buffers");
#undef B

/*
 * Now initialize the log driver (kernel logger, error logger and accounting)
*/
	loginit();

#define	C	(btoc(sizeof (struct xmount)))
	for	(i = 0; i < NMOUNT; i++)
		mount[i].m_extern = (memaddr)malloc(coremap, C);
#undef	C

#if NINGRES > 0
#define	C	(btoc(LOCKTABSIZE))

	if (Locktabseg.se_addr = malloc(coremap, C))
		Locktabseg.se_desc = ((C - 1) << 8) | RW;
#undef  C
#endif

/*
 * Allocate the initial disklabels.
*/
	(void) initdisklabels();

#if NRAM > 0
	ramsize = raminit();
#endif

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	UISA[7] = ka6[1];			/* io segment */
	UISD[7] = ((stoc(1) - 1) << 8) | RW;
}

mem_parity()
{
	register int cnt;

	for (cnt = 0;cnt < 16;++cnt) {
		if (fioword((caddr_t)(MEMSYSMCR+cnt)) == -1)
			return;
		*(MEMSYSMCR+cnt) = MEMMCR_EIE;	/* enable parity interrupts */
	}
}

#if defined(PROFILE) && !defined(ENABLE34)
/*
 * Allocate memory for system profiling.  Called once at boot time.
 * Returns number of clicks used by profiling.
 *
 * The system profiler uses supervisor I space registers 2 and 3
 * (virtual addresses 040000 through 0100000) to hold the profile.
 */
msprof()
{
	memaddr proloc;
	int nproclicks;

	nproclicks = btoc(8192*2);
	proloc = malloc(coremap, nproclicks);
	if (proloc == 0)
		panic("msprof");

	*SISA2 = proloc;
	*SISA3 = proloc + btoc(8192);
	*SISD2 = 077400|RW;
	*SISD3 = 077400|RW;
	*SISD0 = RW;
	*SISD1 = RW;

	/*
	 * Enable system profiling.  Zero out the profile buffer
	 * and then turn the clock (KW11-P) on.
	 */
	clear(proloc, nproclicks);
	isprof();
	printf("profiling on\n");

	return (nproclicks);
}
#endif

/*
 * Re-initialize the Unibus map registers to statically map
 * the clists and buffers.  Free the remaining registers for
 * physical I/O.  At this time the [T]MSCP arena is also mapped.
 */
ubinit()
{
	register int i, ub_nreg;
	long paddr;
	register struct ubmap *ubp;

	if (!ubmap)
		return;
	/*
	 * Clists start at UNIBUS virtual address 0.  The size of
	 * the clist segment can be no larger than UBPAGE bytes.
	 * Clstaddt was the physical address of clists.
	 */
	if (nclist * sizeof(struct cblock) > ctob(stoc(1)))
		panic("clist > 8k");
	setubregno(0, clstaddr);
	clstaddr = (ubadr_t)0;

	/*
	 * Buffers start at UNIBUS virtual address BUF_UBADDR.
	 */
	paddr = ((long)bpaddr) << 6;
	ub_nreg = nubreg(nbuf, MAXBSIZE);
	for (i = BUF_UBADDR/UBPAGE; i < ub_nreg + (BUF_UBADDR/UBPAGE); i++) {
		setubregno(i, paddr);
		paddr += (long)UBPAGE;
	}
	/*
	 * The 3Com ethernet board is hardwired to use UNIBUS registers 28, 29
	 * and 30 (counting from 0) and UNIBUS register 31 isn't usable.
	 */
#include "ec.h"
#if NEC > 0
	mfree(ub_map, 28 - ub_nreg - 1, 1 + ub_nreg);	/* 3Com board */
#else
	mfree(ub_map, 31 - ub_nreg - 1, 1 + ub_nreg);
#endif

/*
 * this early in the system's life there had better be a UMR or two
 * available!!  N.B. This was moved from where the [T]MSCP memory was 
 * allocated because at that point the UMR map was not initialized.
*/

#if	NRAC > 0 || NTMSCP > 0
	_iostart = _iobase;
	i = (int)btoub(_iosize);
	ub_nreg = malloc(ub_map, i);
	_ioumr = (ubadr_t)ub_nreg << 13;
	ubp = &UBMAP[ub_nreg];
	paddr = ctob((ubadr_t)_iostart);
	while (i--) {
		ubp->ub_lo = loint(paddr);
		ubp->ub_hi = hiint(paddr);
		ubp++;
		paddr += (ubadr_t)UBPAGE;
	}
#endif
}

int waittime = -1;

boot(dev, howto)
	register dev_t dev;
	register int howto;
{
	register struct fs *fp;

	/*
	 * Force the root filesystem's superblock to be updated,
	 * so the date will be as current as possible after
	 * rebooting.
	 */
	if (fp = getfs(rootdev))
		fp->fs_fmod = 1;
	if ((howto&RB_NOSYNC)==0 && waittime < 0 && bfreelist[0].b_forw) {
		waittime = 0;
		printf("syncing disks... ");
		(void) _splnet();
		/*
		 * Release inodes held by texts before update.
		 */
		xumount(NODEV);
		sync();
		{ register struct buf *bp;
		  int iter, nbusy;

		  for (iter = 0; iter < 20; iter++) {
			nbusy = 0;
			for (bp = &buf[nbuf]; --bp >= buf; )
				if (bp->b_flags & B_BUSY)
					nbusy++;
			if (nbusy == 0)
				break;
			printf("%d ", nbusy);
			delay(40000L * iter);
		  }
		}
		printf("done\n");
	}
	(void) _splhigh();
	if (howto & RB_HALT) {
		printf("halting\n");
		halt();
		/*NOTREACHED*/
	}
	else {
		if (howto & RB_DUMP) {
			/*
			 * save the registers in low core.
			 */
			saveregs();
			dumpsys();
		}
		doboot(dev, howto);
		/*NOTREACHED*/
	}
}

/*
 * Dumpsys takes a dump of memory by calling (*dump)(), which must
 * correspond to dumpdev.  *(dump)() should dump from dumplo blocks
 * to the end of memory or to the end of the logical device.
 */
dumpsys()
{
	extern int (*dump)();
	register int error;

	if (dumpdev != NODEV) {
		printf("\ndumping to dev %o off %D\ndump ",dumpdev,dumplo);
		error = (*dump)(dumpdev);
		switch(error) {

		case EFAULT:
			printf("dev !ready:EFAULT\n");
			break;
		case EINVAL:
			printf("args:EINVAL\n");
			break;
		case EIO:
			printf("err:EIO\n");
			break;
		default:
			printf("unknown err:%d\n",error);
			break;
		case 0:
			printf("succeeded\n");
			break;
		}
	}
}

#if	NRAC > 0 || NTMSCP > 0
memaddr
_ioget(size)
	u_int size;
	{
	register memaddr base;
	register u_int csize;

	csize = btoc(size);
	size = ctob(csize);
	if (size > _iosize)
		return(0);
	_iosize -= size;
	base = _iobase;
	_iobase += csize;
	return(base);
	}

ubadr_t
_iomap(addr)
	register memaddr addr;
	{
	return(((ubadr_t)(addr - _iostart) << 6) + _ioumr);
	}
#endif

#define	NLABELS	6

	memaddr	_dlabelbase;
	int	_dlabelnum = NLABELS;

void
initdisklabels()
	{
#define	C	(NLABELS * (btoc(sizeof (struct disklabel))))

	_dlabelbase = malloc(coremap, C);
	}

memaddr
disklabelalloc()
	{
	register memaddr base;

	if	(--_dlabelnum)
		{
		base = _dlabelbase;
		_dlabelbase += btoc(sizeof (struct disklabel));
		return(base);
		}
	base = malloc(coremap, btoc (sizeof (struct disklabel)));
	return(base);
	}

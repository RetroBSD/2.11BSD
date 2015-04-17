/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)init_main.c	2.5 (2.11BSD GTE) 1997/9/26
 */

#include "param.h"
#include "../machine/seg.h"

#include "user.h"
#include "fs.h"
#include "mount.h"
#include "map.h"
#include "proc.h"
#include "ioctl.h"
#include "inode.h"
#include "conf.h"
#include "buf.h"
#include "fcntl.h"
#include "vm.h"
#include "clist.h"
#include "uba.h"
#include "reboot.h"
#include "systm.h"
#include "kernel.h"
#include "namei.h"
#include "disklabel.h"
#include "stat.h"
#ifdef QUOTA
#include "quota.h"
#endif

int	netoff = 1;
int	cmask = CMASK;
int	securelevel;

extern	size_t physmem;
extern	struct	mapent _coremap[];

/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 1 execute bootstrap
 */
main()
{
	extern dev_t bootdev;
	extern caddr_t bootcsr;
	register struct proc *p;
	register int i;
	register struct fs *fs;
	time_t  toytime, toyclk();
	daddr_t swsize;
	int	(*ioctl)();
	struct	partinfo dpart;

	startup();

	/*
	 * set up system process 0 (swapper)
	 */
	p = &proc[0];
	p->p_addr = *ka6;
	p->p_stat = SRUN;
	p->p_flag |= SLOAD|SSYS;
	p->p_nice = NZERO;

	u.u_procp = p;			/* init user structure */
	u.u_ap = u.u_arg;
	u.u_cmask = cmask;
	u.u_lastfile = -1;
	for (i = 1; i < NGROUPS; i++)
		u.u_groups[i] = NOGROUP;
	for (i = 0; i < sizeof(u.u_rlimit)/sizeof(u.u_rlimit[0]); i++)
		u.u_rlimit[i].rlim_cur = u.u_rlimit[i].rlim_max = 
		    RLIM_INFINITY;
	bcopy("root", u.u_login, sizeof ("root"));

	/* Initialize signal state for process 0 */
	siginit(p);

	/*
	 * Initialize tables, protocols, and set up well-known inodes.
	 */
	cinit();
	pqinit();
	xinit();
	ihinit();
	bhinit();
	binit();
	ubinit();
#ifdef QUOTA
	QUOTAMAP();
	qtinit();
	u.u_quota = getquota(0, 0, Q_NDQ);
	QUOTAUNMAP();
#endif
	nchinit();
	clkstart();

/*
 * If the kernel is configured for the boot/load device AND the use of the
 * compiled in 'bootdev' has not been overridden (by turning on RB_DFLTROOT,
 * see conf/boot.c for details) THEN switch 'rootdev', 'swapdev' and 'pipedev'
 * over to the boot/load device.  Set 'pipedev' to be 'rootdev'.
 *
 * The &077 removes the controller number (bits 6 and 7) - those bits are 
 * passed thru from /boot but would only greatly confuse the rest of the kernel.
*/
	i = major(bootdev);
	if	((bdevsw[i].d_strategy != nodev) && !(boothowto & RB_DFLTROOT))
		{
		rootdev = makedev(i, minor(bootdev) & 077);
		swapdev = rootdev | 1;	/* partition 'b' */
		pipedev = rootdev;
/*
 * We check that the dump device is the same as the boot device.  If it is 
 * different then it is likely that crashdumps go to a tape device rather than 
 * the swap area.  In that case do not switch the dump device.
*/
		if	((dumpdev != NODEV) && major(dumpdev) == i)
			dumpdev = swapdev;
		}

/*
 * Need to attach the root device.  The CSR is passed thru because this
 * may be a 2nd or 3rd controller rather than the 1st.  NOTE: This poses
 * a big problem if 'swapdev' is not on the same controller as 'rootdev'
 * _or_ if 'swapdev' itself is on a 2nd or 3rd controller.  Short of moving
 * autconfigure back in to the kernel it is not known what can be done about
 * this.
 *
 * One solution (for now) is to call swapdev's attach routine with a zero
 * address.  The MSCP driver treats the 0 as a signal to perform the
 * old (fixed address) attach.  Drivers (all the rest at this point) which
 * do not support alternate controller booting always attach the first
 * (primary) CSR and do not expect an argument to be passed.
*/
	(void)(*bdevsw[major(bootdev)].d_root)(bootcsr);
	(void)(*bdevsw[major(swapdev)].d_root)((caddr_t) 0);	/* XXX */

/*
 * Now we find out how much swap space is available.  Since 'nswap' is
 * a "u_int" we have to restrict the amount of swap to 65535 sectors (~32mb).
 * Considering that 4mb is the maximum physical memory capacity of a pdp-11
 * 32mb swap should be enough ;-)
 *
 * The initialization of the swap map was moved here from machdep2.c because
 * 'nswap' was no longer statically defined and this is where the swap dev
 * is opened/initialized.
 *
 * Also, we toss away/ignore .5kb (1 sector) of swap space (because a 0 value
 * can not be placed in a resource map).
 *
 * 'swplo' was a hack which has _finally_ gone away!  It was never anything
 * but 0 and caused a number of double word adds in the kernel.
*/
	(*bdevsw[major(swapdev)].d_open)(swapdev, FREAD|FWRITE, S_IFBLK);
	swsize = (*bdevsw[major(swapdev)].d_psize)(swapdev);
	if	(swsize <= 0)
		panic("swsiz");		/* don't want to panic, but what ? */

/*
 * Next we make sure that we do not swap on a partition unless it is of
 * type FS_SWAP.  If the driver does not have an ioctl entry point or if
 * retrieving the partition information fails then the driver does not 
 * support labels and we proceed normally, otherwise the partition must be
 * a swap partition (so that we do not swap on top of a filesystem by mistake).
*/
	ioctl = cdevsw[blktochr(swapdev)].d_ioctl;
	if	(ioctl && !(*ioctl)(swapdev, DIOCGPART, (caddr_t)&dpart, FREAD))
		{
		if	(dpart.part->p_fstype != FS_SWAP)
			panic("swtyp");
		}
	if	(swsize > (daddr_t)65535)
		swsize = 65535;
	nswap = swsize;
	mfree(swapmap, --nswap, 1);

	fs = mountfs(rootdev, boothowto & RB_RDONLY ? MNT_RDONLY : 0,
			(struct inode *)0);
	if (!fs)
		panic("iinit");
	mount[0].m_inodp = (struct inode *)1;	/* XXX */
	mount_updname(fs, "/", "root", 1, 4);
	time.tv_sec = fs->fs_time;
	if	(toytime = toyclk())
		time.tv_sec = toytime;
	boottime = time;

/* kick off timeout driven events by calling first time */
	schedcpu();

/* set up the root file system */
	rootdir = iget(rootdev, &mount[0].m_filsys, (ino_t)ROOTINO);
	iunlock(rootdir);
	u.u_cdir = iget(rootdev, &mount[0].m_filsys, (ino_t)ROOTINO);
	iunlock(u.u_cdir);
	u.u_rdir = NULL;

#ifdef INET
	if (netoff = netinit())
		printf("netinit failed\n");
	else
		{
		NETSETHZ();
		NETSTART();
		}
#endif

/*
 * This came from pdp/machdep2.c because the memory available statements
 * were being made _before_ memory for the networking code was allocated.
 * A side effect of moving this code is that network "attach" and MSCP 
 * "online" messages can appear before the memory sizes.  The (currently
 * safe) assumption is made that no 'free' calls are made so that the
 * size in the first entry of the core map is correct.
*/
	printf("\nphys mem  = %D\n", ctob((long)physmem));
	printf("avail mem = %D\n", ctob((long)_coremap[0].m_size));
	maxmem = MAXMEM;
	printf("user mem  = %D\n", ctob((long)MAXMEM));
#if NRAM > 0
	printf("ram disk  = %D\n", ctob((long)ramsize));
#endif
	printf("\n");

	/*
	 * make init process
	 */
	if (newproc(0)) {
		expand((int)btoc(szicode), S_DATA);
		expand((int)1, S_STACK);	/* one click of stack */
		estabur((u_int)0, (u_int)btoc(szicode), (u_int)1, 0, RO);
		copyout((caddr_t)icode, (caddr_t)0, szicode);
		/*
		 * return goes to location 0 of user init code
		 * just copied out.
		 */
		return;
	}
	else
		sched();
}

/*
 * Initialize hash links for buffers.
 */
static
bhinit()
{
	register int i;
	register struct bufhd *bp;

	for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
		bp->b_forw = bp->b_back = (struct buf *)bp;
}

memaddr	bpaddr;		/* physical click-address of buffers */
/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
static
binit()
{
	register struct buf *bp;
	register int i;
	long paddr;

	for (bp = bfreelist; bp < &bfreelist[BQUEUES]; bp++)
		bp->b_forw = bp->b_back = bp->av_forw = bp->av_back = bp;
	paddr = ((long)bpaddr) << 6;
	for (i = 0; i < nbuf; i++, paddr += MAXBSIZE) {
		bp = &buf[i];
		bp->b_dev = NODEV;
		bp->b_bcount = 0;
		bp->b_un.b_addr = (caddr_t)loint(paddr);
		bp->b_xmem = hiint(paddr);
		binshash(bp, &bfreelist[BQ_AGE]);
		bp->b_flags = B_BUSY|B_INVAL;
		brelse(bp);
	}
}

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
static
cinit()
{
	register int ccp;
	register struct cblock *cp;

	ccp = (int)cfree;
#ifdef UCB_CLIST
	mapseg5(clststrt, clstdesc);	/* don't save, we know it's normal */
#else
	ccp = (ccp + CROUND) & ~CROUND;
#endif
	for (cp = (struct cblock *)ccp; cp <= &cfree[nclist - 1]; cp++) {
		cp->c_next = cfreelist;
		cfreelist = cp;
		cfreecount += CBSIZE;
	}
#ifdef UCB_CLIST
	normalseg5();
#endif
}

#ifdef INET
memaddr netdata;		/* click address of start of net data */

/*
 * We are called here after all the other init routines (clist, inode,
 * unibusmap, etc...) have been called.  Open the
 * file NETNIX and read the a.out header, based on that go allocate
 * memory and read the text+data into the memory.  Set up supervisor page
 * registers, SDSA6 and SDSA7 have already been set up in mch_start.s.
 */

static char NETNIX[] = "/netnix";

static
netinit()
{
	register u_short *ap, *dp;
	register int i;
	struct exec ex;
	struct inode *ip;
	memaddr nettext;
	long lsize;
	off_t	off;
	int initdata, netdsize, nettsize, ret, err, resid;
	char oneclick[ctob(1)];
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	ret = 1;
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_SYSSPACE, NETNIX);
	if (!(ip = namei(ndp))) {
		printf("%s not found\n", NETNIX);
		goto leave;
	}
	if ((ip->i_mode & IFMT) != IFREG || !ip->i_size) {
		printf("%s bad inode\n", NETNIX);
		goto leave;
	}
	err = rdwri(UIO_READ, ip, &ex, sizeof (ex), (off_t)0, UIO_SYSSPACE,
			IO_UNIT, &resid);
	if (err || resid) {
		printf("%s header %d\n", NETNIX, ret);
		goto leave;
	}
	if (ex.a_magic != A_MAGIC3) {
		printf("%s bad magic %o\n", NETNIX, ex.a_magic);
		goto leave;
	}
	lsize = (long)ex.a_data + (long)ex.a_bss;
	if (lsize > 48L * 1024L) {
		printf("%s 2big %ld\n", NETNIX, lsize);
		goto leave;
	}
	nettsize = btoc(ex.a_text);
	nettext = (memaddr)malloc(coremap, nettsize);
	netdsize = btoc(ex.a_data + ex.a_bss);
	netdata = (memaddr)malloc(coremap, netdsize);
	initdata = ex.a_data >> 6;
	off = sizeof (ex);
	for (i = 0; i < nettsize; i++) {
		err = rdwri(UIO_READ, ip, oneclick, ctob(1), off, UIO_SYSSPACE,
				IO_UNIT, &resid);
		if (err || resid)
			goto release;
		mapseg5(nettext + i, 077406);
		bcopy(oneclick, SEG5, ctob(1));
		off += ctob(1);
		normalseg5();
	}
	for (i = 0; i < initdata; i++) {
		err = rdwri(UIO_READ, ip, oneclick, ctob(1), off, UIO_SYSSPACE,
				IO_UNIT, &resid);
		if (err || resid)
			goto release;
		mapseg5(netdata + i, 077406);
		bcopy(oneclick, SEG5, ctob(1));
		normalseg5();
		off += ctob(1);
	}
	if (ex.a_data & 077) {
		err = rdwri(UIO_READ, ip, oneclick, ex.a_data & 077, off,
				UIO_SYSSPACE, IO_UNIT, &resid);
		if (err || resid) {
release:		printf("%s err %d\n", NETNIX, err);
			mfree(coremap, nettsize, nettext);
			mfree(coremap, netdsize, netdata);
			nettsize = netdsize = 0;
			netdata = nettext = 0;
			goto leave;
		}
		mapseg5(netdata + i, 077406);	/* i is set from above loop */
		bcopy(oneclick, SEG5, ex.a_data & 077);
		normalseg5();
	}
	for (i = 0, ap = SISA0, dp = SISD0; i < nettsize; i += stoc(1)) {
		*ap++ = nettext + i;
		*dp++ = ((stoc(1) - 1) << 8) | RO;
	}
	/* might have over run the length on the last one, patch it now */
	if (i > nettsize)
		*--dp -= ((i - nettsize) << 8);
	for (i = 0, ap = SDSA0, dp = SDSD0; i < netdsize; i += stoc(1)) {
		*ap++ = netdata + i;
		*dp++ = ((stoc(1) - 1) << 8) | RW;
	}
	if (i > netdsize)
		*--dp -= ((i - netdsize) << 8);
	ret = 0;
leave:	if (ip)
		iput(ip);
	u.u_error = 0;
	return(ret);
}
#endif

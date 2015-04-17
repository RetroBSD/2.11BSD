/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)conf.c	3.2 (2.11BSD GTE) 1997/11/12
 */

#include "param.h"
#include "conf.h"
#include "buf.h"
#include "time.h"
#include "ioctl.h"
#include "resource.h"
#include "inode.h"
#include "proc.h"
#include "clist.h"
#include "tty.h"

int	nulldev();
int	nodev();
int	rawrw();

#include "rk.h"
#if NRK > 0
int	rkopen(), rkstrategy();
daddr_t	rksize();
#define	rkclose		nulldev
#else
#define	rkopen		nodev
#define	rkclose		nodev
#define	rkstrategy	nodev
#define	rksize		NULL
#endif

#include "tm.h"
#if NTM > 0
int	tmopen(), tmclose(), tmioctl(), tmstrategy();
#else
#define	tmopen		nodev
#define	tmclose		nodev
#define	tmioctl		nodev
#define	tmstrategy	nodev
#endif

#include "hk.h"
#if NHK > 0
int	hkopen(), hkstrategy(), hkroot(), hkclose();
daddr_t	hksize();
#else
#define	hkopen		nodev
#define	hkclose		nodev
#define	hkroot		nulldev
#define	hkstrategy	nodev
#define	hksize		NULL
#endif

#include "xp.h"
#if NXPD > 0
int	xpopen(), xpstrategy(), xproot(), xpclose(), xpioctl();
daddr_t	xpsize();
#else
#define	xpopen		nodev
#define	xpclose		nodev
#define	xpioctl		nodev
#define	xproot		nulldev
#define	xpstrategy	nodev
#define	xpsize		NULL
#endif

#include "br.h"
#if NBR > 0
int	bropen(), brstrategy(), brroot();
daddr_t	brsize();
#define	brclose		nulldev
#else
#define	bropen		nodev
#define	brclose		nodev
#define	brroot		nulldev
#define	brstrategy	nodev
#define	brsize		NULL
#endif

#include "ht.h"
#if NHT > 0
int	htopen(), htclose(), htstrategy(), htioctl();
#else
#define	htopen		nodev
#define	htclose		nodev
#define	htioctl		nodev
#define	htstrategy	nodev
#endif

#include "rl.h"
#if NRL > 0
int	rlopen(), rlstrategy(), rlroot(), rlclose(), rlioctl();
daddr_t	rlsize();
#else
#define	rlroot		nulldev
#define	rlopen		nodev
#define	rlclose		nodev
#define	rlioctl		nodev
#define	rlstrategy	nodev
#define	rlsize		NULL
#endif

#include "ts.h"
#if NTS > 0
int	tsopen(), tsclose(), tsstrategy(), tsioctl();
#else
#define	tsopen		nodev
#define	tsclose		nodev
#define	tsioctl		nodev
#define	tsstrategy	nodev
#endif

#include "tms.h"
#if NTMS > 0
int	tmscpopen(), tmscpclose(), tmscpstrategy(), tmscpioctl();
#else
#define	tmscpopen	nodev
#define	tmscpclose	nodev
#define	tmscpioctl	nodev
#define	tmscpstrategy	nodev
#endif

#include "si.h"
#if NSI > 0
int	siopen(), sistrategy(), siroot();
daddr_t	sisize();
#define	siclose		nulldev
#else
#define	siopen		nodev
#define	siclose		nodev
#define	siroot		nulldev
#define	sistrategy	nodev
#define	sisize		NULL
#endif

#include "ra.h"
#if NRAC > 0
int	rastrategy(), raroot(), raopen(), raclose(), raioctl();
daddr_t	rasize();
#else
#define	raopen		nodev
#define	raclose		nodev
#define	raioctl		nodev
#define	raroot		nulldev
#define	rastrategy	nodev
#define	rasize		nodev
#endif

#include "rx.h"
#if NRX > 0
int	rxopen(), rxstrategy(), rxioctl();
#define	rxclose		nulldev
#else
#define	rxopen		nodev
#define	rxclose		nodev
#define	rxstrategy	nodev
#define	rxioctl		nodev
#endif

#include "ram.h"
#if NRAM > 0
int	ramopen(), ramstrategy();
#define ramclose	nulldev
#else
#define ramopen		nodev
#define ramclose	nodev
#define ramstrategy	nodev
#endif

struct bdevsw	bdevsw[] = {
/* ht = 0 */
	htopen,		htclose,	htstrategy,	nulldev,	NULL,
	B_TAPE,
/* tm = 1 */
	tmopen,		tmclose,	tmstrategy,	nulldev,	NULL,
	B_TAPE,
/* ts = 2 */
	tsopen,		tsclose,	tsstrategy,	nulldev,	NULL,
	B_TAPE,
/* ram = 3 */
	ramopen,	ramclose,	ramstrategy,	nulldev,	NULL,
	0,
/* hk = 4 */
	hkopen,		hkclose,	hkstrategy,	hkroot,		hksize,
	0,
/* ra = 5 */
	raopen,		raclose,	rastrategy,	raroot,		rasize,
	0,
/* rk = 6 */
	rkopen,		rkclose,	rkstrategy,	nulldev,	rksize,
	0,
/* rl = 7 */
	rlopen,		rlclose,	rlstrategy,	rlroot,		rlsize,
	0,
/* rx = 8 */
	rxopen,		rxclose,	rxstrategy,	nulldev,	NULL,
	0,
/* si = 9 */
	siopen,		siclose,	sistrategy,	siroot,		sisize,
	0,
/* xp = 10 */
	xpopen,		xpclose,	xpstrategy,	xproot,		xpsize,
	0,
/* br = 11 */
	bropen,		brclose,	brstrategy,	brroot,		brsize,
	0,
/* tmscp = 12 (tu81/tk50) */
	tmscpopen,	tmscpclose,	tmscpstrategy,	nulldev,	NULL,
	B_TAPE,
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

int	cnopen(), cnclose(), cnread(), cnwrite(), cnioctl();
extern struct tty cons[];

#include "lp.h"
#if NLP > 0
int	lpopen(), lpclose(), lpwrite();
#else
#define	lpopen		nodev
#define	lpclose		nodev
#define	lpwrite		nodev
#endif

#include "dh.h"
#if NDH > 0
int	dhopen(), dhclose(), dhread(), dhwrite(), dhioctl(), dhstop();
int	dhselect();
extern struct tty	dh11[];
#else
#define	dhopen		nodev
#define	dhclose		nodev
#define	dhread		nodev
#define	dhwrite		nodev
#define	dhioctl		nodev
#define	dhstop		nodev
#define	dhselect	nodev
#define	dh11		((struct tty *) NULL)
#endif

#include "dz.h"
#if NDZ > 0
int	dzopen(), dzclose(), dzread(), dzwrite(), dzioctl();
int	dzstop();
extern	struct	tty	dz_tty[];
#else
#define	dzopen		nodev
#define	dzclose		nodev
#define	dzread		nodev
#define	dzwrite		nodev
#define	dzioctl		nodev
#define	dzstop		nodev
#define	dz_tty		((struct tty *) NULL)
#endif

#include "pty.h"
#if NPTY > 0
int	ptsopen(), ptsclose(), ptsread(), ptswrite(), ptsstop();
int	ptcopen(), ptcclose(), ptcread(), ptcwrite(), ptyioctl();
int	ptcselect();
extern	struct tty pt_tty[];
#else
#define	ptsopen		nodev
#define	ptsclose	nodev
#define	ptsread		nodev
#define	ptswrite	nodev
#define	ptsstop		nodev
#define	ptcopen		nodev
#define	ptcclose	nodev
#define	ptcread		nodev
#define	ptcwrite	nodev
#define	ptyioctl	nodev
#define	ptcselect	nodev
#define	pt_tty		((struct tty *)NULL)
#endif

#include "dr.h"
#if NDR > 0
int	dropen(), drclose(), drioctl(), drstrategy();
#else
#define	dropen		nodev
#define	drclose		nodev
#define	drioctl		nodev
#define	drstrategy	nodev
#endif

#include "dhu.h"
#if NDHU > 0
int	dhuopen(), dhuclose(), dhuread(), dhuwrite(), dhuioctl(), dhustop();
int	dhuselect();
extern struct tty	dhu_tty[];
#else
#define	dhuopen		nodev
#define	dhuclose	nodev
#define	dhuread		nodev
#define	dhuwrite	nodev
#define	dhuioctl	nodev
#define	dhustop		nodev
#define	dhuselect	nodev
#define	dhu_tty		((struct tty *) NULL)
#endif

#include "dhv.h"
#if NDHV > 0
int	dhvopen(), dhvclose(), dhvread(), dhvwrite(), dhvioctl(), dhvstop();
int	dhvselect();
extern struct tty	dhv_tty[];
#else
#define	dhvopen		nodev
#define	dhvclose	nodev
#define	dhvread		nodev
#define	dhvwrite	nodev
#define	dhvioctl	nodev
#define	dhvstop		nodev
#define	dhvselect	nodev
#define	dhv_tty		((struct tty *) NULL)
#endif

#include "dn.h"
#if NDN > 0
int	dnopen(), dnclose(), dnwrite();
#define	dnread		nodev
#define	dnioctl		nodev
#else
#define	dnopen		nodev
#define	dnclose		nodev
#define	dnread		nodev
#define	dnwrite		nodev
#define	dnioctl		nodev
#endif

int	logopen(), logclose(), logread(), logioctl(), logselect();
int	syopen(), syread(), sywrite(), syioctl(), syselect();

int	mmrw();
#define	mmselect	seltrue

#include "ingres.h"
#if NINGRES > 0
int	ingres_open(), ingres_write();
#define	ingres_read	nodev
#define	ingres_ioctl	nodev
#define	ingres_close	nulldev
#else
#define	ingres_open	nodev
#define	ingres_close	nodev
#define	ingres_read	nodev
#define	ingres_write	nodev
#define	ingres_ioctl	nodev
#endif

int	fdopen();
int	ttselect(), seltrue();

struct cdevsw	cdevsw[] = {
/* cn = 0 */
	cnopen,		cnclose,	cnread,		cnwrite,
	cnioctl,	nulldev,	cons,		ttselect,
	nulldev,
/* mem = 1 */
	nulldev,	nulldev,	mmrw,		mmrw,
	nodev,		nulldev,	0,		mmselect,
	nulldev,
/* dz = 2 */
	dzopen,		dzclose,	dzread,		dzwrite,
	dzioctl,	dzstop,		dz_tty,		ttselect,
	nulldev,
/* dh = 3 */
	dhopen,		dhclose,	dhread,		dhwrite,
	dhioctl,	dhstop,		dh11,		dhselect,
	nulldev,
/* dhu = 4 */
	dhuopen,	dhuclose,	dhuread,	dhuwrite,
	dhuioctl,	dhustop,	dhu_tty,	dhuselect,
	nulldev,
/* lp = 5 */
	lpopen,		lpclose,	nodev,		lpwrite,
	nodev,		nulldev,	0,		nodev,
	nulldev,
/* ht = 6 */
	htopen,		htclose,	rawrw,		rawrw,
	htioctl,	nulldev,	0,		seltrue,
	htstrategy,
/* tm = 7 */
	tmopen,		tmclose,	rawrw,		rawrw,
	tmioctl,	nulldev,	0,		seltrue,
	tmstrategy,
/* ts = 8 */
	tsopen,		tsclose,	rawrw,		rawrw,
	tsioctl,	nulldev,	0,		seltrue,
	tsstrategy,
/* tty = 9 */
	syopen,		nulldev,	syread,		sywrite,
	syioctl,	nulldev,	0,		syselect,
	nulldev,
/* ptc = 10 */
	ptcopen,	ptcclose,	ptcread,	ptcwrite,
	ptyioctl,	nulldev,	pt_tty,		ptcselect,
	nulldev,
/* pts = 11 */
	ptsopen,	ptsclose,	ptsread,	ptswrite,
	ptyioctl,	ptsstop,	pt_tty,		ttselect,
	nulldev,
/* dr = 12 */
	dropen,		drclose,	rawrw,		rawrw,
	drioctl,	nulldev,	0,		seltrue,
	drstrategy,
/* hk = 13 */
	hkopen,		hkclose,	rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	hkstrategy,
/* ra = 14 */
	raopen,		raclose,	rawrw,		rawrw,
	raioctl,	nulldev,	0,		seltrue,
	rastrategy,
/* rk = 15 */
	rkopen,		rkclose,	rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	rkstrategy,
/* rl = 16 */
	rlopen,		rlclose,	rawrw,		rawrw,
	rlioctl,	nulldev,	0,		seltrue,
	rlstrategy,
/* rx = 17 */
	rxopen,		rxclose,	rawrw,		rawrw,
	rxioctl,	nulldev,	0,		seltrue,
	rxstrategy,
/* si = 18 */
	siopen,		siclose,	rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	sistrategy,
/* xp = 19 */
	xpopen,		xpclose,	rawrw,		rawrw,
	xpioctl,	nulldev,	0,		seltrue,
	xpstrategy,
/* br = 20 */
	bropen,		brclose,	rawrw,		rawrw,
	nodev,		nulldev,	0,		seltrue,
	brstrategy,
/* dn = 21 */
	dnopen,		dnclose,	dnread,		dnwrite,
	dnioctl,	nulldev,	0,		seltrue,
	nulldev,
/* log = 22 */
	logopen,	logclose,	logread,	nodev,
	logioctl,	nulldev,	0,		logselect,
	nulldev,
/* tmscp = 23 (tu81/tk50) */
	tmscpopen,	tmscpclose,	rawrw,		rawrw,
	tmscpioctl,	nulldev,	0,		seltrue,
	tmscpstrategy,
/* dhv = 24 */
	dhvopen,	dhvclose,	dhvread,	dhvwrite,
	dhvioctl,	dhvstop,	dhv_tty,	dhvselect,
	nulldev,
/* ingres = 25 */
	ingres_open,	ingres_close,	ingres_read,	ingres_write,
	ingres_ioctl,	nulldev,	0,		seltrue,
	nulldev,
/* fd = 26 */
	fdopen,		nodev,		nodev,		nodev,
	nodev,		nodev,		0,		nodev,
	nodev,
};

int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

/*
 * Routine that identifies /dev/mem and /dev/kmem.
 *
 * A minimal stub routine can always return 0.
 */
iskmemdev(dev)
	register dev_t dev;
{

	if (major(dev) == 1 && (minor(dev) == 0 || minor(dev) == 1))
		return (1);
	return (0);
}

/*
 * Routine to determine if a device is a disk.
 *
 * A minimal stub routine can always return 0.
 */
isdisk(dev, type)
	dev_t dev;
	register int type;
{

	switch (major(dev)) {
	case 3:			/* ram */
	case 4:			/* hk */
	case 5:			/* ra */
	case 6:			/* rk */
	case 7:			/* rl */
	case 8:			/* rx */
	case 9:			/* si */
	case 10:		/* xp */
	case 11:		/* br */
		if (type == IFBLK)
			return (1);
		return (0);
	case 13:		/* rhk */
	case 14:		/* rra */
	case 15:		/* rrk */
	case 16:		/* rrl */
	case 17:		/* rrx */
	case 18:		/* rsi */
	case 19:		/* rxp */
	case 20:		/* rbr */
		if (type == IFCHR)
			return (1);
		/* fall through */
	default:
		return (0);
	}
	/* NOTREACHED */
}

#define MAXDEV	27
static char chrtoblktbl[MAXDEV] =  {
      /* CHR */      /* BLK */
	/* 0 */		NODEV,
	/* 1 */		NODEV,
	/* 2 */		NODEV,
	/* 3 */		NODEV,
	/* 4 */		NODEV,
	/* 5 */		NODEV,
	/* 6 */		0,		/* ht */
	/* 7 */		1,		/* tm */
	/* 8 */		2,		/* ts */
	/* 9 */		NODEV,
	/* 10 */	NODEV,
	/* 11 */	NODEV,
	/* 12 */	NODEV,
	/* 13 */	4,		/* hk */
	/* 14 */	5,		/* ra */
	/* 15 */	6,		/* rk */
	/* 16 */	7,		/* rl */
	/* 17 */	8,		/* rx */
	/* 18 */	9,		/* si */
	/* 19 */	10,		/* xp */
	/* 20 */	11,		/* br */
	/* 21 */	NODEV,
	/* 22 */	NODEV,
	/* 23 */	12,		/* tmscp */
	/* 24 */	NODEV,
	/* 25 */	NODEV,
	/* 26 */	NODEV
};

/*
 * Routine to convert from character to block device number.
 *
 * A minimal stub routine can always return NODEV.
 */
chrtoblk(dev)
	register dev_t dev;
{
	register int blkmaj;

	if (major(dev) >= MAXDEV || (blkmaj = chrtoblktbl[major(dev)]) == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}

/*
 * This routine returns the cdevsw[] index of the block device
 * specified by the input parameter.    Used by init_main and ufs_mount to
 * find the diskdriver's ioctl entry point so that the label and partition
 * information can be obtained for 'block' (instead of 'character') disks.
 *
 * Rather than create a whole separate table 'chrtoblktbl' is scanned
 * looking for a match.  This routine is only called a half dozen times during
 * a system's life so efficiency isn't a big concern.
*/

blktochr(dev)
	register dev_t dev;
	{
	register int maj = major(dev);
	register int i;

	for	(i = 0; i < MAXDEV; i++)
		{
		if	(maj == chrtoblktbl[i])
			return(i);
		}
	return(NODEV);
	}

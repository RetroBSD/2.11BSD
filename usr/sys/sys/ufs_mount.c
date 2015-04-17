/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_mount.c	2.1 (2.11BSD GTE) 1997/6/29
 */

#include "param.h"
#include "../machine/seg.h"

#include "systm.h"
#include "user.h"
#include "inode.h"
#include "fs.h"
#include "buf.h"
#include "mount.h"
#include "file.h"
#include "namei.h"
#include "conf.h"
#include "stat.h"
#include "disklabel.h"
#include "ioctl.h"
#ifdef QUOTA
#include "quota.h"
#endif

smount()
{
	register struct a {
		char	*fspec;
		char	*freg;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	dev_t dev;
	register struct inode *ip;
	register struct fs *fs;
	struct	nameidata nd;
	struct	nameidata *ndp = &nd;
	struct	mount	*mp;
	u_int lenon, lenfrom;
	int	error = 0;
	char	mnton[MNAMELEN], mntfrom[MNAMELEN];

	if	(u.u_error = getmdev(&dev, uap->fspec))
		return;
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, uap->freg);
	if	((ip = namei(ndp)) == NULL)
		return;
	if ((ip->i_mode&IFMT) != IFDIR) {
		error = ENOTDIR;
		goto	cmnout;
	}
/*
 * The following two copyinstr calls will not fault because getmdev() or
 * namei() would have returned an error for invalid parameters.
*/
	copyinstr(uap->freg, mnton, sizeof (mnton) - 1, &lenon);
	copyinstr(uap->fspec, mntfrom, sizeof (mntfrom) - 1, &lenfrom);

	if	(uap->flags & MNT_UPDATE)
		{
		fs = ip->i_fs;
		mp = (struct mount *)
			((int)fs - offsetof(struct mount, m_filsys));
		if	(ip->i_number != ROOTINO)
			{
			error = EINVAL;		/* Not a mount point */
			goto	cmnout;
			}
/*
 * Check that the device passed in is the same one that is in the mount 
 * table entry for this mount point.
*/
		if	(dev != mp->m_dev)
			{
			error = EINVAL;		/* not right mount point */
			goto	cmnout;
			}
/*
 * This is where the RW to RO transformation would be done.  It is, for now,
 * too much work to port pages of code to do (besides which most
 * programs get very upset at having access yanked out from under them).
*/
		if	(fs->fs_ronly == 0 && (uap->flags & MNT_RDONLY))
			{
			error = EPERM;		/* ! RW to RO updates */
			goto	cmnout;
			}
/*
 * However, going from RO to RW is easy.  Then merge in the new
 * flags (async, sync, nodev, etc) passed in from the program.
*/
		if	(fs->fs_ronly && ((uap->flags & MNT_RDONLY) == 0))
			{
			fs->fs_ronly = 0;
			mp->m_flags &= ~MNT_RDONLY;
			}
#define	_MF (MNT_NOSUID | MNT_NODEV | MNT_NOEXEC | MNT_ASYNC | MNT_SYNCHRONOUS | MNT_NOATIME)
		mp->m_flags &= ~_MF;
		mp->m_flags |= (uap->flags & _MF);
#undef _MF
		iput(ip);
		u.u_error = 0;
		goto	updname;
		}
	else
		{
/*
 * This is where a new mount (not an update of an existing mount point) is 
 * done.
 *
 * The directory being mounted on can have no other references AND can not
 * currently be a mount point.  Mount points have an inode number of (you
 * guessed it) ROOTINO which is 2.
*/
		if	(ip->i_count != 1 || (ip->i_number == ROOTINO))
			{
			error = EBUSY;
			goto cmnout;
			}
		fs = mountfs(dev, uap->flags, ip);
		if	(fs == 0)
			return;
		}
/*
 * Lastly, both for new mounts and updates of existing mounts, update the
 * mounted-on and mounted-from fields.
*/
updname:
	mount_updname(fs, mnton, mntfrom, lenon, lenfrom);
	return;
cmnout:
	iput(ip);
	return(u.u_error = error);
}

mount_updname(fs, on, from, lenon, lenfrom)
	struct	fs	*fs;
	char	*on, *from;
	int	lenon, lenfrom;
	{
	struct	mount	*mp;
	register struct	xmount	*xmp;

	bzero(fs->fs_fsmnt, sizeof (fs->fs_fsmnt));
	bcopy(on, fs->fs_fsmnt, sizeof (fs->fs_fsmnt) - 1);
	mp = (struct mount *)((int)fs - offsetof(struct mount, m_filsys));
	xmp = (struct xmount *)SEG5;
	mapseg5(mp->m_extern, XMOUNTDESC);
	bzero(xmp, sizeof (struct xmount));
	bcopy(on, xmp->xm_mnton, lenon);
	bcopy(from, xmp->xm_mntfrom, lenfrom);
	normalseg5();
	}

/* this routine has races if running twice */
struct fs *
mountfs(dev, flags, ip)
	dev_t dev;
	int flags;
	struct inode *ip;
{
	register struct mount *mp = 0;
	struct buf *tp = 0;
	register struct fs *fs;
	register int error;
	int ronly = flags & MNT_RDONLY;
	int needclose = 0;
	int chrdev, (*ioctl)();
	struct	partinfo dpart;

	error =
	    (*bdevsw[major(dev)].d_open)(dev, ronly ? FREAD : FREAD|FWRITE, S_IFBLK);
	if (error)
		goto out;
/*
 * Now make a check that the partition is really a filesystem if the 
 * underlying driver supports disklabels (there is an ioctl entry point 
 * and calling it does not return an error).
 *
 * XXX - Check for NODEV because BLK only devices (i.e. the 'ram' driver) do not
 * XXX - have a CHR counterpart.  Such drivers can not support labels due to
 * XXX - the lack of an ioctl entry point.
*/
	chrdev = blktochr(dev);
	if	(chrdev == NODEV)
		ioctl = NULL;	
	else
		ioctl = cdevsw[chrdev].d_ioctl;
	if	(ioctl && !(*ioctl)(dev, DIOCGPART, &dpart, FREAD))
		{
		if	(dpart.part->p_fstype != FS_V71K)
			{
			error = EINVAL;
			goto out;
			}
		}
	needclose = 1;
	tp = bread(dev, SBLOCK);
	if (tp->b_flags & B_ERROR)
		goto out;
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_inodp != 0 && dev == mp->m_dev) {
			mp = 0;
			error = EBUSY;
			needclose = 0;
			goto out;
		}
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_inodp == 0)
			goto found;
	mp = 0;
	error = EMFILE;		/* needs translation */
	goto out;
found:
	mp->m_inodp = ip;	/* reserve slot */
	mp->m_dev = dev;
	fs = &mp->m_filsys;
	bcopy(mapin(tp), (caddr_t)fs, sizeof(struct fs));
	mapout(tp);
	brelse(tp);
	tp = 0;
	fs->fs_ronly = (ronly != 0);
	if (ronly == 0)
		fs->fs_fmod = 1;
	fs->fs_ilock = 0;
	fs->fs_flock = 0;
	fs->fs_nbehind = 0;
	fs->fs_lasti = 1;
	fs->fs_flags = flags;
	if (ip) {
		ip->i_flag |= IMOUNT;
		cacheinval(ip);
		IUNLOCK(ip);
	}
	return (fs);
out:
	if (error == 0)
		error = EIO;
	if (ip)
		iput(ip);
	if (mp)
		mp->m_inodp = 0;
	if (tp)
		brelse(tp);
	if (needclose) {
		(*bdevsw[major(dev)].d_close)(dev, 
			ronly? FREAD : FREAD|FWRITE, S_IFBLK);
		binval(dev);
	}
	u.u_error = error;
	return (0);
}

umount()
{
	struct a {
		char	*fspec;
	} *uap = (struct a *)u.u_ap;

	u.u_error = unmount1(uap->fspec);
}

unmount1(fname)
	caddr_t fname;
{
	dev_t dev;
	register struct mount *mp;
	register struct inode *ip;
	register int error;
	int aflag;

	error = getmdev(&dev, fname);
	if (error)
		return (error);
	for (mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
		if (mp->m_inodp != NULL && dev == mp->m_dev)
			goto found;
	return (EINVAL);
found:
	xumount(dev);	/* remove unused sticky files from text table */
	nchinval(dev);	/* flush the name cache */
	aflag = mp->m_flags & MNT_ASYNC;
	mp->m_flags &= ~MNT_ASYNC;	/* Don't want async when unmounting */
	ufs_sync(mp);

#ifdef QUOTA
	if (iflush(dev, mp->m_qinod) < 0)
#else
	if (iflush(dev) < 0)
#endif
		{
		mp->m_flags |= aflag;
		return (EBUSY);
		}
#ifdef QUOTA
	QUOTAMAP();
	closedq(mp);
	QUOTAUNMAP();
	/*
	 * Here we have to iflush again to get rid of the quota inode.
	 * A drag, but it would be ugly to cheat, & this doesn't happen often
	 */
	(void)iflush(dev, (struct inode *)NULL);
#endif
	ip = mp->m_inodp;
	ip->i_flag &= ~IMOUNT;
	irele(ip);
	mp->m_inodp = 0;
	mp->m_dev = 0;
	(*bdevsw[major(dev)].d_close)(dev, 0, S_IFBLK);
	binval(dev);
	return (0);
}

/*
 * Common code for mount and umount.
 * Check that the user's argument is a reasonable
 * thing on which to mount, otherwise return error.
 */
getmdev(pdev, fname)
	caddr_t fname;
	dev_t *pdev;
{
	register dev_t dev;
	register struct inode *ip;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	if (!suser())
		return (u.u_error);
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_USERSPACE, fname);
	ip = namei(ndp);
	if (ip == NULL) {
		if (u.u_error == ENOENT)
			return (ENODEV); /* needs translation */
		return (u.u_error);
	}
	if ((ip->i_mode&IFMT) != IFBLK) {
		iput(ip);
		return (ENOTBLK);
	}
	dev = (dev_t)ip->i_rdev;
	iput(ip);
	if (major(dev) >= nblkdev)
		return (ENXIO);
	*pdev = dev;
	return (0);
}

/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ufs_fio.c	1.6 (2.11BSD GTE) 1997/11/28
 */

#include "param.h"
#include "user.h"
#include "fs.h"
#include "inode.h"
#include "mount.h"
#include "namei.h"
#include "systm.h"
#include "acct.h"
#include "stat.h"

/*
 * Check mode permission on inode pointer.
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the
 * read-only status of the file
 * system is checked.
 * Also in WRITE, prototype text
 * segments cannot be written.
 * The mode is shifted to select
 * the owner/group/other fields.
 * The super user is granted all
 * permissions.
 */
access(ip, mode)
	register struct inode *ip;
	int mode;
{
	register m;
	register gid_t *gp;

	m = mode;
	if (m == IWRITE) {
		if (ip->i_flags & IMMUTABLE) {
			u.u_error = EPERM;
			return(1);
		}
		/*
		 * Disallow write attempts on read-only
		 * file systems; unless the file is a block
		 * or character device resident on the
		 * file system.
		 */
		if (ip->i_fs->fs_ronly != 0) {
			if ((ip->i_mode & IFMT) != IFCHR &&
			    (ip->i_mode & IFMT) != IFBLK) {
				u.u_error = EROFS;
				return (1);
			}
		}
		/*
		 * If there's shared text associated with
		 * the inode, try to free it up once.  If
		 * we fail, we can't allow writing.
		 */
		if (ip->i_flag&ITEXT)
			xuntext(ip->i_text);
		if (ip->i_flag & ITEXT) {
			u.u_error = ETXTBSY;
			return (1);
		}
	}
	/*
	 * If you're the super-user,
	 * you always get access.
	 */
	if (u.u_uid == 0)
		return (0);
	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (u.u_uid != ip->i_uid) {
		m >>= 3;
		gp = u.u_groups;
		for (; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (ip->i_gid == *gp)
				goto found;
		m >>= 3;
found:
		;
	}
	if ((ip->i_mode&m) != 0)
		return (0);
	u.u_error = EACCES;
	return (1);
}

/* copied, for supervisory networking, to sys_net.c */
/*
 * Test if the current user is the
 * super user.
 */
suser()
{

	if (u.u_uid == 0) {
		u.u_acflag |= ASU;
		return (1);
	}
	u.u_error = EPERM;
	return (0);
}

/*
 * Set the attributes on a file.  This was placed here because ufs_syscalls
 * is too large already (it will probably be split into two files eventually).
*/

ufs_setattr(ip, vap)
	register struct inode *ip;
	register struct vattr *vap;
	{
	int	error;
	struct	timeval atimeval, mtimeval;

	if	(ip->i_fs->fs_ronly)	/* can't change anything on a RO fs */
		return(EROFS);
	if	(vap->va_flags != VNOVAL)
		{
		if	(u.u_uid != ip->i_uid && !suser())
			return(u.u_error);
		if	(u.u_uid == 0)
			{
			if	((ip->i_flags & (SF_IMMUTABLE|SF_APPEND)) &&
					securelevel > 0)
				return(EPERM);
			ip->i_flags = vap->va_flags;
			}
		else
			{
			if	(ip->i_flags & (SF_IMMUTABLE|SF_APPEND))
				return(EPERM);
			ip->i_flags &= SF_SETTABLE;
			ip->i_flags |= (vap->va_flags & UF_SETTABLE);
			}
		ip->i_flag |= ICHG;
		if	(vap->va_flags & (IMMUTABLE|APPEND))
			return(0);
		}
	if	(ip->i_flags & (IMMUTABLE|APPEND))
		return(EPERM);
/*
 * Go thru the fields (other than 'flags') and update iff not VNOVAL.
*/
	if	(vap->va_uid != (uid_t)VNOVAL || vap->va_gid != (gid_t)VNOVAL)
		if	(error = chown1(ip, vap->va_uid, vap->va_gid))
			return(error);
	if	(vap->va_size != (off_t)VNOVAL)
		{
		if	((ip->i_mode & IFMT) == IFDIR)
			return(EISDIR);
		itrunc(ip, vap->va_size, 0);
		if	(u.u_error)
			return(u.u_error);
		}
	if	(vap->va_atime != (time_t)VNOVAL ||
		 vap->va_mtime != (time_t)VNOVAL)
		{
		if	(u.u_uid != ip->i_uid && !suser() &&
			 ((vap->va_vaflags & VA_UTIMES_NULL) == 0 ||
			 access(ip, IWRITE)))
			return(u.u_error);
		if	(vap->va_atime != (time_t)VNOVAL &&
				!(ip->i_fs->fs_flags & MNT_NOATIME))
			ip->i_flag |= IACC;
		if	(vap->va_mtime != (time_t)VNOVAL)
			ip->i_flag |= (IUPD|ICHG);
		atimeval.tv_sec = vap->va_atime;
		mtimeval.tv_sec = vap->va_mtime;
		iupdat(ip, &atimeval, &mtimeval, 1);
		}
	if	(vap->va_mode != (mode_t)VNOVAL)
		return(chmod1(ip, vap->va_mode));
	return(0);
	}

ufs_mountedon(dev)
	dev_t dev;
	{
	register struct mount *mp;

	for	(mp = mount; mp < &mount[NMOUNT]; mp++)
		{
		if	(mp->m_inodp == NULL)
			continue;
		if	(mp->m_dev == dev)
			return(EBUSY);
		}
	return(0);
	}

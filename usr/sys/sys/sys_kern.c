/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	sys_kern.c 1.2 (2.11BSD) 1997/1/30
 */

#include "param.h"
#include "../machine/seg.h"

#include "user.h"
#include "file.h"
#include "socketvar.h"
#include "inode.h"
#include "proc.h"
#include "namei.h"
#include "mbuf.h"
#include "map.h"

int knetisr;

netcrash()
{
	panic("Network crashed");
}

/*
 * These next entry points exist to allow the network kernel to
 * access components of structures that exist in kernel data space.
 */
netpsignal(p, sig)		/* XXX? sosend, sohasoutofband, sowakeup */
	struct proc *p;		/* if necessary, wrap psignal in save/restor */
	int sig;
{
	mapinfo map;

	savemap(map);
	psignal(p, sig);
	restormap(map);
}

struct proc *
netpfind(pid)
	int pid;
{
	register struct proc *p;
	mapinfo map;

	savemap(map);
	p = pfind(pid);
	restormap(map);
	return(p);
}

void
fpflags(fp, set, clear)
	struct file *fp;
	int set, clear;
{
	fp->f_flag |= set;
	fp->f_flag &= ~clear;
}

void
fadjust(fp, msg, cnt)
	struct file *fp;
	int msg, cnt;
{
	fp->f_msgcount += msg;
	fp->f_count += cnt;
}

fpfetch(fp, fpp)
	struct file *fp, *fpp;
{
	*fpp = *fp;
	return(fp->f_count);
}

void
unpdet(ip)
	struct inode *ip;
{
	ip->i_socket = 0;
	irele(ip);
}

unpbind(path, len, ipp, unpsock)
	char *path;
	int len;
	struct inode **ipp;
	struct socket *unpsock;
{
	/*
	 * As far as I could find out, the 'path' is in the _u area because
	 * a fake mbuf was MBZAP'd in bind().  The inode pointer is in the
	 * kernel stack so we can modify it.  SMS
	 */
	register struct inode *ip;
	char pth[MLEN];
	int error;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	bcopy(path, pth, len);
	NDINIT(ndp, CREATE, FOLLOW, UIO_SYSSPACE, pth);
	ndp->ni_dirp[len - 2] = 0;
	*ipp = 0;
	ip = namei(ndp);
	if (ip) {
		iput(ip);
		return(EADDRINUSE);
	}
	if (u.u_error || !(ip = maknode(IFSOCK | 0777, ndp))) {
		error = u.u_error;
		u.u_error = 0;
		return(error);
	}
	*ipp = ip;
	ip->i_socket = unpsock;
	iunlock(ip);
	return(0);
}

unpconn(path, len, so2, ipp)
	char *path;
	int len;
	struct socket **so2;
	struct inode **ipp;
{
	register struct inode *ip;
	char pth[MLEN];
	int error;
	struct	nameidata nd;
	register struct	nameidata *ndp = &nd;

	bcopy(path, pth, len);
	if (!len)
		return(EINVAL);		/* paranoia */
	NDINIT(ndp, LOOKUP, FOLLOW, UIO_SYSSPACE, pth);
	ndp->ni_dirp[len - 2] = 0;
	ip = namei(ndp);
	*ipp = ip;
	if (!ip || access(ip, IWRITE)) {
		error = u.u_error;
		u.u_error = 0;
		return(error);
	}
	if ((ip->i_mode & IFMT) != IFSOCK)
		return(ENOTSOCK);
	*so2 = ip->i_socket;
	if (*so2 == 0)
		return(ECONNREFUSED);
	return(0);
}

unpgc1(beginf, endf)
	struct file **beginf, **endf;
{
	register struct file *fp;

	for (*beginf = fp = file; fp < fileNFILE; fp++)
		fp->f_flag &= ~(FMARK|FDEFER);
	*endf = fileNFILE;
}

unpdisc(fp)
	struct file *fp;
{
	--fp->f_msgcount;
	return(closef(fp));
}

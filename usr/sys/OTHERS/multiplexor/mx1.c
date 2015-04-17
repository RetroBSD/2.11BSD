#include "param.h"
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/inode.h>
#include <sys/mx.h>
#include <sys/file.h>
#include <sys/conf.h>

/*
 *	SCCS id	@(#)mx1.c	2.1 (Berkeley)	8/5/83
 */

/*
 * Multiplexor:   clist version
 *
 * installation:
 *	requires a line in cdevsw -
 *		mxopen, mxclose, mxread, mxwrite, mxioctl, 0,
 *
 *	also requires a line in linesw -
 *		mxopen, mxclose, mcread, mcwrite, mxioctl, nulldev, nulldev,
 *
 *	The linesw entry for mpx should be the last one in the table.
						 *	'nldisp' (number of line disciplines) should not include the
 *	mpx line.  This is to prevent mpx from being enabled by an ioctl.

 *	mxtty.c must be loaded instead of tty.c so that certain
 *	sleeps will not be done if a typewriter is connected to
 *	a channel and so that sdata will be called from ttyinput.
 *	
 */
struct	chan	chans[NCHANS];
struct	schan	schans[NPORTS];
struct	group	*groups[NGROUPS];
int	mpxline;
struct chan *xcp();
dev_t	mpxdev	= -1;


char	mcdebugs[NDEBUGS];


/*
 * Allocate a channel, set c_index to index.
 */
struct	chan *
challoc(index, isport)
{
register s,i;
register struct chan *cp;

	s = spl6();
	for(i=0;i<((isport)?NPORTS:NCHANS);i++) {
		cp = (isport)? (struct chan *)(schans+i): chans+i;
		if(cp->c_group == NULL) {
			cp->c_index = index;
			cp->c_pgrp = 0;
			cp->c_flags = 0;
			splx(s);
			return(cp);
		}
	}
	splx(s);
	return(NULL);
}



/*
 * Allocate a group table cell.
 */
gpalloc()
{
	register i;

	for (i=NGROUPS-1; i>=0; i--)
		if (groups[i]==NULL) {
			groups[i]++;
			return(i);
		}
	u.u_error = ENXIO;
	return(i);
}


/*
 * Add a channel to the group in
 * inode ip.
 */
struct chan *
addch(ip, isport)
struct inode *ip;
{
register struct chan *cp;
register struct group *gp;
register i;

	plock(ip);
	gp = &ip->i_un.i_group;
	for(i=0;i<NINDEX;i++) {
		cp = (struct chan *)gp->g_chans[i];
		if (cp == NULL) {
			if ((cp=challoc(i, isport)) != NULL) {
				gp->g_chans[i] = cp;
				cp->c_group = gp;
			}
			break;
		}
		cp = NULL;
	}
	prele(ip);
	return(cp);
}

/*
 * Mpxchan system call.
 */

mpxchan()
{
	extern	mxopen(), mcread(), sdata(), scontrol();
	struct	inode	*ip, *gip;
	struct	tty	*tp;
	struct	file	*fp, *chfp, *gfp;
	struct	chan	*cp;
	struct	group	*gp, *ngp;
	struct	mx_args	vec;
	struct	a	{
		int	cmd;
		int	*argvec;
	} *uap;
	dev_t	dev;
	register int i;

	/*
	 * Common setup code.
	 */

	uap = (struct a *)u.u_ap;
	(void) copyin((caddr_t)uap->argvec, (caddr_t)&vec, sizeof vec);
	gp = NULL; gfp = NULL; cp = NULL;

	switch(uap->cmd) {

	case NPGRP:
		if (vec.m_arg[1] < 0)
			break;
	case CHAN:
	case JOIN:
	case EXTR:
	case ATTACH:
	case DETACH:
	case CSIG:
		gfp = getf(vec.m_arg[1]);
		if (gfp==NULL)
			return;
		gip = gfp->f_inode;
		gp = &gip->i_un.i_group;
		if (gp->g_inode != gip) {
			u.u_error = ENXIO;
			return;
		}
	}

	switch(uap->cmd) {

	/*
	 * Create an MPX file.
	 */

	case MPX:
	case MPXN:
		if (mpxdev < 0) {
			for (i=0; linesw[i].l_open; i++) {
				if (linesw[i].l_read==mcread) {
					mpxline = i;
					for (i=0; cdevsw[i].d_open; i++) {
						if (cdevsw[i].d_open==mxopen) {
							mpxdev = (dev_t)(i<<8);
						}
					}
				}
			}
			if (mpxdev < 0) {
				u.u_error = ENXIO;
				return;
			}
		}
		if (uap->cmd==MPXN) {
			if ((ip=ialloc(pipedev))==NULL)
				return;
			ip->i_mode = ((vec.m_arg[1]&0777)+IFMPC) & ~u.u_cmask;
			ip->i_flag = IACC|IUPD|ICHG;
		} else {
			u.u_dirp = vec.m_name;
			ip = namei(uchar,1);
			if (ip != NULL) {
				i = ip->i_mode&IFMT;
				u.u_error = EEXIST;
				if (i==IFMPC || i==IFMPB) {
					i = minor(ip->i_un.i_rdev);
					gp = groups[i];
					if (gp && gp->g_inode==ip)
						u.u_error = EBUSY;
				}
				iput(ip);
				return;
			}
			if (u.u_error)
				return;
			ip = maknode((vec.m_arg[1]&0777)+IFMPC);
			if (ip == NULL)
				return;
		}
		if ((i=gpalloc()) < 0) {
			iput(ip);
			return;
		}
		if ((fp=falloc()) == NULL) {
			iput(ip);
			groups[i] = NULL;
			return;
		}
		ip->i_un.i_rdev = (daddr_t)(mpxdev+i);
		ip->i_count++;
		prele(ip);


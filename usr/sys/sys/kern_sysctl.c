/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Karels at Berkeley Software Design, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_sysctl.c	8.4.11 (2.11BSD) 1999/8/11
 */

/*
 * sysctl system call.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <sys/ioctl.h>
#include <sys/text.h>
#include <sys/tty.h>
#include <sys/vm.h>
#include <sys/map.h>
#include <machine/seg.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#include <conf.h>

sysctlfn kern_sysctl;
sysctlfn hw_sysctl;
#ifdef DEBUG
sysctlfn debug_sysctl;
#endif
sysctlfn vm_sysctl;
sysctlfn fs_sysctl;
#ifdef	INET
sysctlfn NET_SYSCTL;
extern	int	net_sysctl();	/* In supervisor space */
#endif
sysctlfn cpu_sysctl;

/*
 * Locking and stats
 */
static struct sysctl_lock {
	int	sl_lock;
	int	sl_want;
	int	sl_locked;
} memlock;

struct sysctl_args {
	int	*name;
	u_int	namelen;
	void	*old;
	size_t	*oldlenp;
	void	*new;
	size_t	newlen;
};

int
__sysctl()
{
	register struct sysctl_args *uap = (struct sysctl_args *)u.u_ap;
	int error;
	u_int savelen, oldlen = 0;
	sysctlfn *fn;
	int name[CTL_MAXNAME];

	if (uap->new != NULL && !suser())
		return (u.u_error);	/* XXX */
	/*
	 * all top-level sysctl names are non-terminal
	 */
	if (uap->namelen > CTL_MAXNAME || uap->namelen < 2)
		return (u.u_error = EINVAL);
	if (error = copyin(uap->name, &name, uap->namelen * sizeof(int)))
		return (u.u_error = error);

	switch (name[0]) {
	case CTL_KERN:
		fn = kern_sysctl;
		break;
	case CTL_HW:
		fn = hw_sysctl;
		break;
	case CTL_VM:
		fn = vm_sysctl;
		break;
#ifdef	INET
	case CTL_NET:
		fn = NET_SYSCTL;
		break;
#endif
#ifdef notyet
	case CTL_FS:
		fn = fs_sysctl;
		break;
#endif
	case CTL_MACHDEP:
		fn = cpu_sysctl;
		break;
#ifdef DEBUG
	case CTL_DEBUG:
		fn = debug_sysctl;
		break;
#endif
	default:
		return (u.u_error = EOPNOTSUPP);
	}

	if (uap->oldlenp &&
	    (error = copyin(uap->oldlenp, &oldlen, sizeof(oldlen))))
		return (u.u_error = error);
	if (uap->old != NULL) {
		while (memlock.sl_lock) {
			memlock.sl_want = 1;
			sleep((caddr_t)&memlock, PRIBIO+1);
			memlock.sl_locked++;
		}
		memlock.sl_lock = 1;
		savelen = oldlen;
	}
	error = (*fn)(name + 1, uap->namelen - 1, uap->old, &oldlen,
	    uap->new, uap->newlen);
	if (uap->old != NULL) {
		memlock.sl_lock = 0;
		if (memlock.sl_want) {
			memlock.sl_want = 0;
			wakeup((caddr_t)&memlock);
		}
	}
	if (error)
		return (u.u_error = error);
	if (uap->oldlenp) {
		error = copyout(&oldlen, uap->oldlenp, sizeof(oldlen));
		if (error)
			return(u.u_error = error);
	}
	u.u_r.r_val1 = oldlen;
	return (0);
}

/*
 * kernel related system variables.
 */
kern_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	int error, level;
	u_long longhostid;
	char bsd[10];
	extern int Acctthresh;		/* kern_acct.c */
	extern char version[];

	/* all sysctl names at this level are terminal */
	if (namelen != 1 && !(name[0] == KERN_PROC || name[0] == KERN_PROF))
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case KERN_OSTYPE:
	case KERN_OSRELEASE:
		/* code is cheaper than D space */
		bsd[0]='2';bsd[1]='.';bsd[2]='1';bsd[3]='1';bsd[4]='B';
		bsd[5]='S';bsd[6]='D';bsd[7]='\0';
		return (sysctl_rdstring(oldp, oldlenp, newp, bsd));
	case KERN_ACCTTHRESH:
		level = Acctthresh;
		error = sysctl_int(oldp, oldlenp, newp, newlen, &level);
		if	(newp && !error)
			{
 			if	(level < 0 || level > 128)
				error = EINVAL;
			else
				Acctthresh = level;
			}
		return(error);
	case KERN_OSREV:
		return (sysctl_rdlong(oldp, oldlenp, newp, (long)BSD));
	case KERN_VERSION:
		return (sysctl_rdstring(oldp, oldlenp, newp, version));
	case KERN_MAXINODES:
		return(sysctl_rdint(oldp, oldlenp, newp, ninode));
	case KERN_MAXPROC:
		return (sysctl_rdint(oldp, oldlenp, newp, nproc));
	case KERN_MAXFILES:
		return (sysctl_rdint(oldp, oldlenp, newp, nfile));
	case KERN_MAXTEXTS:
		return (sysctl_rdint(oldp, oldlenp, newp, ntext));
	case KERN_ARGMAX:
		return (sysctl_rdint(oldp, oldlenp, newp, NCARGS));
	case KERN_SECURELVL:
		level = securelevel;
		if ((error = sysctl_int(oldp, oldlenp, newp, newlen, &level)) ||
		    newp == NULL)
			return (error);
		if (level < securelevel && u.u_procp->p_pid != 1)
			return (EPERM);
		securelevel = level;
		return (0);
	case KERN_HOSTNAME:
		error = sysctl_string(oldp, oldlenp, newp, newlen,
		    hostname, sizeof(hostname));
		if (newp && !error)
			hostnamelen = newlen;
		return (error);
	case KERN_HOSTID:
		longhostid = hostid;
		error =  sysctl_long(oldp, oldlenp, newp, newlen, &longhostid);
		hostid = longhostid;
		return (error);
	case KERN_CLOCKRATE:
		return (sysctl_clockrate(oldp, oldlenp));
	case KERN_BOOTTIME:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &boottime,
		    sizeof(struct timeval)));
	case KERN_INODE:
		return (sysctl_inode(oldp, oldlenp));
	case KERN_PROC:
		return (sysctl_doproc(name + 1, namelen - 1, oldp, oldlenp));
	case KERN_FILE:
		return (sysctl_file(oldp, oldlenp));
	case KERN_TEXT:
		return (sysctl_text(oldp, oldlenp));
#ifdef GPROF
	case KERN_PROF:
		return (sysctl_doprof(name + 1, namelen - 1, oldp, oldlenp,
		    newp, newlen));
#endif
	case KERN_NGROUPS:
		return (sysctl_rdint(oldp, oldlenp, newp, NGROUPS));
	case KERN_JOB_CONTROL:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));
	case KERN_POSIX1:
	case KERN_SAVED_IDS:
		return (sysctl_rdint(oldp, oldlenp, newp, 0));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * hardware related system variables.
 */
hw_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	char c[10];
	char *cpu2str();
	extern	size_t physmem;			/* machdep2.c */

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case HW_MACHINE:
		return (sysctl_rdstring(oldp, oldlenp, newp, "pdp11"));
	case HW_MODEL:
		return (sysctl_rdstring(oldp, oldlenp, newp,
				cpu2str(c,sizeof (c))));
	case HW_NCPU:
		return (sysctl_rdint(oldp, oldlenp, newp, 1));	/* XXX */
	case HW_BYTEORDER:
		return (sysctl_rdint(oldp, oldlenp, newp, ENDIAN));
	case HW_PHYSMEM:
		return (sysctl_rdlong(oldp, oldlenp, newp,ctob((long)physmem)));
	case HW_USERMEM:
		return (sysctl_rdlong(oldp, oldlenp, newp,ctob((long)freemem)));
	case HW_PAGESIZE:
		return (sysctl_rdint(oldp, oldlenp, newp, NBPG*CLSIZE));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

static char *
cpu2str(buf, len)
	char	*buf;
	int	len;
	{
	register char *cp = buf + len;
	register int i = cputype;

	*--cp = '\0';
	do
		{
		*--cp = (i % 10) + '0';
		} while (i /= 10);
	return(cp);
	}

#ifdef DEBUG
/*
 * Debugging related system variables.
 */
struct ctldebug debug0, debug1, debug2, debug3, debug4;
struct ctldebug debug5, debug6, debug7, debug8, debug9;
struct ctldebug debug10, debug11, debug12, debug13, debug14;
struct ctldebug debug15, debug16, debug17, debug18, debug19;
static struct ctldebug *debugvars[CTL_DEBUG_MAXID] = {
	&debug0, &debug1, &debug2, &debug3, &debug4,
	&debug5, &debug6, &debug7, &debug8, &debug9,
	&debug10, &debug11, &debug12, &debug13, &debug14,
	&debug15, &debug16, &debug17, &debug18, &debug19,
};
int
debug_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	struct ctldebug *cdp;

	/* all sysctl names at this level are name and field */
	if (namelen != 2)
		return (ENOTDIR);		/* overloaded */
	cdp = debugvars[name[0]];
	if (cdp->debugname == 0)
		return (EOPNOTSUPP);
	switch (name[1]) {
	case CTL_DEBUG_NAME:
		return (sysctl_rdstring(oldp, oldlenp, newp, cdp->debugname));
	case CTL_DEBUG_VALUE:
		return (sysctl_int(oldp, oldlenp, newp, newlen, cdp->debugvar));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
#endif /* DEBUG */

#ifdef	INET
/*
 * In 4.4BSD-Lite these functions were scattered amoungst the various
 * subsystems they dealt with.
 *
 * In 2.11BSD the kernel is overlaid and adding code to multiple 
 * files would have caused existing overlay structures to break.
 * This module will be large enough that it will end up in an overlay
 * by itself.  Thus centralizing all sysctl handling in one place makes
 * a lot of sense.  The one exception is the networking related syctl
 * functions.  Because the networking code is not overlaid and runs
 * in supervisor mode the sysctl handling can not be done here and
 * will be implemented in the 4.4BSD manner (by modifying the networking
 * modules).
*/

int
NET_SYSCTL(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	return(KScall(net_sysctl, 6 * sizeof (int),
			name, namelen, oldp, oldlenp, newp, newlen));
}
#endif

/*
 * Bit of a hack.  2.11 currently uses 'short avenrun[3]' and a fixed scale 
 * of 256.  In order not to break all the applications which nlist() for
 * 'avenrun' we build a local 'averunnable' structure here to return to the
 * user.  Eventually (after all applications which look up the load average
 * the old way) have been converted we can change things.
 *
 * We do not call vmtotal(), that could get rather expensive, rather we rely
 * on the 5 second update.
 *
 * The coremap and swapmap cases are 2.11BSD extensions.
*/

int
vm_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	struct	loadavg averunnable;	/* loadavg in resource.h */

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case VM_LOADAVG:
		averunnable.fscale = 256;
		averunnable.ldavg[0] = avenrun[0];
		averunnable.ldavg[1] = avenrun[1];
		averunnable.ldavg[2] = avenrun[2];
		return (sysctl_rdstruct(oldp, oldlenp, newp, &averunnable,
		    sizeof(averunnable)));
	case VM_METER:
#ifdef	notsure
		vmtotal();	/* could be expensive to do this every time */
#endif
		return (sysctl_rdstruct(oldp, oldlenp, newp, &total,
		    sizeof(total)));
	case VM_SWAPMAP:
		if (oldp == NULL) {
			*oldlenp = (char *)swapmap[0].m_limit - 
					(char *)swapmap[0].m_map;
			return(0);
		}
		return (sysctl_rdstruct(oldp, oldlenp, newp, swapmap,
			(int)swapmap[0].m_limit - (int)swapmap[0].m_map));
	case VM_COREMAP:
		if (oldp == NULL) {
			*oldlenp = (char *)coremap[0].m_limit - 
					(char *)coremap[0].m_map;
			return(0);
		}
		return (sysctl_rdstruct(oldp, oldlenp, newp, coremap,
			(int)coremap[0].m_limit - (int)coremap[0].m_map));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/*
 * Validate parameters and get old / set new parameters
 * for an integer-valued sysctl function.
 */
sysctl_int(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	int *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp && newlen != sizeof(int))
		return (EINVAL);
	*oldlenp = sizeof(int);
	if (oldp)
		error = copyout(valp, oldp, sizeof(int));
	if (error == 0 && newp)
		error = copyin(newp, valp, sizeof(int));
	return (error);
}

/*
 * As above, but read-only.
 */
sysctl_rdint(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	int val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(int))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(int);
	if (oldp)
		error = copyout((caddr_t)&val, oldp, sizeof(int));
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for an long-valued sysctl function.
 */
sysctl_long(oldp, oldlenp, newp, newlen, valp)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	long *valp;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(long))
		return (ENOMEM);
	if (newp && newlen != sizeof(long))
		return (EINVAL);
	*oldlenp = sizeof(long);
	if (oldp)
		error = copyout(valp, oldp, sizeof(long));
	if (error == 0 && newp)
		error = copyin(newp, valp, sizeof(long));
	return (error);
}

/*
 * As above, but read-only.
 */
sysctl_rdlong(oldp, oldlenp, newp, val)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	long val;
{
	int error = 0;

	if (oldp && *oldlenp < sizeof(long))
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = sizeof(long);
	if (oldp)
		error = copyout((caddr_t)&val, oldp, sizeof(long));
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a string-valued sysctl function.
 */
sysctl_string(oldp, oldlenp, newp, newlen, str, maxlen)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	char *str;
	int maxlen;
{
	int len, error = 0;

	len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp && newlen >= maxlen)
		return (EINVAL);
	if (oldp) {
		*oldlenp = len;
		error = vcopyout(str, oldp, len);
	}
	if (error == 0 && newp) {
		error = vcopyin(newp, str, newlen);
		str[newlen] = 0;
	}
	return (error);
}

/*
 * As above, but read-only.
 */
sysctl_rdstring(oldp, oldlenp, newp, str)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	char *str;
{
	int len, error = 0;

	len = strlen(str) + 1;
	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = len;
	if (oldp)
		error = vcopyout(str, oldp, len);
	return (error);
}

/*
 * Validate parameters and get old / set new parameters
 * for a structure oriented sysctl function.
 */
sysctl_struct(oldp, oldlenp, newp, newlen, sp, len)
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	void *sp;
	int len;
{
	int error = 0;

	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp && newlen > len)
		return (EINVAL);
	if (oldp) {
		*oldlenp = len;
		error = copyout(sp, oldp, len);
	}
	if (error == 0 && newp)
		error = copyin(newp, sp, len);
	return (error);
}

/*
 * Validate parameters and get old parameters
 * for a structure oriented sysctl function.
 */
sysctl_rdstruct(oldp, oldlenp, newp, sp, len)
	void *oldp;
	size_t *oldlenp;
	void *newp, *sp;
	int len;
{
	int error = 0;

	if (oldp && *oldlenp < len)
		return (ENOMEM);
	if (newp)
		return (EPERM);
	*oldlenp = len;
	if (oldp)
		error = copyout(sp, oldp, len);
	return (error);
}

/*
 * Get file structures.
 */
sysctl_file(where, sizep)
	char *where;
	size_t *sizep;
{
	int buflen, error;
	register struct file *fp;
	struct	file *fpp;
	char *start = where;
	register int i;

	buflen = *sizep;
	if (where == NULL) {
		for (i = 0, fp = file; fp < fileNFILE; fp++)
			if (fp->f_count) i++;

#define	FPTRSZ	sizeof (struct file *)
#define	FILESZ	sizeof (struct file)
		/*
		 * overestimate by 5 files
		 */
		*sizep = (i + 5) * (FILESZ + FPTRSZ);
		return (0);
	}

	/*
	 * array of extended file structures: first the address then the
	 * file structure.
	 */
	for (fp = file; fp < fileNFILE; fp++) {
		if (fp->f_count == 0)
			continue;
		if (buflen < (FPTRSZ + FILESZ)) {
			*sizep = where - start;
			return (ENOMEM);
		}
		fpp = fp;
		if ((error = copyout(&fpp, where, FPTRSZ)) ||
			(error = copyout(fp, where + FPTRSZ, FILESZ)))
			return (error);
		buflen -= (FPTRSZ + FILESZ);
		where += (FPTRSZ + FILESZ);
	}
	*sizep = where - start;
	return (0);
}

/*
 * This one is in kern_clock.c in 4.4 but placed here for the reasons
 * given earlier (back around line 367).
*/

int
sysctl_clockrate(where, sizep)
	char *where;
	size_t *sizep;
{
	struct	clockinfo clkinfo;

	/*
	 * Construct clockinfo structure.
	*/
	clkinfo.hz = hz;
	clkinfo.tick = mshz;
	clkinfo.profhz = 0;
	clkinfo.stathz = hz;
	return(sysctl_rdstruct(where, sizep, NULL, &clkinfo, sizeof (clkinfo)));
}

/*
 * Dump inode list (via sysctl).
 * Copyout address of inode followed by inode.
 */
/* ARGSUSED */
sysctl_inode(where, sizep)
	char *where;
	size_t *sizep;
{
	register struct inode *ip;
	register char *bp = where;
	struct inode *ipp;
	char *ewhere;
	int error, numi;

	for (numi = 0, ip = inode; ip < inodeNINODE; ip++)
		if (ip->i_count) numi++;

#define IPTRSZ	sizeof (struct inode *)
#define INODESZ	sizeof (struct inode)
	if (where == NULL) {
		*sizep = (numi + 5) * (IPTRSZ + INODESZ);
		return (0);
	}
	ewhere = where + *sizep;
		
	for (ip = inode; ip < inodeNINODE; ip++) {
		if (ip->i_count == 0) 
			continue;
		if (bp + IPTRSZ + INODESZ > ewhere) {
			*sizep = bp - where;
			return (ENOMEM);
		}
		ipp = ip;
		if ((error = copyout((caddr_t)&ipp, bp, IPTRSZ)) ||
			  (error = copyout((caddr_t)ip, bp + IPTRSZ, INODESZ)))
			return (error);
		bp += IPTRSZ + INODESZ;
	}

	*sizep = bp - where;
	return (0);
}

/*
 * Get text structures.  This is a 2.11BSD extension.  sysctl() is supposed
 * to be extensible...
 */
sysctl_text(where, sizep)
	char *where;
	size_t *sizep;
{
	int buflen, error;
	register struct text *xp;
	struct	text *xpp;
	char *start = where;
	register int i;

	buflen = *sizep;
	if (where == NULL) {
		for (i = 0, xp = text; xp < textNTEXT; xp++)
			if (xp->x_count) i++;

#define	TPTRSZ	sizeof (struct text *)
#define	TEXTSZ	sizeof (struct text)
		/*
		 * overestimate by 3 text structures
		 */
		*sizep = (i + 3) * (TEXTSZ + TPTRSZ);
		return (0);
	}

	/*
	 * array of extended file structures: first the address then the
	 * file structure.
	 */
	for (xp = text; xp < textNTEXT; xp++) {
		if (xp->x_count == 0)
			continue;
		if (buflen < (TPTRSZ + TEXTSZ)) {
			*sizep = where - start;
			return (ENOMEM);
		}
		xpp = xp;
		if ((error = copyout(&xpp, where, TPTRSZ)) ||
			(error = copyout(xp, where + TPTRSZ, TEXTSZ)))
			return (error);
		buflen -= (TPTRSZ + TEXTSZ);
		where += (TPTRSZ + TEXTSZ);
	}
	*sizep = where - start;
	return (0);
}

/*
 * try over estimating by 5 procs
 */
#define KERN_PROCSLOP	(5 * sizeof (struct kinfo_proc))

sysctl_doproc(name, namelen, where, sizep)
	int *name;
	u_int namelen;
	char *where;
	size_t *sizep;
{
	register struct proc *p;
	register struct kinfo_proc *dp = (struct kinfo_proc *)where;
	int needed = 0;
	int buflen = where != NULL ? *sizep : 0;
	int doingzomb;
	struct eproc eproc;
	int error = 0;
	dev_t ttyd;
	uid_t ruid;
	struct tty *ttyp;

	if (namelen != 2 && !(namelen == 1 && name[0] == KERN_PROC_ALL))
		return (EINVAL);
	p = (struct proc *)allproc;
	doingzomb = 0;
again:
	for (; p != NULL; p = p->p_nxt) {
		/*
		 * Skip embryonic processes.
		 */
		if (p->p_stat == SIDL)
			continue;
		/*
		 * TODO - make more efficient (see notes below).
		 * do by session.
		 */
		switch (name[0]) {

		case KERN_PROC_PID:
			/* could do this with just a lookup */
			if (p->p_pid != (pid_t)name[1])
				continue;
			break;

		case KERN_PROC_PGRP:
			/* could do this by traversing pgrp */
			if (p->p_pgrp != (pid_t)name[1])
				continue;
			break;

		case KERN_PROC_TTY:
			fill_from_u(p, &ruid, &ttyp, &ttyd);
			if (!ttyp || ttyd != (dev_t)name[1])
				continue;
			break;

		case KERN_PROC_UID:
			if (p->p_uid != (uid_t)name[1])
				continue;
			break;

		case KERN_PROC_RUID:
			fill_from_u(p, &ruid, &ttyp, &ttyd);
			if (ruid != (uid_t)name[1])
				continue;
			break;

		case KERN_PROC_ALL:
			break;
		default:
			return(EINVAL);
		}
		if (buflen >= sizeof(struct kinfo_proc)) {
			fill_eproc(p, &eproc);
			if (error = copyout((caddr_t)p, &dp->kp_proc,
			    sizeof(struct proc)))
				return (error);
			if (error = copyout((caddr_t)&eproc, &dp->kp_eproc,
			    sizeof(eproc)))
				return (error);
			dp++;
			buflen -= sizeof(struct kinfo_proc);
		}
		needed += sizeof(struct kinfo_proc);
	}
	if (doingzomb == 0) {
		p = zombproc;
		doingzomb++;
		goto again;
	}
	if (where != NULL) {
		*sizep = (caddr_t)dp - where;
		if (needed > *sizep)
			return (ENOMEM);
	} else {
		needed += KERN_PROCSLOP;
		*sizep = needed;
	}
	return (0);
}

/*
 * Fill in an eproc structure for the specified process.  Slightly
 * inefficient because we have to access the u area again for the
 * information not kept in the proc structure itself.  Can't afford
 * to expand the proc struct so we take a slight speed hit here.
 */
void
fill_eproc(p, ep)
	register struct proc *p;
	register struct eproc *ep;
{
	struct	tty	*ttyp;

	ep->e_paddr = p;
	fill_from_u(p, &ep->e_ruid, &ttyp, &ep->e_tdev);
	if	(ttyp)
		ep->e_tpgid = ttyp->t_pgrp;
	else
		ep->e_tpgid = 0;
}

/*
 * Three pieces of information we need about a process are not kept in
 * the proc table: real uid, controlling terminal device, and controlling
 * terminal tty struct pointer.  For these we must look in either the u
 * area or the swap area.  If the process is still in memory this is
 * easy but if the process has been swapped out we have to read in the
 * u area.
 *
 * XXX - We rely on the fact that u_ttyp, u_ttyd, and u_ruid are all within
 * XXX - the first 1kb of the u area.  If this ever changes the logic below
 * XXX - will break (and badly).  At the present time (97/9/2) the u area
 * XXX - is 856 bytes long.
*/

fill_from_u(p, rup, ttp, tdp)
	struct	proc	*p;
	uid_t	*rup;
	struct	tty	**ttp;
	dev_t	*tdp;
	{
	register struct	buf	*bp;
	dev_t	ttyd;
	uid_t	ruid;
	struct	tty	*ttyp;
	struct	user	*up;

	if	(p->p_stat == SZOMB)
		{
		ruid = (uid_t)-2;
		ttyp = NULL;
		ttyd = NODEV;
		goto out;
		}
	if	(p->p_flag & SLOAD)
		{
		mapseg5(p->p_addr, (((USIZE - 1) << 8) | RO));
		ttyd = ((struct user *)SEG5)->u_ttyd;
		ttyp = ((struct user *)SEG5)->u_ttyp;
		ruid = ((struct user *)SEG5)->u_ruid;
		normalseg5();
		}
	else
		{
		bp = geteblk();
		bp->b_dev = swapdev;
		bp->b_blkno = (daddr_t)p->p_addr;
		bp->b_bcount = DEV_BSIZE;	/* XXX */
		bp->b_flags = B_READ;

		(*bdevsw[major(swapdev)].d_strategy)(bp);
		biowait(bp);

		if	(u.u_error)
			{
			ttyd = NODEV;
			ttyp = NULL;
			ruid = (uid_t)-2;
			}
		else
			{
			up = (struct user *)mapin(bp);
			ruid = up->u_ruid;	/* u_ruid = offset 164 */
			ttyd = up->u_ttyd;	/* u_ttyd = offset 654 */
			ttyp = up->u_ttyp;	/* u_ttyp = offset 652 */
			mapout(bp);
			}
		bp->b_flags |= B_AGE;
		brelse(bp);
		u.u_error = 0;		/* XXX */
		}
out:
	if	(rup)
		*rup = ruid;
	if	(ttp)
		*ttp = ttyp;
	if	(tdp)
		*tdp = ttyd;
	}

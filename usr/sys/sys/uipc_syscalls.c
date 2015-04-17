/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)uipc_syscalls.c	7.1.3 (2.11BSD) 1999/9/13
 */

#include "param.h"

#include "../machine/seg.h"
#include "../machine/psl.h"

#include "systm.h"
#include "user.h"
#include "proc.h"
#include "file.h"
#include "inode.h"
#include "buf.h"
#include "mbuf.h"
#include "protosw.h"
#include "socket.h"
#include "socketvar.h"
#include "uio.h"
#include "domain.h"
#include "pdpif/if_uba.h"
#include "netinet/in.h"
#include "netinet/in_systm.h"

static void
MBZAP(m, len, type)
	register struct mbuf *m;
	int len, type;
	{
	m->m_next = 0;
	m->m_off = MMINOFF;
	m->m_len = len;
	m->m_type = type;
	m->m_act = 0;
	}

/*
 * System call interface to the socket abstraction.
 */

extern int netoff;
struct file *gtsockf();

socket()
{
	register struct a {
		int	domain;
		int	type;
		int	protocol;
	} *uap = (struct a *)u.u_ap;
	struct socket *so;
	register struct file *fp;

	if (netoff)
		return(u.u_error = ENETDOWN);
	if ((fp = falloc()) == NULL)
		return;
	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_SOCKET;
	u.u_error = SOCREATE(uap->domain, &so, uap->type, uap->protocol);
	if (u.u_error)
		goto bad;
	fp->f_socket = so;
	return;
bad:
	u.u_ofile[u.u_r.r_val1] = 0;
	fp->f_count = 0;
}

bind()
{
	register struct a {
		int	s;
		caddr_t	name;
		u_int	namelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct mbuf *nam;
	char sabuf[MSIZE];

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(uap->s);
	if (fp == 0)
		return;
	nam = (struct mbuf *)sabuf;
	MBZAP(nam, uap->namelen, MT_SONAME);
	if (uap->namelen > MLEN)
		return (u.u_error = EINVAL);
	u.u_error = copyin(uap->name, mtod(nam, caddr_t), uap->namelen);
	if (u.u_error)
		return;
	u.u_error = SOBIND(fp->f_socket, nam);
}

listen()
{
	register struct a {
		int	s;
		int	backlog;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(uap->s);
	if (fp == 0)
		return;
	u.u_error = SOLISTEN(fp->f_socket, uap->backlog);
}

accept()
{
	register struct a {
		int	s;
		caddr_t	name;
		int	*anamelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct mbuf *nam;
	int namelen;
	int s;
	register struct socket *so;
	char sabuf[MSIZE];

	if (netoff)
		return(u.u_error = ENETDOWN);
	if (uap->name == 0)
		goto noname;
	u.u_error = copyin((caddr_t)uap->anamelen, (caddr_t)&namelen,
		sizeof (namelen));
	if (u.u_error)
		return;
#ifndef pdp11
	if (useracc((caddr_t)uap->name, (u_int)namelen, B_WRITE) == 0) {
		u.u_error = EFAULT;
		return;
	}
#endif
noname:
	fp = gtsockf(uap->s);
	if (fp == 0)
		return;
	s = splnet();
	so = fp->f_socket;
	if (SOACC1(so)) {
		splx(s);
		return;
	}
	if (ufalloc(0) < 0) {
		splx(s);
		return;
	}
	fp = falloc();
	if (fp == 0) {
		u.u_ofile[u.u_r.r_val1] = 0;
		splx(s);
		return;
	}
	if (!(so = (struct socket *)ASOQREMQUE(so, 1)))	/* deQ in super */
		panic("accept");
	fp->f_type = DTYPE_SOCKET;
	fp->f_flag = FREAD|FWRITE;
	fp->f_socket = so;
	nam = (struct mbuf *)sabuf;
	MBZAP(nam, 0, MT_SONAME);
	u.u_error = SOACCEPT(so, nam);
	if (uap->name) {
		if (namelen > nam->m_len)
			namelen = nam->m_len;
		/* SHOULD COPY OUT A CHAIN HERE */
		(void) copyout(mtod(nam, caddr_t), (caddr_t)uap->name,
		    (u_int)namelen);
		(void) copyout((caddr_t)&namelen, (caddr_t)uap->anamelen,
		    sizeof (*uap->anamelen));
	}
	splx(s);
}

connect()
{
	register struct a {
		int	s;
		caddr_t	name;
		u_int	namelen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct socket *so;
	struct mbuf *nam;
	int s;
	char sabuf[MSIZE];
	struct	socket	kcopy;

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(uap->s);
	if (fp == 0)
		return;
	if (uap->namelen > MLEN)
		return (u.u_error = EINVAL);
	nam = (struct mbuf *)sabuf;
	MBZAP(nam, uap->namelen, MT_SONAME);
	u.u_error = copyin(uap->name, mtod(nam, caddr_t), uap->namelen);
	if (u.u_error)
		return;
	so = fp->f_socket;
	/*
	 * soconnect was modified to clear the isconnecting bit on errors.
	 * also, it was changed to return the EINPROGRESS error if
	 * nonblocking, etc.
	 */
	u.u_error = SOCON1(so, nam);
	if (u.u_error)
		return;
	/*
	 * i don't think the setjmp stuff works too hot in supervisor mode,
	 * so what is done instead is do the setjmp here and then go back
	 * to supervisor mode to do the "while (isconnecting && !error)
	 * sleep()" loop.
	 */
	s = splnet();
	if (setjmp(&u.u_qsave))
		{
		u.u_error = EINTR;
		goto bad2;
		}
	u.u_error = CONNWHILE(so);
bad2:
	splx(s);
}

socketpair()
{
	register struct a {
		int	domain;
		int	type;
		int	protocol;
		int	*rsv;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp1, *fp2;
	struct socket *so1, *so2;
	int sv[2];

	if	(netoff)
		return(u.u_error = ENETDOWN);
	u.u_error = SOCREATE(uap->domain, &so1, uap->type, uap->protocol);
	if	(u.u_error)
		return;
	u.u_error = SOCREATE(uap->domain, &so2, uap->type, uap->protocol);
	if	(u.u_error)
		goto free;
	fp1 = falloc();
	if (fp1 == NULL)
		goto free2;
	sv[0] = u.u_r.r_val1;
	fp1->f_flag = FREAD|FWRITE;
	fp1->f_type = DTYPE_SOCKET;
	fp1->f_socket = so1;
	fp2 = falloc();
	if (fp2 == NULL)
		goto free3;
	fp2->f_flag = FREAD|FWRITE;
	fp2->f_type = DTYPE_SOCKET;
	fp2->f_socket = so2;
	sv[1] = u.u_r.r_val1;
	u.u_error = SOCON2(so1, so2);
	if (u.u_error)
		goto free4;
	if (uap->type == SOCK_DGRAM) {
		/*
		 * Datagram socket connection is asymmetric.
		 */
		 u.u_error = SOCON2(so2, so1);
		 if (u.u_error)
			goto free4;
	}
	u.u_r.r_val1 = 0;
	(void) copyout((caddr_t)sv, (caddr_t)uap->rsv, 2 * sizeof (int));
	return;
free4:
	fp2->f_count = 0;
	u.u_ofile[sv[1]] = 0;
free3:
	fp1->f_count = 0;
	u.u_ofile[sv[0]] = 0;
free2:
	(void)SOCLOSE(so2);
free:
	(void)SOCLOSE(so1);
}

sendto()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
		caddr_t	to;
		int	tolen;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = uap->to;
	msg.msg_namelen = uap->tolen;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	sendit(uap->s, &msg, uap->flags);
}

send()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	sendit(uap->s, &msg, uap->flags);
}

sendmsg()
{
	register struct a {
		int	s;
		caddr_t	msg;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov[MSG_MAXIOVLEN];

	u.u_error = copyin(uap->msg, (caddr_t)&msg, sizeof (msg));
	if (u.u_error)
		return;
	if ((u_int)msg.msg_iovlen >= sizeof (aiov) / sizeof (aiov[0])) {
		u.u_error = EMSGSIZE;
		return;
	}
	u.u_error =
	    copyin((caddr_t)msg.msg_iov, (caddr_t)aiov,
		(unsigned)(msg.msg_iovlen * sizeof (aiov[0])));
	if (u.u_error)
		return;
	msg.msg_iov = aiov;
	sendit(uap->s, &msg, uap->flags);
}

sendit(s, mp, flags)
	int s;
	register struct msghdr *mp;
	int flags;
{
	register struct file *fp;
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *to, *rights;
	int len;
	char sabuf[MSIZE], ribuf[MSIZE];

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(s);
	if (fp == 0)
		return;
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_offset = 0;				/* XXX */
	auio.uio_resid = 0;
	auio.uio_rw = UIO_WRITE;
	iov = mp->msg_iov;
	for	(i = 0; i < mp->msg_iovlen; i++, iov++)
		{
		if	(iov->iov_len == 0)
			continue;
		auio.uio_resid += iov->iov_len;
		}
	if (mp->msg_name) {
		to = (struct mbuf *)sabuf;
		MBZAP(to, mp->msg_namelen, MT_SONAME);
		u.u_error =
		    copyin(mp->msg_name, mtod(to, caddr_t), mp->msg_namelen);
		if (u.u_error)
			return;
	} else
		to = 0;
	if (mp->msg_accrights) {
		rights = (struct mbuf *)ribuf;
		MBZAP(rights, mp->msg_accrightslen, MT_RIGHTS);
		if (mp->msg_accrightslen > MLEN)
			return(u.u_error = EINVAL);
		u.u_error = copyin(mp->msg_accrights, mtod(rights, caddr_t),
				mp->msg_accrightslen);
		if (u.u_error)
			return;
	} else
		rights = 0;
	len = auio.uio_resid;
	if	(setjmp(&u.u_qsave))
		{
		if	(auio.uio_resid == len)
			return;
		else
			u.u_error = 0;
		}
	else
		u.u_error = SOSEND(fp->f_socket, to, &auio, flags, rights);
	u.u_r.r_val1 = len - auio.uio_resid;
}

recvfrom()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
		caddr_t	from;
		int	*fromlenaddr;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;
	int len;

	u.u_error = copyin((caddr_t)uap->fromlenaddr, (caddr_t)&len,
	   sizeof (len));
	if (u.u_error)
		return;
	msg.msg_name = uap->from;
	msg.msg_namelen = len;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	recvit(uap->s, &msg, uap->flags, (caddr_t)uap->fromlenaddr, (caddr_t)0);
}

recv()
{
	register struct a {
		int	s;
		caddr_t	buf;
		int	len;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = 0;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->len;
	msg.msg_accrights = 0;
	msg.msg_accrightslen = 0;
	recvit(uap->s, &msg, uap->flags, (caddr_t)0, (caddr_t)0);
}

recvmsg()
{
	register struct a {
		int	s;
		struct	msghdr *msg;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	struct msghdr msg;
	struct iovec aiov[MSG_MAXIOVLEN];

	u.u_error = copyin((caddr_t)uap->msg, (caddr_t)&msg, sizeof (msg));
	if (u.u_error)
		return;
	if ((u_int)msg.msg_iovlen >= sizeof (aiov) / sizeof (aiov[0])) {
		u.u_error = EMSGSIZE;
		return;
	}
	u.u_error =
	    copyin((caddr_t)msg.msg_iov, (caddr_t)aiov,
		(unsigned)(msg.msg_iovlen * sizeof(aiov[0])));
	if (u.u_error)
		return;
	recvit(uap->s, &msg, uap->flags,
	    (caddr_t)&uap->msg->msg_namelen,
	    (caddr_t)&uap->msg->msg_accrightslen);
}

recvit(s, mp, flags, namelenp, rightslenp)
	int s;
	register struct msghdr *mp;
	int flags;
	caddr_t namelenp, rightslenp;
{
	register struct file *fp;
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *from, *rights;
	int len, m_freem();

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(s);
	if (fp == 0)
		return;
	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_offset = 0;				/* XXX */
	auio.uio_resid = 0;
	auio.uio_rw = UIO_READ;
	iov = mp->msg_iov;
	for	(i = 0; i < mp->msg_iovlen; i++, iov++)
		{
		if	(iov->iov_len == 0)
			continue;
		auio.uio_resid += iov->iov_len;
		}
	len = auio.uio_resid;
	if	(setjmp(&u.u_qsave))
		{
		if	(auio.uio_resid == len)
			return;
		else
			u.u_error = 0;
		}
	else
		u.u_error = SORECEIVE((struct socket *)fp->f_data,
					&from, &auio,flags, &rights);
	if	(u.u_error)
		return;
	u.u_r.r_val1 = len - auio.uio_resid;
	if (mp->msg_name) {
		len = mp->msg_namelen;
		if (len <= 0 || from == 0)
			len = 0;
		else
			(void) NETCOPYOUT(from, mp->msg_name, &len);
		(void) copyout((caddr_t)&len, namelenp, sizeof(int));
	}
	if (mp->msg_accrights) {
		len = mp->msg_accrightslen;
		if (len <= 0 || rights == 0)
			len = 0;
		else
			(void) NETCOPYOUT(rights, mp->msg_accrights, &len);
		(void) copyout((caddr_t)&len, rightslenp, sizeof(int));
	}
	if (rights)
		M_FREEM(rights);
	if (from)
		M_FREEM(from);
}

shutdown()
{
	register struct a {
		int	s;
		int	how;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(uap->s);
	if (fp == 0)
		return;
	u.u_error = SOSHUTDOWN(fp->f_socket, uap->how);
}

setsockopt()
{
	register struct a {
		int	s;
		int	level;
		int	name;
		caddr_t	val;
		u_int	valsize;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct mbuf *m = NULL;
	char optbuf[MSIZE];

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(uap->s);
	if (fp == 0)
		return;
	if (uap->valsize > MLEN) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->val) {
		m = (struct mbuf *)optbuf;
		MBZAP(m, uap->valsize, MT_SOOPTS);
		u.u_error =
		    copyin(uap->val, mtod(m, caddr_t), (u_int)uap->valsize);
		if (u.u_error)
			return;
	}
	u.u_error = SOSETOPT(fp->f_socket, uap->level, uap->name, m);
}

getsockopt()
{
	register struct a {
		int	s;
		int	level;
		int	name;
		caddr_t	val;
		int	*avalsize;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct mbuf *m = NULL, *m_free();
	int valsize;

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(uap->s);
	if (fp == 0)
		return;
	if (uap->val) {
		u.u_error = copyin((caddr_t)uap->avalsize, (caddr_t)&valsize,
			sizeof (valsize));
		if (u.u_error)
			return;
	} else
		valsize = 0;
	u.u_error =
	    SOGETOPT(fp->f_socket, uap->level, uap->name, &m);
	if (u.u_error)
		goto bad;
	if (uap->val && valsize && m != NULL) {
		u.u_error = NETCOPYOUT(m, uap->val, &valsize);
		if (u.u_error)
			goto bad;
		u.u_error = copyout((caddr_t)&valsize, (caddr_t)uap->avalsize,
		    sizeof (valsize));
	}
bad:
	if (m != NULL)
		M_FREE(m);
}

/*
 * Get socket name.
 */
getsockname()
{
	register struct a {
		int	fdes;
		caddr_t	asa;
		int	*alen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct mbuf *m;
	int len;
	char sabuf[MSIZE];

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(uap->fdes);
	if (fp == 0)
		return;
	u.u_error = copyin((caddr_t)uap->alen, (caddr_t)&len, sizeof (len));
	if (u.u_error)
		return;
	m = (struct mbuf *)sabuf;
	MBZAP(m, 0, MT_SONAME);
	u.u_error = SOGETNAM(fp->f_socket, m);
	if (u.u_error)
		return;
	if (len > m->m_len)
		len = m->m_len;
	u.u_error = copyout(mtod(m, caddr_t), (caddr_t)uap->asa, (u_int)len);
	if (u.u_error)
		return;
	u.u_error = copyout((caddr_t)&len, (caddr_t)uap->alen, sizeof (len));
}

/*
 * Get name of peer for connected socket.
 */
getpeername()
{
	register struct a {
		int	fdes;
		caddr_t	asa;
		int	*alen;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	struct mbuf *m;
	u_int len;
	char sabuf[MSIZE];

	if (netoff)
		return(u.u_error = ENETDOWN);
	fp = gtsockf(uap->fdes);
	if (fp == 0)
		return;
	m = (struct mbuf *)sabuf;
	MBZAP(m, 0, MT_SONAME);
	u.u_error = copyin((caddr_t)uap->alen, (caddr_t)&len, sizeof (len));
	if (u.u_error)
		return;
	u.u_error = SOGETPEER(fp->f_socket, m);
	if (u.u_error)
		return;
	if (len > m->m_len)
		len = m->m_len;
	u.u_error = copyout(mtod(m, caddr_t), (caddr_t)uap->asa, (u_int)len);
	if (u.u_error)
		return;
	u.u_error = copyout((caddr_t)&len, (caddr_t)uap->alen, sizeof (len));
}

#ifndef pdp11
sockargs(aname, name, namelen, type)
	struct mbuf **aname;
	caddr_t name;
	int namelen, type;
{
	register struct mbuf *m;
	int error;
	struct mbuf *m_free();

	if (namelen > MLEN)
		return (EINVAL);
	m = m_get(M_WAIT, type);
	if (m == NULL)
		return (ENOBUFS);
	m->m_len = namelen;
	error = copyin(name, mtod(m, caddr_t), (u_int)namelen);
	if (error)
		(void) m_free(m);
	else
		*aname = m;
	return (error);
}
#endif

struct file *
gtsockf(fdes)
	int fdes;
{
	register struct file *fp;

	fp = getf(fdes);
	if (fp == NULL)
		return (0);
	if (fp->f_type != DTYPE_SOCKET) {
		u.u_error = ENOTSOCK;
		return (0);
	}
	return (fp);
}
